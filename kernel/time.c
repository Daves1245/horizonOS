#include <apic/apic.h>
#include <interrupts/isr.h>
#include <kernel/panic.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <time.h>

#ifdef __x86_64__
#include <kernel/scheduler/process.h>
#include <kernel/scheduler/scheduler.h>
#endif

#define TIMER_HZ 1000

static volatile uint32_t tick_count;
static volatile uint32_t uptime_ms;

uint32_t timer_ticks(void) {
	return tick_count;
}

// timer hz is set at 1000 so it already fires once every ms
// for now, just return tick_count
uint32_t get_uptime_ms(void) {
	return tick_count;
}

void timer_interrupt_handler(struct interrupt_context *regs) {
	tick_count++;
	apic_send_eoi();
}

#ifdef __x86_64__
void timer_interrupt_handler_sched(struct interrupt_context *regs) {
	tick_count++;
	apic_send_eoi();
	if (mycpu() == NULL) {
		panic("[timer]: mycpu() returned NULL");
	}
	ctx_switch(mycpu()->scheduler_proc);
}
#endif

void init_timer(void) {
	log_debug("[timer]: registering timer handler for vector 32\n");
	register_interrupt_handler(TIMER_INT_NO, timer_interrupt_handler);
	apic_timer_init(TIMER_HZ);
	log_success("[timer]: APIC timer initialized at %d Hz\n", TIMER_HZ);
}

void sleep_ms(uint32_t ms) {
	uint32_t start = tick_count;

  // TODO spin or yield instead of busy waiting
	while ((tick_count - start) < ms)
		;
}

void sleep(uint32_t s) {
	sleep_ms(s * 1000);
}
