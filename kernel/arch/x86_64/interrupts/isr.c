#include <stdint.h>
#include "descriptor_tables.h"
#include "isr.h"
#include <kernel/panic.h>

static isr_t interrupt_handlers[256] = {0};

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

static inline uint8_t com1_lsr(void) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"((uint16_t)(0x3F8 + 5)));
    return val;
}

static inline void com1_putc(char c) {
    while (!(com1_lsr() & 0x20));
    asm volatile("outb %0, %1" : : "a"(c), "Nd"((uint16_t)0x3F8));
}

static void com1_write(const char *s) {
    while (*s) com1_putc(*s++);
}

static void com1_hex64(uint64_t val) {
    const char *h = "0123456789ABCDEF";
    com1_write("0x");
    for (int i = 60; i >= 0; i -= 4)
        com1_putc(h[(val >> i) & 0xF]);
}

void isr_handler(struct interrupt_context *regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        interrupt_handlers[regs->int_no](regs);
        return;
    }

    if (regs->int_no < 32) {
        com1_write("\n[EXCEPTION] vec=");
        com1_hex64(regs->int_no);
        com1_write(" err=");
        com1_hex64(regs->err_code);
        com1_write(" rip=");
        com1_hex64(regs->rip);
        com1_write(" cr2=");
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        com1_hex64(cr2);
        com1_write("\n");
        panic("unhandled CPU exception");
    }
}
