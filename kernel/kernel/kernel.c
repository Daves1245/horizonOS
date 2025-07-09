#include <stdio.h>

#include <kernel/tty.h>

#include "descriptor_tables.h"

#include <stdint.h>

void print(int *arr, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

extern void halt_without_apic();
extern int check_msr();

int check_msr() {
    uint32_t eax, ebx, ecx, edx;
    asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    return (edx & (1 << 5)) != 0;
}

void kernel_main(void) {
    // IRS and segmentation (we implement paging later)
    init_descriptor_tables();
    terminal_initialize();

    // test for APIC support
    halt_without_apic();

    printf("APIC supported\n");
    printf("check_msr (apic base msr): %d", check_msr());

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
