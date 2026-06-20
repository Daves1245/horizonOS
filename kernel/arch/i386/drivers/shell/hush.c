#include "hush.h"
#include <i386/common/ctype.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/logger.h>
#include <string.h>
#include <drivers/console.h>
#include <drivers/keyboard/keyboard.h>

#include <jury/i386/test_paging.h>
#include <jury/i386/test_vm.h>
#include <i386/drivers/shell/builtins/maze.h>
#include <games/pong.h>

struct hush_state hush_state;

static int parse_command_buffer(void);

static struct hush_command *hush_registry[MAX_COMMANDS];
static int num_commands = 0;

void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    console_puts("available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        console_printf("  %s - %s\n", hush_registry[i]->name, hush_registry[i]->description);
    }
}

void cmd_echo(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        console_puts(argv[i]);
        if (i < argc - 1) console_putchar(' ');
    }
    console_putchar('\n');
}

void cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    console_clear();
}

void paging_test(int argc, char **argv) {
    test_paging();
}

void vm_test(int argc, char **argv) {
    test_vm();
}

void cmd_maze(int argc, char **argv) {
    generate_maze();
}

void cmd_display(int argc, char **argv) {
    display_maze();
}

void cmd_solve(int argc, char **argv) {
    solve_maze();
}

void cmd_pong(int argc, char **argv) {
    (void)argc; (void)argv;
    pong_start();
}

static struct hush_command builtin_help = {"help", "show available commands", cmd_help};
static struct hush_command builtin_echo = {"echo", "echo", cmd_echo};
static struct hush_command builtin_clear = {"clear", "clear the screen", cmd_clear};
static struct hush_command builtin_paging_test = {"test-paging", "run the paging tests", paging_test};
static struct hush_command builtin_vm_test = {"test-vm", "run the vm tests", vm_test};
static struct hush_command builtin_maze = {"maze", "generate maze", cmd_maze};
static struct hush_command builtin_display = {"display", "display the current maze", cmd_display};
static struct hush_command builtin_solve = {"solve", "solve the current maze", cmd_solve};
static struct hush_command builtin_pong = {"pong", "play pong", cmd_pong};

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
    console_puts("[hush]: no match found\n");
    return NULL;
}

void hush_init() {
    memset(&hush_state, 0, sizeof(hush_state));
    hush_state.running = 1;

    hush_register_command(&builtin_help);
    hush_register_command(&builtin_echo);
    hush_register_command(&builtin_clear);
    hush_register_command(&builtin_paging_test);
    hush_register_command(&builtin_vm_test);
    hush_register_command(&builtin_maze);
    hush_register_command(&builtin_display);
    hush_register_command(&builtin_solve);
    hush_register_command(&builtin_pong);

    console_puts("Horizon Utility Shell (hush)\n");
    console_puts("try 'help' for available commands\n");

    int level = register_keyboard_listener();
    char buf[MAX_COMMAND_LEN];

    while (hush_state.running) {
        console_puts(PROMPT);
        int len = readline(level, buf, sizeof(buf));
        if (len > 0) {
            int copy_len = len < MAX_COMMAND_LEN - 1 ? len : MAX_COMMAND_LEN - 1;
            memcpy(hush_state.command_buffer, buf, copy_len);
            hush_state.command_buffer[copy_len] = '\0';
            hush_state.command_len = copy_len;
            parse_command_buffer();
            hush_execute_command();
        }
    }
}

void hush_run() {
}

enum HUSH_STATE hush_execute_command() {
    if (hush_state.argc < 0) {
        log_warn("[hush]: invalid argc: %d\n", hush_state.argc);
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

static int parse_command_buffer() {
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

