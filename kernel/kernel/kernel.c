#include <stdio.h>

#include <kernel/tty.h>

#include "descriptor_tables.h"

void print(int *arr, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void kernel_main(void) {
    // IRS and segmentation (we implement paging later)
    init_descriptor_tables();
    terminal_initialize();

    // test int 1 - trigger directly
    // asm volatile ("int $0x1");

    // int 14 - page fault
    /*
    int *p = (int *) 0;
    *p = 42; // page fault with error code
    */
    // ok nevermind apparently gcc recognizes this as ub and replaces it with a
    // special op `ud2` meant to signify 'this should never be run'

    // passes if we see debug prints for int #9

    return;
}
