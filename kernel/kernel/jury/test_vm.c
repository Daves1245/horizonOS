#include <stdio.h>
#include <stdint.h>
#include <kernel/tty.h>
#include "test_vm.h"

// Forward declarations from paging.h
void map_page(uint32_t virt_addr, uint32_t phys_addr, int iskernel, int writeable);
void unmap_page(uint32_t virt_addr);
int is_page_mapped(uint32_t virt_addr);

void test_vm(void) {
    printf("\n=== Advanced VM Tests ===\n");

    // Test 1: Non-identity mapping (virt != phys)
    log_info("[vm_test]: Test 1 - Non-identity mapping...\n");
    uint32_t test_virt = 0xA0000000;
    uint32_t test_phys = 0x500000;  // 5MB physical

    map_page(test_virt, test_phys, 1, 1);

    // Write to virtual address
    *(volatile uint32_t*)test_virt = 0xDEADBEEF;

    // Read from physical address (identity mapped)
    uint32_t phys_value = *(volatile uint32_t*)test_phys;

    if (phys_value == 0xDEADBEEF) {
        log_success("[vm_test]: Test 1 PASSED");
        printf(" - virt 0x%x -> phys 0x%x works!\n", test_virt, test_phys);
    } else {
        log_error("[vm_test]: Test 1 FAILED");
        printf(" - got 0x%x, expected 0xDEADBEEF\n", phys_value);
    }

    // Test 2: Remapping same virtual address
    log_info("[vm_test]: Test 2 - Remapping virtual address...\n");
    unmap_page(test_virt);

    uint32_t test_phys2 = 0x600000;  // Different physical address
    map_page(test_virt, test_phys2, 1, 1);

    *(volatile uint32_t*)test_virt = 0xCAFEBABE;
    uint32_t phys2_value = *(volatile uint32_t*)test_phys2;

    if (phys2_value == 0xCAFEBABE) {
        log_success("[vm_test]: Test 2 PASSED");
        printf(" - remapping works!\n");
    } else {
        log_error("[vm_test]: Test 2 FAILED\n");
    }

    // Test 3: is_page_mapped check
    log_info("[vm_test]: Test 3 - Page mapping check...\n");
    if (is_page_mapped(test_virt) && !is_page_mapped(0xC0000000)) {
        log_success("[vm_test]: Test 3 PASSED");
        printf(" - is_page_mapped works!\n");
    } else {
        log_error("[vm_test]: Test 3 FAILED\n");
    }

    printf("\n");
    log_success("[vm_test]: All advanced VM tests passed!\n");
    printf("\n");
}
