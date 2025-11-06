#include "hush.h"
#include <i386/common/ctype.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <i386/common/logger.h>
#include <string.h>

#include <jury/test_paging.h>
#include <jury/test_vm.h>

static size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

struct hush_state hush_state;

static struct hush_command *hush_registry[MAX_COMMANDS];
static int num_commands = 0;

void cmd_help(int argc, char **argv) {
    printf("available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        printf("  %s - %s\n", hush_registry[i]->name, hush_registry[i]->description);
    }
}

void cmd_echo(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
}

void cmd_clear(int argc, char **argv) {
    terminal_scrolln(25);
    hush_state.cursor_position = 0;
    terminal_set_cursor(0, 0);
}

void paging_test(int argc, char **argv) {
    test_paging();
}

void vm_test(int argc, char **argv) {
    test_vm();
}

static struct hush_command builtin_help = {"help", "show available commands", cmd_help};
static struct hush_command builtin_echo = {"echo", "echo", cmd_echo};
static struct hush_command builtin_clear = {"clear", "clear the screen", cmd_clear};
static struct hush_command builtin_paging_test = {"test-paging", "run the paging tests", paging_test};
static struct hush_command builtin_vm_test = {"test-vm", "run the vm tests", vm_test};

void hush_register_command(struct hush_command *cmd) {
    if (num_commands < MAX_COMMANDS) {
        hush_registry[num_commands++] = cmd;
    }
}

struct hush_command const *hush_lookup_registry(const char *name) {
#ifdef DEBUG
    printf("[DEBUG] Looking up command: '%s'\n", name);
#endif
    for (int i = 0; i < num_commands; i++) {
        size_t name_len = strnlen(name, COMMAND_NAME_LEN);
#ifdef DEBUG
        printf("[DEBUG] Comparing with registry[%d]: '%s' (len=%zu vs %zu)\n", 
               i, hush_registry[i]->name, name_len, strnlen(hush_registry[i]->name, COMMAND_NAME_LEN));
#endif
        if (strncmp(hush_registry[i]->name, name, name_len) == 0 &&
                strnlen(hush_registry[i]->name, COMMAND_NAME_LEN) == name_len) {
#ifdef DEBUG
            printf("[DEBUG] Match found: '%s'\n", hush_registry[i]->name);
#endif
            return hush_registry[i];
        }
    }
    printf("[hush]: no match found\n");
    return NULL;
}

void hush_init() {
#ifdef DEBUG
    printf("[DEBUG] Debug mode enabled in hush\n");
#endif
    // zero out the registry
    for (int i = 0; i < MAX_COMMAND_LEN; i++) {
        hush_state.command_buffer[i] = 0;
    }
    for (int i = 0; i < COMMAND_NAME_LEN; i++) {
        hush_state.name[i] = 0;
    }
    for (int i = 0; i < MAX_ARGS; i++) {
        for (int j = 0; j < ARG_LEN; j++) {
            hush_state.args[i][j] = 0;
        }
    }

    hush_state.command_len = 0;
    hush_state.cursor_position = 0;
    hush_state.argc = 0;
    hush_state.running = 1;

    hush_register_command(&builtin_help);
    hush_register_command(&builtin_echo);
    hush_register_command(&builtin_clear);
    hush_register_command(&builtin_paging_test);
    hush_register_command(&builtin_vm_test);

    printf("Horizon Utility Shell (hush)\n");
    printf("try 'help' for available commands\n");
    printf(PROMPT);
}

void hush_run() {
    // TODO not currently needed, implement later if this changes
}

enum HUSH_STATE hush_execute_command() {
    if (hush_state.argc < 0) {
        log_warning("[hush]: invalid argc: %d\n", hush_state.argc);
        return INVALID_COMMAND;
    }

    struct hush_command const *command = hush_lookup_registry(hush_state.name);
    if (!command) {
        return COMMAND_NOT_FOUND;
    }

    char* argv[MAX_ARGS];
    for (int i = 0; i < hush_state.argc; i++) {
        argv[i] = hush_state.args[i];
    }

    command->handler(hush_state.argc, argv);
    return OK;
}

int parse_command_buffer() {
#ifdef DEBUG
    printf("[DEBUG] Command buffer: '%s' (len=%zu)\n", hush_state.command_buffer, hush_state.command_len);
    halt();
#endif
    char *it = hush_state.command_buffer;
    const char *end = hush_state.command_buffer + MAX_COMMAND_LEN;
    while (*it && iswhitespace(*it) && (it < end)) {
        it++;
    }

    if (!*it || it >= end) {
        hush_state.argc = -1;
        return -1;
    }

    char *namep = hush_state.name;
    int name_len = 0;
    while (*it && !iswhitespace(*it) && (it < end) && name_len < COMMAND_NAME_LEN - 1) {
        *namep++ = *it++;
        name_len++;
    }
    *namep = '\0';

    if (!*it || it >= end) {
        hush_state.argc = 0;
        return 0;
    }

    int argc = 0;
    while (*it && (it < end)) {
        while (*it && iswhitespace(*it) && (it < end)) it++;

        if (!*it) break;

        if (argc >= MAX_ARGS) break;
        char *dest = hush_state.args[argc];
        int arg_len = 0;
        while (*it && !iswhitespace(*it) && (it < end) && arg_len < ARG_LEN - 1) {
            *dest++ = *it++;
            arg_len++;
        }
        *dest = '\0';
        argc++;
    }

    hush_state.argc = argc;
    return 0;
}

void hush_handle_keypress(char key) {
    if (key == '\n') {
        printf("\n");
        if (hush_state.command_len > 0) {
            hush_state.command_buffer[hush_state.command_len] = '\0';
            parse_command_buffer();
            hush_execute_command();
        }
        hush_state.command_len = 0;
        hush_state.cursor_position = 0;
        printf(PROMPT);
    } else if (key == '\b') {
        if (hush_state.command_len > 0) {
            hush_state.command_len--;
            hush_state.cursor_position--;
        }
    } else if (isprintable(key) && hush_state.command_len < MAX_COMMAND_LEN - 1) {
        hush_state.command_buffer[hush_state.command_len++] = key;
        hush_state.cursor_position++;
    }
}
