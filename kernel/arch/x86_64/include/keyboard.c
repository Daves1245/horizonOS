#include "keyboard.h"

struct key_event_t key_event_queue[RING_BUFFER_SIZE];
// this is a global data structure, avoid optimizations since
// this acts as a primitive synchronization tool
static volatile int head_index;
static volatile int tail_index;

void keyboard_push(struct key_event_t entry) {
    (void) entry;
}

/*
 * keyboard_poll(): poll the latest keyboard entry
 *
 * returns positive if the value at *out was modified.
 * zero otherwise.
 * */
int keyboard_poll(struct key_event_t *out) {
    if (tail_index == head_index) {
        return 0;
    }

    /* TODO make thread safe. works for now since we're on a single
     * processor without the concept of tasks/processes, and we're
     * assuming we'll read before the ring buffer is full, so that
     * we never push into a currently-reading tail. this will fail
     * silently however, which could be a pain to debug later! */
    out->type = key_event_queue[tail_index].type;
    out->scan_code = key_event_queue[tail_index].scan_code;
    out->value = key_event_queue[tail_index].value;
    out->timestamp = key_event_queue[tail_index].timestamp;

    tail_index++;
}

/*
 * keyboard_block_read(): block until a keyboard event is pushed
 *
 * guaranteed to return a valid struct key_event, given
 * the keyboard is pressed.
 */
struct key_event_t keyboard_block_read() {
    struct key_event_t out;
    while (!keyboard_poll(&out));
    return out;
}

int add_keyboard_listener() {

}

void remove_keyboard_listener() {

}
