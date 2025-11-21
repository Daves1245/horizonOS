#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <kernel/types.h>
#include <common.h>

virt_addr_t kmalloc_a(uint32_t sz); // page aligned
virt_addr_t kmalloc_p(uint32_t sz, phys_addr_t *phys); // returns virtual address and fills phys with physical address
virt_addr_t kmalloc_ap(uint32_t sz, phys_addr_t *phys); // page aligned, returns virtual address and fills phys
virt_addr_t kmalloc(uint32_t sz); // vanilla

#endif
