#include <apic/apic.h>
#include <interrupts/isr.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>

#define TIMER_HZ 1000

static volatile uint32_t tick_count = 0;
static volatile uint32_t uptime_ms = 0;

uint32_t timer_ticks() {
    return tick_count;
}

// timer hz is set at 1000 so it already fires once every ms
// for now, just return tick_count
uint32_t get_uptime_ms() {
    return tick_count;
}

void timer_interrupt_handler(struct interrupt_context *regs) {
    tick_count++;
    apic_send_eoi();
}

void init_timer() {
    log_debug("[timer]: registering timer handler for vector 32\n");
    register_interrupt_handler(32, timer_interrupt_handler);
    apic_timer_init(TIMER_HZ);
    log_success("[timer]: APIC timer initialized at %d Hz\n", TIMER_HZ);
}

void sleep_ms(uint32_t ms) {
    uint32_t start = tick_count;
    while ((tick_count - start) < ms);
}

void sleep(uint32_t s) {
    sleep_ms(s * 1000);
}
