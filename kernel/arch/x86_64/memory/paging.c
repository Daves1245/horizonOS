#include <stdint.h>
#include <stdio.h>

// TODO: Proper x86_64 paging implementation
// For now, Limine sets up paging for us

void init_paging(void) {
    // for now, limine sets up paging for us
}

// Stub implementations for VM tests
void map_page(uint32_t virt_addr, uint32_t phys_addr, int iskernel, int writeable) {
    (void)virt_addr;
    (void)phys_addr;
    (void)iskernel;
    (void)writeable;
    printf("[x86_64 paging stub]: map_page not implemented\n");
}

void unmap_page(uint32_t virt_addr) {
    (void)virt_addr;
    printf("[x86_64 paging stub]: unmap_page not implemented\n");
}

int is_page_mapped(uint32_t virt_addr) {
    (void)virt_addr;
    return 0;  // always return not mapped
}
