#ifndef LOCK_H
#define LOCK_H

#include <stdatomic.h>

/* Thin API wrapper around stdatomic's atomic_flag type */
typedef atomic_flag lock_t;

#define INIT_LOCK (lock_t) ATOMIC_FLAG_INIT

void spinlock(lock_t *lock);
void release(lock_t *lock);

#endif
