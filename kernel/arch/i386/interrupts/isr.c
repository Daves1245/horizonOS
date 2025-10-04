/*
 * High level interrupt service routines and interrupt request handlers.
 * Part of this code is modified from Bran's kernel development tutorials.
 * Rewritten for JamesM's kernel development tutorials.
 *
 */

#include "interrupts/isr.h"
#include <kernel/tty.h>

#include <stdint.h>
#include <stdio.h>

int has_err_code(int n) {
    return (n == 8) || ((n >= 10) && (n <= 14));
}

void debug_print_registers(struct interrupt_context regs) {
    printf("=== REGISTER DUMP ===\n");

    // segment selectors
    printf("DS: 0x%x\n", regs.ds);
    printf("CS: 0x%x\n", regs.cs);
    printf("SS: 0x%x\n", regs.ss);

    // general purpose registers (from pusha)
    printf("EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
           regs.eax, regs.ebx, regs.ecx, regs.edx);
    printf("ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n",
           regs.esi, regs.edi, regs.ebp, regs.esp);

    // control registers
    printf("EIP: 0x%x  EFLAGS: 0x%x\n", regs.eip, regs.eflags);
    printf("User ESP: 0x%x\n", regs.useresp);

    // interrupt info
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

volatile uint16_t *vga_buffer = (uint16_t *) 0xB8000;
int vga_cursor = 0;

void vga_putchar(char c) {
    if (c == '\n') {
        vga_cursor = (vga_cursor / 80 + 1) * 80;
    } else {
        vga_buffer[vga_cursor++] = (uint16_t) c | 0x0F00; // white on black
    }
    if (vga_cursor >= 80 * 25) {
        vga_cursor = 0; // wrap around
    }
}

void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_print_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 7; i >= 0; i--) {
        vga_putchar(hex_chars[(value >> (i * 4)) & 0xF]);
    }
}

// array of interrupt handlers
static isr_t interrupt_handlers[256] = {0};

// register a handler for a specific interrupt
void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

// this gets called from our ASM interrupt handler stub.
void isr_handler(const struct interrupt_context *int_context) {
    // temporary - just print the interrupt number
    volatile uint16_t *vga = (uint16_t *) 0xB8000;
    vga[10] = 0x0F00 | ('0' + int_context->int_no);

    // dispatch to registered handler if exists
    if (interrupt_handlers[int_context->int_no] != 0) {
        interrupt_handlers[int_context->int_no]((struct interrupt_context*)int_context);
    }
    /*
    if (regs->int_no == 13) {
        // use direct VGA output to avoid triggering another GPF
        vga_print("\n*** GENERAL PROTECTION FAULT ***\n");
        vga_print("Error Code: ");
        vga_print_hex(regs->err_code);
        vga_print("\nEIP: ");
        vga_print_hex(regs->eip);
        vga_print("\nCS: ");
        vga_print_hex(regs->cs);
        vga_print("\nESP: ");
        vga_print_hex(regs->esp);
        vga_print("\nEFLAGS: ");
        vga_print_hex(regs->eflags);

        // decode error code
        vga_print("\nerror details: ");
        if (regs->err_code & 1) vga_print("External ");
        else vga_print("Internal ");

        uint32_t table = (regs->err_code >> 1) & 3;
        switch (table) {
            case 0: vga_print("GDT "); break;
            case 1: vga_print("IDT "); break;
            case 2: vga_print("LDT "); break;
            case 3: vga_print("IDT "); break;
        }

        vga_print("selector: ");
        vga_print_hex(regs->err_code & 0xFFF8);
        vga_print("\n*** HALTING ***\n");

        // Halt the system to prevent infinite loop
        asm volatile("cli; hlt");

        // Should never reach here
        for(;;);
    }

    // for all other interrupts, use normal debugging
    debug_print_registers(*regs);
    */
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
