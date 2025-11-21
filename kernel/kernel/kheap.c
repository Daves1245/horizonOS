#include <stdint.h>

#include "kheap.h"
#include <common.h>

virt_addr_t placement_address;

virt_addr_t kmalloc_a(uint32_t sz) {
    // align to page boundary if not already aligned
    if (placement_address & 0xFFF) {
        placement_address &= ~0xFFFUL;
        placement_address += 0x1000;
    }
    virt_addr_t ret = placement_address;
    placement_address += sz;
    return ret;
}

virt_addr_t kmalloc_p(uint32_t sz, phys_addr_t *phys) {
    *phys = placement_address;
    return kmalloc(sz);
}

virt_addr_t kmalloc_ap(uint32_t sz, phys_addr_t *phys) {
    // align to page boundary if not already aligned
    if (placement_address & 0xFFF) {
        placement_address &= ~0xFFFUL;
        placement_address += 0x1000;
    }
    *phys = placement_address;
    return kmalloc(sz);
}

virt_addr_t kmalloc(uint32_t sz) {
    virt_addr_t ret = placement_address;
    placement_address += sz;
    return ret;
}
