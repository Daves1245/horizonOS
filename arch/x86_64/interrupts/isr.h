#ifndef X86_64_ISR_H
#define X86_64_ISR_H

#include <stdint.h>

struct interrupt_context {
    uint64_t ds;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

typedef void (*isr_t)(struct interrupt_context*);

void isr_handler(struct interrupt_context *regs);
void register_interrupt_handler(uint8_t n, isr_t handler);

#endif
