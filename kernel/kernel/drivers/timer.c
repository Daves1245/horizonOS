#include <apic/apic.h>
#include <interrupts/isr.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>

static uint32_t tick_count = 0;

void timer_interrupt_handler(struct interrupt_context *regs) {
    tick_count++;

    // print every 10000 ticks to avoid spam
    if (tick_count % 10000 == 0) {
        printf("[timer]: tick %d\n", tick_count);
    }

    // send APIC EOI (architecture-independent)
    apic_send_eoi();
}

uint32_t timer_ticks() {
    return tick_count;
}

void init_timer() {
    log_debug("[timer]: registering timer handler for vector 32\n");
    register_interrupt_handler(32, timer_interrupt_handler);
    log_success("[timer]: timer handler registered\n");
}
