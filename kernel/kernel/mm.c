#include "mm.h"

phys_addr_t virt_to_phys(virt_addr_t addr) {
#ifdef __x86_64__
    return addr - hhdm_offset; // hhdm in x86_64
#else
    return (phys_addr_t) addr; // identity mapped in i386
#endif
}
