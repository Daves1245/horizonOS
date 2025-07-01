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

    __asm__ volatile ("int $0x0");
    __asm__ volatile ("int $0x1");
    __asm__ volatile ("int $0x2");
    __asm__ volatile ("int $0x3");
    __asm__ volatile ("int $0x4");

    return;
}
