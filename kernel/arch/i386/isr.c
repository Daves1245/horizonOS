/*
 * High level interrupt service routines and interrupt request handlers.
 * Part of this code is modified from Bran's kernel development tutorials.
 * Rewritten for JamesM's kernel development tutorials.
 *
 */

#include "common.h"
#include "isr.h"
#include <kernel/tty.h>

#include <stdio.h>

int has_err_code(int n) {
    return (n == 8) || ((n >= 10) && (n <= 14));
}

void debug_print_registers(struct registers regs) {
    printf("=== REGISTER DUMP ===\n");

    // Segment selectors
    printf("DS: 0x%x\n", regs.ds);
    printf("CS: 0x%x\n", regs.cs);
    printf("SS: 0x%x\n", regs.ss);

    // General purpose registers (from pusha)
    printf("EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
           regs.eax, regs.ebx, regs.ecx, regs.edx);
    printf("ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n",
           regs.esi, regs.edi, regs.ebp, regs.esp);

    // Control registers
    printf("EIP: 0x%x  EFLAGS: 0x%x\n", regs.eip, regs.eflags);
    printf("User ESP: 0x%x\n", regs.useresp);

    // Interrupt info
    printf("Interrupt: %d (0x%x)", regs.int_no, regs.int_no);
    if (has_err_code(regs.int_no)) {
        printf("  Error Code: %d (0x%x)", regs.err_code, regs.err_code);
    }
    printf("\n");

    // EFLAGS breakdown
    printf("EFLAGS bits: ");
    if (regs.eflags & (1 << 0)) printf("CF ");
    if (regs.eflags & (1 << 2)) printf("PF ");
    if (regs.eflags & (1 << 4)) printf("AF ");
    if (regs.eflags & (1 << 6)) printf("ZF ");
    if (regs.eflags & (1 << 7)) printf("SF ");
    if (regs.eflags & (1 << 8)) printf("TF ");
    if (regs.eflags & (1 << 9)) printf("IF ");
    if (regs.eflags & (1 << 10)) printf("DF ");
    if (regs.eflags & (1 << 11)) printf("OF ");
    printf("\n");

    printf("==================\n");
}

// This gets called from our ASM interrupt handler stub.
void isr_handler(struct registers *regs) {
    debug_print_registers(*regs);
}

void isr_0() {}
void isr_1() {}
void isr_2() {}
void isr_3() {}
void isr_4() {}
void isr_5() {}
void isr_6() {}
void isr_7() {}
void isr_8(int errno) {}
void isr_9() {}
void isr_10(int errno) {}
void isr_11(int errno) {}
void isr_12(int errno) {}
void isr_13(int errno) {}
void isr_14(int errno) {}
void isr_15() {}
void isr_16() {}
void isr_17() {}
void isr_18() {}
void isr_19() {}
void isr_20() {}
void isr_21() {}
void isr_22() {}
void isr_23() {}
void isr_24() {}
void isr_25() {}
void isr_26() {}
void isr_27() {}
void isr_28() {}
void isr_29() {}
void isr_30() {}
void isr_31() {}
