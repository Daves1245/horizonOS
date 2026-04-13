#include "keyboard.h"
#include <string.h>
#include <drivers/console.h>
#include <kernel/panic.h>

extern struct key_event_t keyboard_multilevel_queue[KEYBOARD_QUEUE_LEVELS][RING_BUFFER_SIZE];
extern struct keyboard_queue_state keyboard_queue_state[KEYBOARD_QUEUE_LEVELS];

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

/*
 * keyboard_poll(): poll the latest keyboard entry
 *
 * returns positive if the value at *out was modified.
 * zero otherwise.
 * */
int keyboard_poll(int level, struct key_event_t *out) {
    struct key_event_t *key_queue = keyboard_multilevel_queue[level];
    int head_index = keyboard_queue_state[level].head;
    int tail_index = keyboard_queue_state[level].tail;

    if (tail_index == head_index) {
        return 0;
    }

    /* TODO make thread safe. works for now since we're on a single
     * processor without the concept of tasks/processes, and we're
     * assuming we'll read before the ring buffer is full, so that
     * we never push into a currently-reading tail. this will fail
     * silently however, which could be a pain to debug later! */
    out->type = key_queue[tail_index].type;
    out->scan_code = key_queue[tail_index].scan_code;
    out->value = key_queue[tail_index].value;
    out->timestamp = key_queue[tail_index].timestamp;

    keyboard_queue_state[level].tail = (tail_index + 1) % RING_BUFFER_SIZE;
    return 1;
}

/*
 * keyboard_block_read(): block until a keyboard event is pushed
 *
 * guaranteed to return a valid struct key_event, given
 * the keyboard is pressed.
 */
struct key_event_t keyboard_block_read(int level) {
    struct key_event_t out;
    while (!keyboard_poll(level, &out));
    return out;
}

/*
 * register_keyboard_listener: give a task a keyboard queue to listen to
 *
 * returns an id that maps to a specific level in the multiqueue managed
 * by the ps2k driver. the caller then uses this id to call the read()
 * functions. in the case that there are no open slots, we intentionally
 * crash instead of failing silently. since we don't plan on expanding
 * much for now (would only really make sense if e.g. we have lots of
 * windows that concurrently want keyboard access, but only one window
 * will be focused at a time - at best, we'd have a parent that captures
 * input like in a VM), this suffices for now. TODO
 */
int register_keyboard_listener() {
    for (int level = 0; level < KEYBOARD_QUEUE_LEVELS; level++) {
        // TODO make test_set atomic claiming to avoid future data races
        if (!keyboard_queue_state[level].used) {
            keyboard_queue_state[level].used = 1;
            return level;
        }
    }

    // temporary fix involves simply increasing KEYBOARD_QUEUE_LEVELS
    console_puts("[register_keyboard_listener]: multiqueue is full\n");
    panic("keyboard multiqueue full");
}

/*
 *
 */
void remove_keyboard_listener(int level) {
    // ids are associated with a level, id <-> level
    // reset head, tail, and used flag
    memset(&keyboard_queue_state[level], 0, sizeof(struct keyboard_queue_state));
}
