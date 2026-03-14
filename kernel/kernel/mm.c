#include "mm.h"

phys_addr_t virt_to_phys(virt_addr_t addr) {
#ifdef __x86_64__
    // kernel higher-half: mapped at 0xffffffff80000000 by linker/Limine
    if (addr >= 0xffffffff80000000UL) {
        return addr - 0xffffffff80000000UL;
    }
    // HHDM range: physical memory mapped at hhdm_offset
    return addr - hhdm_offset;
#else
    return (phys_addr_t) addr; // identity mapped in i386
#endif
}
