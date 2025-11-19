#include <stdio.h>
#include <stdint.h>
#include <kernel/tty.h>
#include "test_paging.h"

extern uint32_t kernel_end;
extern uint32_t placement_address;

void test_paging(void) {
#ifdef DEBUG
    log_debug("[kernel]: testing paging...\n");
#endif

    // we'll use this to test paging some amount after the end of the kernel
    volatile int *mem_test_addr = (volatile int *) (placement_address + 0x0100);

#ifdef DEBUG
    // display memory mapping info
    log_debug("\n[kernel]: memory mapping info:\n");
    log_debug("  [kernel]: kernel start: 0x100000 (1MB) \n");
    log_debug("  [kernel]: kernel end: 0x%x\n", (uint32_t) &kernel_end);

    log_debug("  [kernel]: mem testing address: 0x%x\n", (uint32_t) mem_test_addr);

    // paging test 1: read memory (please don't fail)
    log_debug("[kernel]: testing read\n");
    log_debug("[kernel]: read kernel memory (garbage): 0x%x\n", (uint32_t) mem_test_addr);
    log_debug("[kernel]: value read: 0x%x\n", *mem_test_addr);
#endif

    // test 2: write, and verify with read
    *mem_test_addr = 0xDEADBEEF;
    if (*mem_test_addr != 0xDEADBEEF) {
        log_error("Paging test failed: ");
        printf("got 0x%x, expected 0xDEADBEEF\n", *mem_test_addr);
        return;
    }

    // test 3: test another mapped location within kernel space
    mem_test_addr = (volatile int *) (placement_address + 0x0100);
    *mem_test_addr = 0xDEADBEEF;
    if (*mem_test_addr != 0xDEADBEEF) {
        log_error("Paging test failed: second memory location write failed\n");
        return;
    }

    // test 5: page fault test (uncomment to test page fault handler)
    // printf("\ntesting page fault handler...\n");
    // volatile int *unmapped = (volatile int *)0xA0000000;  // high unmapped address
    // *unmapped = 42;  // this should trigger a page fault

    log_success("[paging]: all paging tests passed\n");
}
