#include <stdint.h>
#include <stdio.h>

void init_paging(void) {
    // no need - limine sets this up for us
}

void map_physical_range(uint64_t phys_addr, int iskernel, int writeable) {
    // nop - handled by limine
}
