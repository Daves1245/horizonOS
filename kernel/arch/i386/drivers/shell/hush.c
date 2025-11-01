#include "hush.h"
#include <i386/common/ctype.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <i386/common/logger.h>

static size_t strnlen(const char *s, size_t maxlen) {
    size_t len = 0;
    while (len < maxlen && *s++) len++;
    return len;
}

static size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

static int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char *) s1 - *(unsigned char *) s2;
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
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
}

void cmd_clear(int argc, char **argv) {
    terminal_scrolln(25);
}

static struct hush_command builtin_help = {"help", "show available commands", cmd_help};
static struct hush_command builtin_echo = {"echo", "echo", cmd_echo};
static struct hush_command builtin_clear = {"clear", "clear the screen", cmd_clear};

void hush_register_command(struct hush_command *cmd) {
    if (num_commands < MAX_COMMANDS) {
        hush_registry[num_commands++] = cmd;
    }
}

struct hush_command const *hush_lookup_registry(const char *name) {
    printf("[hush]: looking up command '%s' in %d registered commands\n", name, num_commands);
    for (int i = 0; i < num_commands; i++) {
        printf("[hush]: comparing '%s' with '%s'\n", name, hush_registry[i]->name);
        size_t name_len = strnlen(name, COMMAND_NAME_LEN);
        if (strncmp(hush_registry[i]->name, name, name_len) == 0 && 
            strnlen(hush_registry[i]->name, COMMAND_NAME_LEN) == name_len) {
            printf("[hush]: found match at index %d\n", i);
            return hush_registry[i];
        }
    }
    printf("[hush]: no match found\n");
    return NULL;
}

void hush_init() {
    log_info("[hush]: initializing shell\n");

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
    num_commands = 3;

    log_info("[hush]: registered %d commands\n", num_commands);

    printf("Horizon Utility Shell (hush)\n");
    printf("try 'help' for available commands\n");
    printf(PROMPT);
}

void hush_run() {
    // TODO not currently needed, implement later if this changes
}

enum HUSH_STATE hush_execute_command() {
    printf("[hush]: executing command '%s' with %d args\n", hush_state.name, hush_state.argc);

    if (hush_state.argc < 0) {
        log_warning("[hush]: invalid argc: %d\n", hush_state.argc);
        return INVALID_COMMAND;
    }

    printf("looking up command...\n");
    struct hush_command const *command = hush_lookup_registry(hush_state.name);
    if (!command) {
        printf("[hush]: command not found: '%s'\n", hush_state.name);
        printf("command not found: %s\n", hush_state.name);
        return COMMAND_NOT_FOUND;
    } else {
        printf("found command %s\n", hush_state.command_buffer);
    }

    char* argv[MAX_ARGS];
    for (int i = 0; i < hush_state.argc; i++) {
        argv[i] = hush_state.args[i];
        log_debug("[hush]: arg[%d] = '%s'\n", i, argv[i]);
    }

    log_debug("[hush]: calling handler for '%s'\n", command->name);
    command->handler(hush_state.argc, argv);
    return OK;
}

int parse_command_buffer() {
    printf("[hush]: parsing command buffer: '%s'\n", hush_state.command_buffer);

    char *it = hush_state.command_buffer;
    const char *end = hush_state.command_buffer + MAX_COMMAND_LEN;
    while (*it && iswhitespace(*it) && (it < end)) {
        it++;
    }

    if (!*it || it >= end) {
        log_debug("[hush]: empty command buffer\n");
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

    printf("[hush]: parsed command name: '%s'\n", hush_state.name);

    if (!*it || it >= end) {
        hush_state.argc = 0;
        log_debug("[hush]: no arguments\n");
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
        log_debug("[hush]: parsed arg[%d]: '%s'\n", argc, hush_state.args[argc]);
        argc++;
    }

    hush_state.argc = argc;
    log_debug("[hush]: total args: %d\n", argc);
    return 0;
}

void hush_handle_keypress(char key) {
    if (key == '\n') {
        printf("\n");
        if (hush_state.command_len > 0) {
            hush_state.command_buffer[hush_state.command_len] = '\0';

            printf("DEBUG: buffer len=%d\n", hush_state.command_len);
            printf("DEBUG: buffer hex: ");
            for (int i = 0; i < hush_state.command_len; i++) {
                printf("%02x ", (unsigned char)hush_state.command_buffer[i]);
            }
            printf("\n");
            printf("DEBUG: buffer chars: ");
            for (int i = 0; i < hush_state.command_len; i++) {
                char c = hush_state.command_buffer[i];
                printf("'%c' ", isprintable(c) ? c : '?');
            }
            printf("\n");

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
            printf("\b \b");
        }
    } else if (isprintable(key) && hush_state.command_len < MAX_COMMAND_LEN - 1) {
        hush_state.command_buffer[hush_state.command_len++] = key;
        hush_state.cursor_position++;
    }
}
