#include "shell.h"
#include "keyboard.h"
#include <string.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <log.h>
#include <kernel/types.h>

#include <games/pong.h>

static char input_buffer[SHELL_BUFFER_SIZE];
static int shell_listener_id;

static void cmd_help(void) {
    console_puts("commands: help, clear, echo, uptime, peek, pong\n");
}

static void cmd_clear(void) {
    console_clear();
}

static void cmd_echo(const char *args) {
    console_puts(args);
    console_putchar('\n');
}

static void cmd_uptime() {
    console_printf("uptime: ms since boot: %d\n", timer_ticks());
}

static virt_addr_t parse_hex(const char *s, const char **endp) {

    // skip the 0x hex specifier
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    virt_addr_t val = 0;
    // per-char parsing. handle digits and letters appropriately
    while (*s) {
        uint8_t nibble;
        if (*s >= '0' && *s <= '9') {
            nibble = *s - '0';
        } else if (*s >= 'a' && *s <= 'f') {
            nibble = *s - 'a' + 10;
        } else if (*s >= 'A' && *s <= 'F') {
            nibble = *s - 'A' + 10;
        } else {
            break;
        }
        val = (val << 4) | nibble;
        s++;
    }
    if (endp) {
        *endp = s;
    }
    return val;
}

static void cmd_peek(const char *args) {
    if (!args || args[0] == '\0') {
        console_puts("usage: peek <addr> [len]\n");
        return;
    }

    const char *end;
    virt_addr_t addr = parse_hex(args, &end);

    virt_addr_t len = 64;
    while (*end == ' ') end++;
    if (*end != '\0') {
        len = parse_hex(end, NULL);
    }
    if (len == 0 || len > 512) {
        len = 64;
    }

    volatile uint8_t *mem = (volatile uint8_t *)addr;
    for (virt_addr_t i = 0; i < len; i += 16) {
        virt_addr_t row = addr + i;
        console_printf("%08x%08x  ", (uint32_t)(row >> 32), (uint32_t)row);

        for (virt_addr_t j = 0; j < 16; j++) {
            if (i + j < len) {
                console_printf("%02x ", mem[i + j]);
            } else {
                console_puts("   ");
            }
            if (j == 7) {
                console_putchar(' ');
            }
        }

        console_puts(" |");
        for (virt_addr_t j = 0; j < 16 && i + j < len; j++) {
            uint8_t c = mem[i + j];
            console_putchar(c >= 0x20 && c < 0x7f ? c : '.');
        }
        console_puts("|\n");
    }
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
    } else if (strcmp(line, "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(line, "pong") == 0) {
        pong_start();
    } else if (strcmp(line, "peek") == 0) {
        cmd_peek(args);
    } else {
        console_puts("unknown command: ");
        console_puts(line);
        console_putchar('\n');
    }

    // TODO sysinfo, lspci, snake, doom, poke, hexdump, uname, color test
    // change theme to something from maybe coloors or some other pallate-selector site.
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
