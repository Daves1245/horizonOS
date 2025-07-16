#ifndef APIC_H
#define APIC_H

#include <stdint.h>

#define APIC_BASE 0xFEE00000

// MSR functions for x86_64
static inline uint64_t read_msr(uint32_t msr) {
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void write_msr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

// Legacy 32-bit functions for compatibility
static inline uint32_t read_msr_low(uint32_t msr) {
    return (uint32_t)read_msr(msr);
}

static inline void write_msr_low(uint32_t msr, uint32_t value) {
    write_msr(msr, value);
}

// APIC functions
void enable_apic_hardware(void);
void enable_apic_software(void);

#endif
