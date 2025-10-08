#include "keyboard.h"
#include "../../interrupts/isr.h"
#include <stdint.h>
#include <stdio.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

static char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

static int shift_pressed = 0;

void keyboard_interrupt_handler(struct interrupt_context *regs) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    // left / right shift pressed
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }

    // left / right shift released
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
        return;
    }

    // Ignore key releases (high bit set)
    if (scancode & 0x80) {
        return;
    }

    // convert to ASCII and print
    if (scancode < sizeof(scancode_to_ascii)) {
        char c;
        if (shift_pressed) {
            c = scancode_to_ascii_shift[scancode];
        } else {
            c = scancode_to_ascii[scancode];
        }

        if (c != 0) {
            printf("%c", c);
        }
    }
}

void setup_keyboard_irq() {
    // Register keyboard interrupt handler for IRQ 1 (vector 33 after PIC remap)
    register_interrupt_handler(33, keyboard_interrupt_handler);
}

void init_keyboard() {
    printf("[keyboard]: initializing PS/2 keyboard driver\n");
    setup_keyboard_irq();
    printf("[keyboard]: keyboard driver ready\n");
}
