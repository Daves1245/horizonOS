#include <stdint.h>

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
