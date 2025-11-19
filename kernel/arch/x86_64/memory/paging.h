#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

// TODO
void init_paging(void);
void map_physical_range(uint64_t phys_addr, uint32_t size, int iskernel, int writeable);

#endif
