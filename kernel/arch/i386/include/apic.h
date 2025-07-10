#ifndef APIC_H
#define APIC_H

#include <stdint.h>

#define APIC_BASE 0xFEE00000

// MSR functions
static inline uint32_t read_msr_low(uint32_t msr) {
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return low;
}

static inline void write_msr_low(uint32_t msr, uint32_t value) {
    uint32_t high = 0;
    asm volatile("wrmsr" : : "c"(msr), "a"(value), "d"(high));
}

// APIC functions
void enable_api_hardware(void);
void enable_apic_software(void);

#endif
