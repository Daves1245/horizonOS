#ifndef APIC_H
#define APIC_H

#include <stdint.h>

/* register offsets */
#define APIC_BASE 0xFEE00000

#define APIC_ID 0x020
#define APIC_VERSION 0x030
#define APIC_EOI 0x0B0
#define APIC_SPURIOUS 0x0F0
#define APIC_LVT_TIMER 0x320
#define APIC_ICR_LOW 0x300
#define APIC_ICR_HIGH 0x310

/* helper macro */
#define APIC_REG(offset) (*(volatile uint32_t)(APIC_BASE + offset))

static inline uint32_t read_msr_low(uint32_t msr) {
    uint32_t low;
    asm volatile("rdmsr" : "=a"(low) : "c"(msr) : "edx");
    return low;
}

static inline uint32_t read_msr_high(uint32_t msr) {
    uint32_t high;
    asm volatile("rdmsr" : "=d"(high) : "c"(msr) : "eax");
    return high;
}

int check_msr();

static inline void write_msr(uint32_t msr, uint32_t low, uint32_t high) {
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline void write_msr_low(uint32_t msr, uint32_t low) {
    uint32_t high = read_msr_high(msr);
    write_msr(msr, low, high);
}

// there are two registers for ioapic - address register and
// data register at APIC_BASE and APIC_BASE + 4 bytes respectively
// address register uses the bottom byte for register select
void enable_api_hardware();
void enable_apic_software();
uint32_t ioapic_read(void *ioapic_base, uint32_t reg);
void ioapic_write(void *ioapic_base, uint32_t reg, uint32_t value);
uint32_t get_ioapic_base();
uint8_t get_local_apic_id();
void configure_ioapic_irq(void *ioapic_base, uint8_t irq, uint8_t vector, uint8_t dest_apic_id);
void configure_ioapic_irq_with_flags(void *ioapic_base, uint8_t irq, uint8_t vector, uint8_t dest_apic_id, uint16_t flags);
void disable_pic();

// abstractino for architecture-independent implementation
void apic_send_eoi(void);

#endif
