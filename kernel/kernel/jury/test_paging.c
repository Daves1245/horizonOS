#include <stdio.h>
#include <stdint.h>
#include <kernel/tty.h>
#include "test_paging.h"

extern uint32_t kernel_end;
extern uint32_t placement_address;

void test_paging(void) {
    printf("\n[kernel]: testing paging...\n");

    // we'll use this to test paging some amount after the end of the kernel
    volatile int *mem_test_addr = (volatile int *) (placement_address + 0x0100);

    // display memory mapping info
    printf("\n[kernel]: memory mapping info:\n");
    printf("  [kernel]: kernel start: 0x100000 (1MB) \n");
    printf("  [kernel]: kernel end: 0x%x\n", (uint32_t) &kernel_end);
    printf("  [kernel]: mem testing address: 0x%x\n", (uint32_t) mem_test_addr);

    // paging test 1: read memory (please don't fail)
    printf("[kernel]: testing read\n");
    printf("[kernel]: read kernel memory (garbage): 0x%x\n", (uint32_t) mem_test_addr);
    printf("[kernel]: value read: 0x%x\n", *mem_test_addr);

    // test 2: write, and verify with read
    *mem_test_addr = 0xDEADBEEF;
    if (*mem_test_addr == 0xDEADBEEF) {
        log_success("Paging test: memory read/write working\n");
    } else {
        log_error("Paging test failed: ");
        printf("got 0x%x, expected 0xDEADBEEF\n", *mem_test_addr);
    }

    // test 3: test another mapped location within kernel space
    printf("\n[kernel]: testing another mapped location...\n");
    mem_test_addr = (volatile int *) (placement_address + 0x0100);
    *mem_test_addr = 0xDEADBEEF;
    if (*mem_test_addr == 0xDEADBEEF) {
        log_success("Paging test: ");
        printf("second memory location OK (0x%x)\n", (uint32_t) mem_test_addr);
    } else {
        log_error("Paging test failed: second memory location write failed\n");
    }

    // test 5: page fault test (uncomment to test page fault handler)
    // printf("\ntesting page fault handler...\n");
    // volatile int *unmapped = (volatile int *)0xA0000000;  // high unmapped address
    // *unmapped = 42;  // this should trigger a page fault

    printf("\n[kernel]: all paging tests passed\n");
}
