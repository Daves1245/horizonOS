#include "keyboard.h"
#include "../../interrupts/isr.h"
#include "../../apic/apic.h"
#include <stdint.h>
#include <stdio.h>
#include <halt.h>
#include <kernel/tty.h>
#include <i386/common/io.h>
#include <i386/common/ctype.h>
#include <i386/drivers/shell/hush.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

enum SCANCODE {
    ESCAPE = 27,
    ONE = '1',
    TWO = '2',
    THREE = '3',
    FOUR = '4',
    FIVE = '5',
    SIX = '6',
    SEVEN = '7',
    EIGHT = '8',
    NINE = '9',
    ZERO = '0',
    HYPHEN = '-',
    EQUALS = '=',
    BACKSPACE = '\b',
    TAB = '\t',
    q = 'q',
    w = 'w',
    e = 'e',
    r = 'r',
    t = 't',
    y = 'y',
    u = 'u',
    i = 'i',
    o = 'o',
    p = 'p',
    Q = 'Q',
    W = 'W',
    E = 'E',
    R = 'R',
    T = 'T',
    Y = 'Y',
    U = 'U',
    I = 'I',
    O = 'O',
    P = 'P',
    OPEN_BRACKET = '[',
    CLOSE_BRACKET = ']',
    OPEN_BRACE = '{',
    CLOSE_BRACE = '}',
    ENTER = '\n',
    LEFT_CTRL = 1,
    a = 'a',
    s = 's',
    d = 'd',
    f = 'f',
    g = 'g',
    h = 'h',
    j = 'j',
    k = 'k',
    l = 'l',
    A = 'A',
    S = 'S',
    D = 'D',
    F = 'F',
    G = 'G',
    H = 'H',
    J = 'J',
    K = 'K',
    L = 'L',
    SEMICOLON = ';',
    COLON = ':',
    SINGLE_QUOTE = '\'',
    DOUBLE_QUOTE = '"',
    BACKTICK = '`',
    TILDE = '~',
    LEFT_SHIFT = 2,
    BACK_SLASH = '\\',
    PIPE = '|',
    z = 'z',
    x = 'x',
    c = 'c',
    v = 'v',
    b = 'b',
    n = 'n',
    m = 'm',
    Z = 'Z',
    X = 'X',
    C = 'C',
    V = 'V',
    B = 'B',
    N = 'N',
    M = 'M',
    COMMA = ',',
    LESS_THAN = '<',
    PERIOD = '.',
    GREATER_THAN = '>',
    FORWARD_SLASH = '/',
    QUESTION_MARK = '?',
    RIGHT_SHIFT = 3,
    KEYPAD_ASTERISK = '*',
    LEFT_ALT = 4,
    SPACE = ' '
};

static char scancode_to_ascii[] = {
    0,  ESCAPE, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, ZERO, HYPHEN, EQUALS, BACKSPACE,
    TAB, q, w, e, r, t, y, u, i, o, p, OPEN_BRACKET, CLOSE_BRACKET, ENTER,
    LEFT_CTRL, a, s, d, f, g, h, j, k, l, SEMICOLON, SINGLE_QUOTE, BACKTICK, LEFT_SHIFT,
    BACK_SLASH, z, x, c, v, b, n, m, COMMA, PERIOD, FORWARD_SLASH, RIGHT_SHIFT, KEYPAD_ASTERISK, LEFT_ALT, SPACE
};

static char scancode_to_ascii_uppercase[] = {
    0,  ESCAPE, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', BACKSPACE,
    TAB, Q, W, E, R, T, Y, U, I, O, P, OPEN_BRACE, CLOSE_BRACE, ENTER,
    LEFT_CTRL, A, S, D, F, G, H, J, K, L, COLON, DOUBLE_QUOTE, TILDE, LEFT_SHIFT,
    PIPE, Z, X, C, V, B, N, M, LESS_THAN, GREATER_THAN, QUESTION_MARK, RIGHT_SHIFT, KEYPAD_ASTERISK, LEFT_ALT, SPACE
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
#ifdef DEBUG
    log_debug("[keyboard]: timeout waiting for input buffer\n");
#endif
}

// wait for keyboard controller output buffer to be full
static int wait_for_kbd_output() {
    int timeout = 100000;
    while (timeout-- > 0) {
        if (inb(KEYBOARD_STATUS_PORT) & 0x01) {
            return 1;
        }
    }
#ifdef DEBUG
    log_debug("[keyboard]: timeout waiting for output buffer\n");
#endif
    return 0;
}

// initialize PS/2 keyboard controller hardware
static void init_ps2_controller() {
#ifdef DEBUG
    log_debug("[keyboard]: initializing PS/2 controller...\n");
#endif

    // disable both PS/2 ports
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xAD);  // disable first port
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xA7);  // disable second port

    // flush output buffer
    inb(KEYBOARD_DATA_PORT);

    // read controller configuration byte
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0x20);
    if (wait_for_kbd_output()) {
        uint8_t config = inb(KEYBOARD_DATA_PORT);

        // enable keyboard interrupt (bit 0) and translation (bit 6)
        config |= 0x01;   // enable keyboard interrupt
        config |= 0x40;   // enable translation to scan code set 1

        // write back configuration
        wait_for_kbd_input();
        outb(KEYBOARD_STATUS_PORT, 0x60);
        wait_for_kbd_input();
        outb(KEYBOARD_DATA_PORT, config);
    }

    // perform controller self-test
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xAA);
    if (wait_for_kbd_output()) {
        uint8_t result = inb(KEYBOARD_DATA_PORT);
        if (result == 0x55) {
#ifdef DEBUG
            log_success("[keyboard]: controller self-test passed\n");
#endif
        } else {
            log_error("Controller self-test failed: ");
            printf("0x%x\n", result);
            halt();
        }
    }

    // test keyboard port
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xAB);
    if (wait_for_kbd_output()) {
        uint8_t result = inb(KEYBOARD_DATA_PORT);
        if (result == 0x00) {
#ifdef DEBUG
            log_success("[keyboard]: keyboard port test passed\n");
#endif
        } else {
            log_error("Keyboard port test failed: ");
            printf("0x%x\n", result);
        }
    }

    // enable keyboard port
    wait_for_kbd_input();
    outb(KEYBOARD_STATUS_PORT, 0xAE);

    // reset keyboard device
    wait_for_kbd_input();
    outb(KEYBOARD_DATA_PORT, 0xFF);
    if (wait_for_kbd_output()) {
        uint8_t ack = inb(KEYBOARD_DATA_PORT);
        if (ack == 0xFA) {
#ifdef DEBUG
            log_debug("[keyboard]: keyboard reset ACK received\n");
#endif
            // wait for self-test result
            if (wait_for_kbd_output()) {
                uint8_t result = inb(KEYBOARD_DATA_PORT);
                if (result == 0xAA) {
#ifdef DEBUG
                    log_success("[keyboard]: keyboard self-test passed\n");
#endif
                } else {
                    log_error("[keyboard]: keyboard self-test result: ");
                    printf("0x%x\n", result);
                    halt();
                }
            }
        } else {
            log_error("unexpected response to reset: ");
            printf("0x%x\n", ack);
            halt();
        }
    }

    // enable scanning
    wait_for_kbd_input();
    outb(KEYBOARD_DATA_PORT, 0xF4);
    if (wait_for_kbd_output()) {
        uint8_t ack = inb(KEYBOARD_DATA_PORT);
        if (ack == 0xFA) {
#ifdef DEBUG
            log_success("[keyboard]: scanning enabled\n");
#endif
        } else {
            log_error("[keyboard]: failed to enable scanning: ");
            printf("0x%x\n", ack);
            halt();
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
    // ignore key releases (high bit set)
    else if (!(scancode & 0x80)) {
        // convert to ASCII and print
        if (scancode < sizeof(scancode_to_ascii)) {
            char c;
            if (shift_pressed) {
                c = scancode_to_ascii_uppercase[scancode];
            } else {
                c = scancode_to_ascii[scancode];
            }

            if (c != 0) {
                printf("%c", c);
                hush_handle_keypress(c);
            }
        }
    }

    // send apic EOI (End of Interrupt)
    volatile uint32_t *apic_eoi = (volatile uint32_t *)(APIC_BASE + APIC_EOI);
    *apic_eoi = 0;
}

void setup_keyboard_irq() {
    // register keyboard interrupt handler for IRQ 1 (vector 33 after PIC remap)
    register_interrupt_handler(33, keyboard_interrupt_handler);
}

void init_keyboard() {
#ifdef DEBUG
    log_info("[keyboard]: initializing PS/2 keyboard driver\n");
#endif

    // initialize PS/2 controller hardware
    init_ps2_controller();

    // register interrupt handler
    setup_keyboard_irq();

    log_success("[keyboard]: keyboard driver ready\n");
}
