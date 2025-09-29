#ifndef ISR_H
#define ISR_H

#include "common/common.h"

struct interrupt_context {
    u32int ds;
		// this is the pusha order, verified by the intel 64 manual
		// we write it in reverse since this is the way it appears
		// in assembly?? TODO
		u32int edi, esi, ebp, esp, ebx, edx, ecx, eax;
		u32int int_no, err_code;                       // isr_common_stub
    u32int eip, cs, eflags, useresp, ss;           // cpu on interrupt
} __attribute__((packed));

// Function pointer type for interrupt handlers
typedef void (*isr_t)(struct interrupt_context*);

// Register a handler for a specific interrupt
void register_interrupt_handler(u8int n, isr_t handler);

#endif
