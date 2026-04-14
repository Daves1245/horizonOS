/**
 * @file keyboard.c
 * @brief Implementation of the multi-listener keyboard event queue.
 *
 * Implements a fixed-size ring buffer per listener level. The ps2k
 * IRQ handler pushes events; consumers drain them with
 * ::keyboard_poll, ::keyboard_block_read, or the convenience
 * ::readline wrapper.
 */
#include "keyboard.h"
#include <string.h>
#include <drivers/console.h>
#include <drivers/serial.h>
#include <kernel/panic.h>

extern struct key_event_t keyboard_multilevel_queue[KEYBOARD_QUEUE_LEVELS][RING_BUFFER_SIZE];
extern struct keyboard_queue_state keyboard_queue_state[KEYBOARD_QUEUE_LEVELS];

/**
 * @copydoc keyboard_push
 */
void keyboard_push(int level, struct key_event_t entry) {
    struct key_event_t *queue = keyboard_multilevel_queue[level];

    int head = keyboard_queue_state[level].head;
    int next = (head + 1) % RING_BUFFER_SIZE;

    // on full, drop the oldest entry by advancing tail past it.
    // keeping newer input matters more than protecting a slow consumer
    // from seeing a gap; dropping the newest would silently hide keys.
    if (next == keyboard_queue_state[level].tail) {
        keyboard_queue_state[level].tail =
            (keyboard_queue_state[level].tail + 1) % RING_BUFFER_SIZE;
    }

    memcpy(&queue[head], &entry, sizeof(struct key_event_t));
    keyboard_queue_state[level].head = next;
}

/**
 * @copydoc keyboard_poll
 *
 * @note Not yet thread-safe. Safe today because the kernel is
 * uniprocessor and has no preemptive tasks, and we assume consumers
 * drain faster than the buffer fills. A racing push into a
 * currently-reading tail would corrupt silently.
 */
int keyboard_poll(int level, struct key_event_t *out) {
    struct key_event_t *key_queue = keyboard_multilevel_queue[level];
    int head_index = keyboard_queue_state[level].head;
    int tail_index = keyboard_queue_state[level].tail;

    if (tail_index == head_index) {
        return 0;
    }

    out->type = key_queue[tail_index].type;
    out->scan_code = key_queue[tail_index].scan_code;
    out->value = key_queue[tail_index].value;
    out->timestamp = key_queue[tail_index].timestamp;

    keyboard_queue_state[level].tail = (tail_index + 1) % RING_BUFFER_SIZE;
    return 1;
}

/**
 * @copydoc keyboard_block_read
 */
struct key_event_t keyboard_block_read(int level) {
    struct key_event_t out;
    while (!keyboard_poll(level, &out)) asm volatile("hlt");
    return out;
}

/**
 * @copydoc register_keyboard_listener
 *
 * @note Claiming is not atomic; fine on a single processor without
 * tasks, but will need a test-and-set once concurrency is introduced.
 * If the table is full we panic rather than expanding — the expected
 * workload (one focused "window" at a time, optionally a parent that
 * captures input) stays well under ::KEYBOARD_QUEUE_LEVELS.
 * TODO
 */
int register_keyboard_listener() {
    for (int level = 0; level < KEYBOARD_QUEUE_LEVELS; level++) {
        if (!keyboard_queue_state[level].used) {
            keyboard_queue_state[level].used = 1;
            return level;
        }
    }

    // temporary fix involves simply increasing KEYBOARD_QUEUE_LEVELS
    console_puts("[register_keyboard_listener]: multiqueue is full\n");
    panic("keyboard multiqueue full");
}

/**
 * @copydoc readline
 *
 * Backspace rubs out one character from both the buffer and the
 * console. Non-printable bytes outside the range `[' ', 126]` are
 * dropped.
 */
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

/**
 * @copydoc remove_keyboard_listener
 *
 * @brief Frees up the associated @p level for future
 * listener registration.
 *
 * @param level The level assigned to the consumer
 */
void remove_keyboard_listener(int level) {
    // ids are associated with a level, id <-> level
    // reset head, tail, and used flag
    memset(&keyboard_queue_state[level], 0, sizeof(struct keyboard_queue_state));
}
