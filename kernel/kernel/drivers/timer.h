#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* polls condition until true or ms elapses. returns 1 if condition met, 0 if timed out */
#define timeout(ms, condition) ({                         \
    uint32_t _start = timer_ticks();                      \
    while (!(condition) &&                                \
           (timer_ticks() - _start) < (uint32_t)(ms));   \
    (condition);                                          \
})

void init_timer();
uint32_t timer_ticks();
void sleep(uint32_t s);
void sleep_ms(uint32_t ms);

#endif
