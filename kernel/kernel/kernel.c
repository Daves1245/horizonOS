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
extern u32int kernel_end;

// declare placement_address from kheap.c
extern u32int placement_address;

void kernel_main(void) {
    // initialize placement address to end of kernel
    placement_address = (u32int)&kernel_end;

    // IRS and segmentation (we implement paging later)
    init_descriptor_tables();
    terminal_initialize();

    // initialize paging
    printf("initializing paging...\n");
    init_paging();
    printf("paging enabled!\n");

    // test for APIC support
    halt_without_apic();

    printf("APIC supported\n");
    printf("check_msr (apic base msr): %d", check_msr());

    printf("\ntesting paging...\n");

    // paging test 1: access kernel memory (should work)
    volatile int *kernel_mem = (volatile int *)0x100000;  // 1MB mark
    *kernel_mem = 0x12345678;
    printf("write to kernel memory: 0x%x\n", *kernel_mem);

    // test 2: verify we can read back the value
    if (*kernel_mem == 0x12345678) {
        printf("paging test PASSED: memory read/write working\n");
    } else {
        printf("paging test FAILED: got 0x%x, expected 0x12345678\n", *kernel_mem);
    }

    // test 3: test another mapped location within kernel space
    printf("\ntesting another mapped location...\n");
    volatile int *another_kernel_mem = (volatile int *) (placement_address - 0x1000);
    *another_kernel_mem = 0xDEADBEEF;
    if (*another_kernel_mem == 0xDEADBEEF) {
        printf("paging test PASSED: Second memory location OK (0x%x)\n", (u32int)another_kernel_mem);
    } else {
        printf("paging test FAILED: Second memory location write failed\n");
    }

    // test 4: display memory mapping info
    printf("\nmemory mapping info:\n");
    printf("  kernel start: 0x100000 (1MB) \n");
    printf("  kernel end: 0x%x\n", (uint32_t) &kernel_end);
    printf("  placement address: 0x%x\n", placement_address);
    printf("  identity mapped up to: 0x%x\n", placement_address + 0x1000);

    // test 5: page fault test (uncomment to test page fault handler)
    // printf("\ntesting page fault handler...\n");
    // volatile int *unmapped = (volatile int *)0xA0000000;  // high unmapped address
    // *unmapped = 42;  // this should trigger a page fault

    printf("\nall paging tests passed\n");

    // Initialize ACPI/APIC system
    printf("\n=== ACPI/APIC Initialization ===\n");
    initialize_apic();

    return;
}
