#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <interrupts/isr.h>

/* polls condition until true or ms elapses. returns 1 if condition met, 0 if timed out */
#define timeout(ms, condition)                                    \
	({                                                        \
		uint32_t _start = timer_ticks();                  \
		while (!(condition) &&                            \
		       (timer_ticks() - _start) < (uint32_t)(ms)) \
			;                                         \
		(condition);                                      \
	})

#define TIMER_INT_NO 32

void init_timer(void);
uint32_t timer_ticks(void);
void sleep(uint32_t s);
void sleep_ms(uint32_t ms);

/* ISRs */
void timer_interrupt_handler(struct interrupt_context *regs);
void timer_interrupt_handler_sched(struct interrupt_context *regs);

#endif
