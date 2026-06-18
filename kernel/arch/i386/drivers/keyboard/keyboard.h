#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <interrupts/isr.h>
#include <stdint.h>

#define RING_BUFFER_SIZE      100
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

struct keyboard_queue_state {
    volatile int head;
    volatile int tail;
    int used;
};

extern struct key_event_t keyboard_multilevel_queue[KEYBOARD_QUEUE_LEVELS][RING_BUFFER_SIZE];
extern struct keyboard_queue_state keyboard_queue_state[KEYBOARD_QUEUE_LEVELS];

void keyboard_push(int level, struct key_event_t entry);
int keyboard_poll(int level, struct key_event_t *out);
struct key_event_t keyboard_block_read(int level);
int register_keyboard_listener(void);
void remove_keyboard_listener(int level);
int readline(int level, char *buf, int len);

void setup_keyboard_irq(void);
void keyboard_interrupt_handler(struct interrupt_context *);
void init_keyboard(void);
int is_key_pressed(uint8_t scan_code);

#endif
