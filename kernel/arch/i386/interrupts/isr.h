#ifndef ISR_H
#define ISR_H

#include <stdint.h>

struct interrupt_context {
    uint32_t ds;
		// this is the pusha order, verified by the intel 64 manual
		// we write it in reverse since this is the way it appears
		// in assembly?? TODO
		uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
		uint32_t int_no, err_code;                       // isr_common_stub
    uint32_t eip, cs, eflags, useresp, ss;           // cpu on interrupt
} __attribute__((packed));

// Function pointer type for interrupt handlers
typedef void (*isr_t)(struct interrupt_context*);

// Register a handler for a specific interrupt
void register_interrupt_handler(uint8_t n, isr_t handler);

#endif
