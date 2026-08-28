#ifndef KMALLOC_WRAPPER_H
#define KMALLOC_WRAPPER_H

#include <stdint.h>
#include "../../../kernel/kheap.h"

// Wrapper functions to handle 32-bit kmalloc return values in 64-bit environment
static inline void* kmalloc_64(uint32_t sz) {
    return (void*)(uint64_t)kmalloc(sz);
}

static inline void* kmalloc_a_64(uint32_t sz) {
    return (void*)(uint64_t)kmalloc_a(sz);
}

#endif
