#include "keyboard.h"
#include "../../interrupts/isr.h"
#include "../../apic/apic.h"
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>

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

// Wait for keyboard controller input buffer to be empty
static void wait_for_kbd_input() {
    int timeout = 100000;
    while (timeout-- > 0) {
        if ((inb(KEYBOARD_STATUS_PORT) & 0x02) == 0) {
            return;
        }
    }
    log_warning("Timeout waiting for input buffer\n");
}

// Wait for keyboard controller output buffer to be full
static int wait_for_kbd_output() {
    int timeout = 100000;
    while (timeout-- > 0) {
        if (inb(KEYBOARD_STATUS_PORT) & 0x01) {
            return 1;
        }
    }
    log_warning("Timeout waiting for output buffer\n");
    return 0;
}

// Initialize PS/2 keyboard controller hardware
static void init_ps2_controller() {
    log_debug("[keyboard]: initializing PS/2 controller...\n");
    
    // Disable both PS/2 ports
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xAD);  // Disable first port
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xA7);  // Disable second port
    
    // Flush output buffer
    inb(KEYBOARD_DATA_PORT);
    
    // Read controller configuration byte
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0x20);
    if (wait_for_kbd_output()) {
        uint8_t config = inb(KEYBOARD_DATA_PORT);
        
        // Enable keyboard interrupt (bit 0) and translation (bit 6)
        config |= 0x01;   // Enable keyboard interrupt
        config |= 0x40;   // Enable translation to scan code set 1
        
        // Write back configuration
        wait_for_kbd_input();
        outb(KEYBOARD_STATUS_PORT, 0x60);
        wait_for_kbd_input();
        outb(KEYBOARD_DATA_PORT, config);
    }
    
    // Perform controller self-test
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xAA);
    if (wait_for_kbd_output()) {
        uint8_t result = inb(KEYBOARD_DATA_PORT);
        if (result == 0x55) {
            log_success("[keyboard]: controller self-test passed\n");
        } else {
            log_error("Controller self-test failed: ");
            printf("0x%x\n", result);
        }
    }
    
    // Test keyboard port
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xAB);
    if (wait_for_kbd_output()) {
        uint8_t result = inb(KEYBOARD_DATA_PORT);
        if (result == 0x00) {
            log_success("[keyboard]: keyboard port test passed\n");
        } else {
            log_error("Keyboard port test failed: ");
            printf("0x%x\n", result);
        }
    }
    
    // Enable keyboard port
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xAE);
    
    // Reset keyboard device
    wait_for_kbd_input();
    outb(KEYBOARD_DATA_PORT, 0xFF);
    if (wait_for_kbd_output()) {
        uint8_t ack = inb(KEYBOARD_DATA_PORT);
        if (ack == 0xFA) {
            log_debug("[keyboard]: keyboard reset ACK received\n");
            // Wait for self-test result
            if (wait_for_kbd_output()) {
                uint8_t result = inb(KEYBOARD_DATA_PORT);
                if (result == 0xAA) {
                    log_success("[keyboard]: keyboard self-test passed\n");
                } else {
                    log_warning("Keyboard self-test result: ");
                    printf("0x%x\n", result);
                }
            }
        } else {
            log_warning("Unexpected response to reset: ");
            printf("0x%x\n", ack);
        }
    }
    
    // Enable scanning
    wait_for_kbd_input();
    outb(KEYBOARD_DATA_PORT, 0xF4);
    if (wait_for_kbd_output()) {
        uint8_t ack = inb(KEYBOARD_DATA_PORT);
        if (ack == 0xFA) {
            log_success("[keyboard]: scanning enabled\n");
        } else {
            log_error("Failed to enable scanning: ");
            printf("0x%x\n", ack);
        }
    }
    
    log_success("[keyboard]: PS/2 controller initialized\n");
}

void keyboard_interrupt_handler(struct interrupt_context *regs) {
    (void)regs;  // Unused but required by handler signature
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    

    // left / right shift pressed
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    }
    // left / right shift released
    else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
    }
    // Ignore key releases (high bit set)
    else if (!(scancode & 0x80)) {
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
    
    // Send APIC EOI (End of Interrupt)
    // This is CRITICAL - without it, no more interrupts will be processed
    volatile uint32_t *apic_eoi = (volatile uint32_t *)(APIC_BASE + APIC_EOI);
    *apic_eoi = 0;
}

void setup_keyboard_irq() {
    // Register keyboard interrupt handler for IRQ 1 (vector 33 after PIC remap)
    register_interrupt_handler(33, keyboard_interrupt_handler);
}

void init_keyboard() {
    log_info("[keyboard]: initializing PS/2 keyboard driver\n");
    
    // Initialize PS/2 controller hardware
    init_ps2_controller();
    
    // Register interrupt handler
    setup_keyboard_irq();
    
    log_success("[keyboard]: keyboard driver ready\n");
}
