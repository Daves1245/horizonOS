#include <stdio.h>
#include <stdint.h>

#include <kernel/tty.h>
#include "memory/paging.h"
#include "gdt/descriptor_tables.h"
#include "common/common.h"
#include "apic/madt.h"
#include "apic/rsdp.h"

extern void halt_without_apic();

int check_msr() {
    uint32_t eax, ebx, ecx, edx;
    asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    return (edx & (1 << 5)) != 0;
}

// declare kernel_end from linker script
extern uint32_t kernel_end;

// declare placement_address from kheap.c
extern uint32_t placement_address;

void kernel_main(void) {
    // we'll use this to test paging some amount after the end of the kernel
    placement_address = (uint32_t) &kernel_end;
    volatile int *mem_test_addr = (volatile int *) (placement_address + 0x0100);

    // IRS and segmentation (we implement paging later)
    init_descriptor_tables();
    terminal_initialize();

    // initialize paging
    printf("[kernel]: initializing paging...\n");
    init_paging();
    printf("[kernel]: paging enabled!\n");

    // test for APIC support
    halt_without_apic();

    printf("[kernel]: APIC supported\n");
    printf("[kernel]: check_msr (apic base msr): %d", check_msr());

    printf("\n[kernel]: testing paging...\n");

    // display memory mapping info
    printf("\n[kernel]: memory mapping info:\n");
    printf("  [kernel]: kernel start: 0x100000 (1MB) \n");
    printf("  [kernel]: kernel end: 0x%x\n", (uint32_t) &kernel_end);
    printf("  [kernel]: mem testing address: 0x%x\n", (uint32_t) mem_test_addr);


    // paging test 1: read memory (should work)
    printf("[kernel]: testing read\n");
    printf("[kernel]: read kernel memory (garbage): 0x%x\n", (uint32_t) mem_test_addr);
    printf("[kernel]: value read: 0x%x\n", *mem_test_addr);

    // test 2: write, and verify with read
    *mem_test_addr = 0xDEADBEEF;
    if (*mem_test_addr == 0xDEADBEEF) {
        printf("[kernel]: paging test PASSED: memory read/write working\n");
    } else {
        printf("[kernel]: paging test FAILED: got 0x%x, expected 0xDEADBEEF\n", *mem_test_addr);
    }

    // test 3: test another mapped location within kernel space
    printf("\n[kernel]: testing another mapped location...\n");
    mem_test_addr = (volatile int *) (placement_address + 0x1000);
    *mem_test_addr = 0xDEADBEEF;
    if (*mem_test_addr == 0xDEADBEEF) {
        printf("[kernel]: paging test PASSED: Second memory location OK (0x%x)\n", (uint32_t) mem_test_addr);
    } else {
        printf("[kernel]: paging test FAILED: Second memory location write failed\n");
    }

        // test 5: page fault test (uncomment to test page fault handler)
    // printf("\ntesting page fault handler...\n");
    // volatile int *unmapped = (volatile int *)0xA0000000;  // high unmapped address
    // *unmapped = 42;  // this should trigger a page fault

    printf("\n[kernel]: all paging tests passed\n");

    // Initialize ACPI/APIC system
    printf("\n=== ACPI/APIC Initialization ===\n");
    initialize_apic();

    return;
}
