#include "../include/apic.h"

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
