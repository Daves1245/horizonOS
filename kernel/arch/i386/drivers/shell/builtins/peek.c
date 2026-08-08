#include <drivers/console.h>
#include <kernel/types.h>
#include <stddef.h>
#include <stdint.h>

static virt_addr_t parse_hex(const char *s, const char **endp) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    virt_addr_t val = 0;
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

void cmd_peek(int argc, char **argv) {
    if (argc < 1) {
        console_puts("usage: peek <addr> [len]\n");
        return;
    }

    const char *end;
    virt_addr_t addr = parse_hex(argv[0], &end);

    virt_addr_t len = 64;
    if (argc >= 2) {
        len = parse_hex(argv[1], NULL);
    }
    if (len == 0 || len > 512) {
        len = 64;
    }

    volatile uint8_t *mem = (volatile uint8_t *)addr;
    for (virt_addr_t i = 0; i < len; i += 16) {
        console_printf("%08x  ", addr + i);

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
