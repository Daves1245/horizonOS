#include <stdint.h>
#include "descriptor_tables.h"
#include "isr.h"

static isr_t interrupt_handlers[256] = {0};

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(struct interrupt_context *regs) {
    // dispatch to registered handler if exists
    if (interrupt_handlers[regs->int_no] != 0) {
        interrupt_handlers[regs->int_no](regs);
    }
}

void* isr_stub_table[32] = {0};
