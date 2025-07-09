#ifndef APIC_H
#define APIC_H

#include <stdint.h>

/* Register offsets */
#define APIC_BASE 0xFEE00000

#define APIC_ID 0x020
#define APIC_VERSION 0x030
#define APIC_EOI 0x0B0
#define APIC_SPURIOUS 0x0F0
#define APIC_LVT_TIMER 0x320
#define APIC_ICR_LOW 0x300
#define APIC_ICR_HIGH 0x310

/* Helper macro */
#define APIC_REG(offset) (*(volatile uint32_t)(APIC_BASE + offset))

uint32_t read_msr_low(uint32_t offset) {

}

uint32_t read_msr_high(uint32_t offset) {

}

#endif
