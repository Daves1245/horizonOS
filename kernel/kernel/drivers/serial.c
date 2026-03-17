#include <stdint.h>
#include <stdarg.h>

#define COM1_PORT 0x3F8

int serial_initialized = 0;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void init_serial() {
    serial_initialized = 0;

    // Disable interrupts
    outb(COM1_PORT + 1, 0x00);

    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 3, 0x80);

    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);

    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 3, 0x03);

    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 2, 0xC7);

    // IRQs enabled, RTS/DSR set
    outb(COM1_PORT + 4, 0x0B);

    serial_initialized = 1;
}

int serial_transmit_empty() {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_putchar(char c) {
    while (serial_transmit_empty() == 0);
    outb(COM1_PORT, c);
}

void serial_write(const char *str) {
    while (*str) {
        serial_putchar(*str++);
    }
}

static void serial_write_uint(uint64_t val, int base) {
    const char digits[] = "0123456789abcdef";
    // uint max - 1e19 + 1 null byte
    // whoops, if we pass in base = 2 then
    // we need 64 bits to display this
    char buf[65] = {0};
    int i = 64;
    buf[i] = '\0';
    if (val == 0) {
        serial_putchar('0');
        return;
    }
    while (val && i > 0) {
        buf[--i] = digits[val % base];
        val /= base;
    }
    serial_write(&buf[i]);
}

static void serial_write_int(int64_t val) {
    if (val < 0) {
        serial_putchar('-');
        val = -val;
    }
    serial_write_uint((uint64_t) val, 10);
}

void serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            serial_putchar(*p);
            continue;
        }
        p++;
        switch (*p) {
            case 'd':
                serial_write_int(va_arg(args, int64_t));
                break;
            case 'u':
                serial_write_uint(va_arg(args, uint64_t), 10);
                break;
            case 'x':
                serial_write_uint(va_arg(args, uint64_t), 16);
                break;
            case 'p':
                serial_write("0x");
                serial_write_uint((uint64_t)(uintptr_t) va_arg(args, void *), 16);
                break;
            case 's':
                serial_write(va_arg(args, const char *));
                break;
            case 'c':
                serial_putchar((char) va_arg(args, unsigned int));
                break;
            case '%':
                serial_putchar('%');
                break;
            default:
                serial_putchar('%');
                serial_putchar(*p);
                break;
        }
    }

    va_end(args);
}

void serial_write_hex(const char *prefix, uint32_t val) {
    serial_write(prefix);
    serial_write("0x");
    static const char hex[] = "0123456789abcdef";
    char buf[9];
    buf[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    serial_write(buf);
    serial_write("\n");
}
