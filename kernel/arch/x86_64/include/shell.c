#include "shell.h"
#include "keyboard.h"
#include <string.h>
#include <drivers/serial.h>
#include <log.h>

static char input_buffer[SHELL_BUFFER_SIZE];
static int shell_listener_id;

static void cmd_help(void) {
    console_puts("commands: help, clear, echo\n");
}

static void cmd_clear(void) {
    console_clear();
}

static void cmd_echo(const char *args) {
    console_puts(args);
    console_putchar('\n');
}

/*
 * parse_and_dispatch: minimal skeleton. split on first space to
 * separate command from args, then strcmp against known commands.
 * real parser (quoting, piping, etc.) goes here later.
 */
static void parse_and_dispatch(char *line) {
    if (line[0] == '\0') return;

    char *args = line;
    while (*args && *args != ' ') args++;
    if (*args == ' ') {
        *args++ = '\0';
        while (*args == ' ') args++;
    }

    if (strcmp(line, "help") == 0) {
        cmd_help();
    } else if (strcmp(line, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(line, "echo") == 0) {
        cmd_echo(args);
    } else {
        console_puts("unknown command: ");
        console_puts(line);
        console_putchar('\n');
    }
}

void shell_init(void) {
    shell_listener_id = register_keyboard_listener();
    printk(KERN_INFO, "shell registered with id: %d\n", shell_listener_id);
}

void shell_run(void) {
    for (;;) {
        console_puts("> ");
        int n = readline(shell_listener_id, input_buffer, SHELL_BUFFER_SIZE);
        (void) n;
        parse_and_dispatch(input_buffer);
    }
}
