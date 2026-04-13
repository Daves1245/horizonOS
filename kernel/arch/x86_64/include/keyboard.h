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

/* non-blocking */
struct key_event_t keyboard_read(int level);
/* blocking */
struct key_event_t keyboard_block_read(int level);
/* some task that wants to track keyboard input */
int add_keyboard_listener();
void remove_keybord_listener(int level);

#endif
