#ifndef MM_H
#define MM_H

#include <stdint.h>

#ifdef __x86_64__
typedef uint64_t virt_addr_t;
typedef uint64_t phys_addr_t;
extern uint64_t hhdm_offset;
#else
typedef uint32_t virt_addr_t;
typedef uint32_t phys_addr_t;
#endif

phys_addr_t virt_to_phys(virt_addr_t addr);

#endif
