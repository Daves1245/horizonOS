#include "apic.h"
#include <stdint.h>

// To enable the APIC, we have to set both hardware enable and software enable
// (intel64 IA32 manual)

// APIC registers can wbe found from 0xFEE00000 - 0xFEE00FFF

// According to the manual (for specific processors) - apic physical addresses
// have to be marked as uncacheable to work correctly. also, apic overlaps with irq0-9,
// so these must be mapped to other ivt entries

// TODO we have to enable paging before we can enable the APIC
// (not technically - it's possible to directly access the physical address, but it's
// better practice)

// MSR 0x1B (APIC base MSR)
void enable_api_hardware() {
    uint32_t apic_base = read_msr_low(0x1B);
    apic_base |= (1 << 11); // global (hardware) apic enable
    write_msr_low(0x1B, apic_base);
}

// spurious vector register (APIC 0xF0)
void enable_apic_software() {
    volatile uint32_t *spurious_reg = (uint32_t *)(APIC_BASE + 0xF0);
    *spurious_reg |= (1 << 8); // apic software enable
    *spurious_reg |= 0xFF;
}

// use of volatile is necessary to ensure optimizations don't optimize away memory
// accesses or reorder operations
uint32_t ioapic_read(void *ioapic_base, uint32_t reg) {
    uint32_t volatile *ioapic = (uint32_t volatile *) ioapic_base;
    ioapic[0] = (reg & 0xff);
    return ioapic[4];
}

// same thing about volatile
void ioapic_write(void *ioapic_base, uint32_t reg, uint32_t value) {
    uint32_t volatile *ioapic = (uint32_t volatile *) ioapic_base;
    ioapic[0] = (reg & 0xff);
    ioapic[4] = value;
}

// grab the base address of the ioapic
uint32_t get_ioapic_base() {
    uint32_t low = read_msr_low(0x1B);
    return low & 0xfffff000; // mask the lower 12 bits
}

// Configure IOAPIC pin to route to specific vector
void configure_ioapic_irq(void *ioapic_base, uint8_t irq, uint8_t vector, uint8_t dest_apic_id) {
    // IOAPIC redirection table entry is 64 bits (2 registers)
    uint32_t low_reg = 0x10 + (irq * 2);     // Low 32 bits register
    uint32_t high_reg = 0x10 + (irq * 2) + 1; // High 32 bits register
    
    // Configure low 32 bits: vector + delivery mode (fixed) + edge triggered + not masked
    uint32_t low_val = vector;  // Vector in bits 0-7, rest defaults to 0 (fixed delivery, edge, not masked)
    
    // Configure high 32 bits: destination APIC ID in bits 56-63 (upper 8 bits of high register)
    uint32_t high_val = (uint32_t)dest_apic_id << 24;
    
    printf("[ioapic]: Configuring IRQ %d -> vector %d, dest APIC %d\n", irq, vector, dest_apic_id);
    ioapic_write(ioapic_base, high_reg, high_val);
    ioapic_write(ioapic_base, low_reg, low_val);
    printf("[ioapic]: IRQ %d configured\n", irq);
}
