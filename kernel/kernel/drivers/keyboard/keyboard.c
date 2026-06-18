#include "keyboard.h"
#include <interrupts/isr.h>
#include <apic/apic.h>
#include <stdint.h>
#include <stdio.h>
#include <halt.h>
#include <kernel/tty.h>
#include <drivers/io.h>
#ifdef __i386__
#include <string.h>
#include <drivers/console.h>
#include <kernel/panic.h>
#include <drivers/timer.h>
#endif

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

#ifdef __i386__
static uint8_t key_state[256];
static int extended_prefix = 0;

int is_key_pressed(uint8_t scan_code) {
    return key_state[scan_code];
}

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
#endif /* __i386__ */

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
    (void)regs;
#ifdef __i386__
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode == 0xE0) {
        extended_prefix = 1;
        apic_send_eoi();
        return;
    }

    int was_extended = extended_prefix;
    extended_prefix = 0;

    uint8_t base = scancode & 0x7F;
    int is_release = (scancode & 0x80) != 0;

    /* index extended keys into the upper half of key_state (base | 0x80) */
    uint8_t key_idx = was_extended ? (base | 0x80) : base;
    key_state[key_idx] = !is_release;

    /* track shift state for ascii translation */
    if (!was_extended) {
        if (base == 0x2A || base == 0x36)
            shift_pressed = !is_release;
    }

    /* queue key-down events for readline (non-extended only — no ascii for arrows) */
    if (!is_release && !was_extended && base < sizeof(scancode_to_ascii)) {
        char c = shift_pressed
            ? scancode_to_ascii_uppercase[base]
            : scancode_to_ascii[base];

        if (c != 0) {
            struct key_event_t ev = {
                .type      = KEY_EVENT_DOWN,
                .scan_code = (int8_t) base,
                .value     = (int8_t) c,
                .timestamp = (int8_t) timer_ticks(),
            };
            for (int lvl = 0; lvl < KEYBOARD_QUEUE_LEVELS; lvl++) {
                if (keyboard_queue_state[lvl].used)
                    keyboard_push(lvl, ev);
            }
        }
    }

    apic_send_eoi();
#endif
}

void setup_keyboard_irq() {
    // register keyboard interrupt handler for IRQ 1 (vector 33 after PIC remap)
    register_interrupt_handler(33, keyboard_interrupt_handler);
}

void init_keyboard() {
#ifdef DEBUG
    log_info("[keyboard]: initializing PS/2 keyboard driver\n");
#endif

    init_ps2_controller();
    setup_keyboard_irq();

    log_success("[keyboard]: keyboard driver ready\n");
}

#ifdef __i386__

struct key_event_t keyboard_multilevel_queue[KEYBOARD_QUEUE_LEVELS][RING_BUFFER_SIZE];
struct keyboard_queue_state keyboard_queue_state[KEYBOARD_QUEUE_LEVELS];

void keyboard_push(int level, struct key_event_t entry) {
    struct key_event_t *queue = keyboard_multilevel_queue[level];
    int head = keyboard_queue_state[level].head;
    int next = (head + 1) % RING_BUFFER_SIZE;

    if (next == keyboard_queue_state[level].tail)
        keyboard_queue_state[level].tail =
            (keyboard_queue_state[level].tail + 1) % RING_BUFFER_SIZE;

    memcpy(&queue[head], &entry, sizeof(struct key_event_t));
    keyboard_queue_state[level].head = next;
}

int keyboard_poll(int level, struct key_event_t *out) {
    int head = keyboard_queue_state[level].head;
    int tail = keyboard_queue_state[level].tail;
    if (tail == head) return 0;
    *out = keyboard_multilevel_queue[level][tail];
    keyboard_queue_state[level].tail = (tail + 1) % RING_BUFFER_SIZE;
    return 1;
}

struct key_event_t keyboard_block_read(int level) {
    struct key_event_t out;
    while (!keyboard_poll(level, &out)) asm volatile("hlt");
    return out;
}

int register_keyboard_listener(void) {
    for (int level = 0; level < KEYBOARD_QUEUE_LEVELS; level++) {
        if (!keyboard_queue_state[level].used) {
            keyboard_queue_state[level].used = 1;
            return level;
        }
    }
    panic("keyboard multiqueue full");
    return -1;
}

void remove_keyboard_listener(int level) {
    memset(&keyboard_queue_state[level], 0, sizeof(struct keyboard_queue_state));
}

int readline(int level, char *buf, int len) {
    int pos = 0;
    if (len <= 0) return 0;

    for (;;) {
        struct key_event_t ev = keyboard_block_read(level);
        if (ev.type != KEY_EVENT_DOWN) continue;

        char c = (char) ev.value;
        if (c == '\n') {
            console_putchar('\n');
            buf[pos] = '\0';
            return pos;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                console_backspace();
            }
        } else if (c >= ' ' && c < 127) {
            if (pos < len - 1) {
                buf[pos++] = c;
                console_putchar(c);
            }
        }
    }
}

#endif /* __i386__ */
