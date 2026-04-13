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

#include <stddef.h>

#define RING_BUFFER_SIZE 100

struct key_event_t {
		int8_t type;
		int8_t scan_code;
		int8_t value;
		int8_t timestamp;
};

extern struct key_event_t key_event_queue[RING_BUFFER_SIZE];
extern int key_queue_head_idx;
extern int key_queue_tail_idx;

/* non-blocking */
struct key_event keyboard_read();
/* blocking */
struct key_event keyboard_block_read();
/* some task that wants to track keyboard input */
int add_keyboard_listener();
void remove_keybord_listener();

#endif
