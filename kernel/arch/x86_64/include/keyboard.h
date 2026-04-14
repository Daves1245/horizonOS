/**
 * @file keyboard.h
 * @brief Higher-level keyboard event API.
 *
 * Higher-level keyboard abstractions built on top of the raw PS/2
 * driver in `drivers/ps2k`. ps2k handles the hardware interface and
 * raw I/O; this module provides an API for querying keyboard events
 * (which keys were pressed, in what order) and dispatching them to
 * multiple listeners through a multi-level ring-buffer queue.
 *
 * This prevents hard-coding a keyboard events interface to a single
 * task and easily allows to add later features such as focusing
 * windows when a GUI will be included.
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

/** @brief Capacity of each per-listener ring buffer, in events. */
#define RING_BUFFER_SIZE 100
/** @brief Maximum number of concurrent keyboard listeners. Intentionally
 * kept relatively small because not many tasks should be listening
 * to the keyboard at the same time. */
#define KEYBOARD_QUEUE_LEVELS 10

/**
 * @brief Kind of key event carried in ::key_event_t.
 *
 * Depicts if a key was pressed down or released, with an empty
 * field as well..
 */
enum key_event_type {
    KEY_EVENT_NONE = 0, /**< Empty / uninitialized slot. */
    KEY_EVENT_DOWN = 1, /**< Key was pressed (make code). */
    KEY_EVENT_UP   = 2, /**< Key was released (break code). */
};

/**
 * @brief Single keyboard event delivered to a listener.
 *
 * Keep track of if the key was held down or released, the raw
 * scan code from the PS/2 driver, the translated ascii value if
 * it's valid, and the time in ms uptime since boot that the key
 * event was triggered. The timestamp will be useful if we ever
 * decide to do logic for key holds, debouncing, or other configurations.
 * TODO
 */
struct key_event_t {
    int8_t type;       /**< One of ::key_event_type. */
    int8_t scan_code;  /**< Raw PS/2 scan code. */
    int8_t value;      /**< Translated character value, if any. */
    int8_t timestamp;  /**< Timestamp of the event in ms since boot. */
};

/**
 * @brief Per-listener ring buffers indexed by queue level.
 *
 * The ps2k IRQ will push events into the queue via
 * ::keyboard_push and the queue will be drained by any listener
 * through a call to ::keyboard_poll / ::keyboard_block_read with
 * the respective id given from the `register_keyboard_listener()`
 * call. We assume tasks act in good-faith and provide the id that
 * was given to them, and the id is valid. TODO
 */
extern struct key_event_t keyboard_multilevel_queue[KEYBOARD_QUEUE_LEVELS][RING_BUFFER_SIZE];

/**
 * @brief Head/tail indices and in-use flag for one listener queue.
 */
struct keyboard_queue_state {
    volatile int head; /**< Next slot to write. */
    volatile int tail; /**< Next slot to read. */
    int used;          /**< Non-zero if this level has a registered listener. */
};

/**
 * @brief Push a key event onto the ring buffer for @p level.
 *
 * Called from the ps2k IRQ handler. On a full buffer, advances the tail
 * to drop the oldest entry — keeping newer input is preferred over
 * silently hiding recent keys from a slow consumer.
 *
 * @param level Queue level (listener id) to push into.
 * @param entry Event to enqueue (copied by value).
 */
void keyboard_push(int level, struct key_event_t entry);

/**
 * @brief Non-blocking read of the next event at @p level.
 *
 * @param level Queue level to read from.
 * @param out   Destination for the event; only written on success.
 * @return 1 if @p out was filled, 0 if the queue was empty.
 */
int keyboard_poll(int level, struct key_event_t *out);

/**
 * @brief Block until a key event is available on @p level.
 *
 * Halts the CPU between polls. Returns the first event observed.
 *
 * @param level Queue level to read from.
 * @return The dequeued event.
 */
struct key_event_t keyboard_block_read(int level);

/**
 * @brief Claim an unused queue level for a new listener.
 *
 * Panics if all ::KEYBOARD_QUEUE_LEVELS slots are in use rather than
 * failing silently. If all levels are filled, we have a bigger
 * architectural issue to fix before addressing this.
 *
 * @return The newly-claimed level id, to be passed to the read() API.
 */
int register_keyboard_listener(void);

/**
 * @brief Release a previously-claimed queue level.
 *
 * Clears head/tail and the in-use flag so the slot can be reused.
 *
 * @param level Level id returned by ::register_keyboard_listener.
 */
void remove_keyboard_listener(int level);

/**
 * @brief Read a line of input into a caller-owned buffer.
 *
 * Echoes keystrokes (including backspace) to the console and returns
 * when the user presses Enter. The buffer is always null-terminated.
 *
 * TODO XXX this should be a library function, and shouldn't echo to the
 * console - instead, that should be handled by the tty + console
 * automatically.
 *
 * @param level Listener level to read events from.
 * @param buf Destination buffer.
 * @param len Size of @p buf in bytes, including the null terminator.
 * @return Number of bytes written (excluding the terminator).
 */
int readline(int level, char *buf, int len);

#endif
