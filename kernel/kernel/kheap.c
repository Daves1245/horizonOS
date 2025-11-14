#include <stdint.h>

#include "kheap.h"
#include <common.h>

uint32_t placement_address;

uint32_t kmalloc_a(uint32_t sz) {
    // align to page boundary if not already aligned
    if (placement_address & 0xFFF) {
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    uint32_t ret = placement_address;
    placement_address += sz;
    return ret;
}

uint32_t kmalloc_p(uint32_t sz, uint32_t *phys) {
    *phys = placement_address;
    return kmalloc(sz);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys) {
    // align to page boundary if not already aligned
    if (placement_address & 0xFFF) {
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    *phys = placement_address;
    return kmalloc(sz);
}

uint32_t kmalloc(uint32_t sz) {
    uint32_t ret = placement_address;
    placement_address += sz;
    return ret;
}
