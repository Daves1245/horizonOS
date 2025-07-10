#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

#include "../arch/i386/include/common.h"

uint32_t kmalloc_a(uint32_t sz); // page aligned
uint32_t kmalloc_p(uint32_t sz, uint32_t *phys); // returns a physical address
uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys); // page aligned and returns a physical address
uint32_t kmalloc(uint32_t sz); // vanilla

#endif
