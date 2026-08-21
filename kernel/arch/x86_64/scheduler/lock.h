#ifndef LOCK_H
#define LOCK_H

#include <stdatomic.h>

/* Thin API wrapper around stdatomic's atomic_flag type */
typedef atomic_flag lock_t;

#define INIT_LOCK (lock_t) ATOMIC_FLAG_INIT

// according to the documentation at [1], atomic_flag is the only type
// that is *guaranteed* to be lock-free, while the other implementations are
// *usually* lock-free.
//
// https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/stdatomic.h.html,

// NOTE
// there is overhead by writing these in a function. inlining can be forced by writing
// these as a macro

void spinlock(lock_t *lock) {
  // explicit: atomic read-modify-write operation. reads the current state,
  // writes the new state, and returns the value of the previous state

  // memory_order_acquire:
  while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
    __builtin_ia32_pause();
  }
}

void release(lock_t *lock) {
  atomic_flag_clear_explicit(lock, memory_order_release);
}

#endif
