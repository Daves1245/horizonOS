#ifndef KEYBOARD_H
#define KEYBOARD_H
/*
 * higher-level keyboard abstractions. for interfacing
 * with hardware, check the ps2k keyboard driver in
 * drivers/ps2k.
 *
 * ps2k handles interfacing with the hardware and raw
 * I/O. keyboard provides an API for querying keyboard
 * events, such as what keys were pressed in what order.
 * this allows
 */
#include <stdint.h>

#define RING_BUFFER_SIZE 100
#define KEYBOARD_QUEUE_LEVELS 10

enum key_event_type {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_DOWN = 1,
    KEY_EVENT_UP   = 2,
};

struct key_event_t {
    int8_t type;
    int8_t scan_code;
    int8_t value;
    int8_t timestamp;
};

extern struct key_event_t keyboard_multilevel_queue[KEYBOARD_QUEUE_LEVELS][RING_BUFFER_SIZE];

struct keyboard_queue_state {
    volatile int head;
    volatile int tail;
    int used;
};

void keyboard_push(int level, struct key_event_t entry);
int keyboard_poll(int level, struct key_event_t *out);
struct key_event_t keyboard_block_read(int level);
int register_keyboard_listener(void);
void remove_keyboard_listener(int level);

/*
 * readline: fill caller-owned buffer with a line of input.
 * echoes keystrokes (including backspace) to the console.
 * returns number of bytes written once the user hits enter.
 * buf is always null-terminated. len includes the terminator.
 */
int readline(int level, char *buf, int len);

#endif
