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

    asm volatile ("int $0x1");
    asm volatile ("int $0x2");
    asm volatile ("int $0x3");
    asm volatile ("int $0x4");
    asm volatile ("int $0x5");
    asm volatile ("int $0x6");
    asm volatile ("int $0x7");
    asm volatile ("int $0x8");
    asm volatile ("int $0x9");
    asm volatile ("int $0xA");
    asm volatile ("int $0xB");
    asm volatile ("int $0xC");
    asm volatile ("int $0xD");
    asm volatile ("int $0xE");
    asm volatile ("int $0xF");

    // passes if we see debug prints for int #9

    return;
}
