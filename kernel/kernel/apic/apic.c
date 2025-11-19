#include <apic/apic.h>
#include <drivers/io.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>

int check_msr() {
    uint32_t eax, ebx, ecx, edx;
    asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    return (edx & (1 << 5)) != 0;
}

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

// disable legacy PIC - required when using APIC
void disable_pic() {
    log_debug("Disabling legacy PIC\n");
    // mask all interrupts on both PICs
    outb(0x21, 0xFF);  // master PIC
    outb(0xA1, 0xFF);  // slave PIC
    log_success("Legacy PIC disabled\n");
}

// get Local APIC ID from the APIC ID register
uint8_t get_local_apic_id() {
    volatile uint32_t *apic_id_reg = (volatile uint32_t *)(APIC_BASE + APIC_ID);
    // APIC ID is in bits 24-31 of the APIC ID register
    return (*apic_id_reg >> 24) & 0xFF;
}

// configure IOAPIC pin to route to specific vector with flags
void configure_ioapic_irq_with_flags(void *ioapic_base, uint8_t irq, uint8_t vector, uint8_t dest_apic_id, uint16_t flags) {
    // IOAPIC redirection table entry is 64 bits (2 registers)
    uint32_t low_reg = 0x10 + (irq * 2);     // low 32 bits register
    uint32_t high_reg = 0x10 + (irq * 2) + 1; // high 32 bits register

    // start with vector in bits 0-7
    uint32_t low_val = vector;

    // delivery mode: fixed (000) in bits 8-10 (already 0)
    // destination mode: physical (0) in bit 11 (already 0)

    // polarity: bit 13
    // flags & 2 means active low
    if (flags & 0x02) {
        low_val |= (1 << 13);  // active low
    }
    // otherwise active high (bit 13 = 0, already set)

    // trigger mode: bit 15
    // flags & 8 means level triggered
    if (flags & 0x08) {
        low_val |= (1 << 15);  // level triggered
    }
    // otherwise edge triggered (bit 15 = 0, already set)

    // mask bit: bit 16 - unmask the interrupt (0 = not masked)
    // low_val |= (0 << 16);  // not masked (already 0)

    // configure high 32 bits: destination APIC ID in bits 56-63 (upper 8 bits of high register)
    uint32_t high_val = (uint32_t)dest_apic_id << 24;

    log_debug("Configuring IOAPIC: ");
    printf("IRQ %d -> vector %d (dest=%d, flags=0x%x)\n", irq, vector, dest_apic_id, flags);

    ioapic_write(ioapic_base, high_reg, high_val);
    ioapic_write(ioapic_base, low_reg, low_val);

    log_success("IOAPIC IRQ configured: ");
    printf("%d\n", irq);
}

// configure IOAPIC pin to route to specific vector (legacy wrapper)
void configure_ioapic_irq(void *ioapic_base, uint8_t irq, uint8_t vector, uint8_t dest_apic_id) {
    // default flags: active high, edge triggered
    configure_ioapic_irq_with_flags(ioapic_base, irq, vector, dest_apic_id, 0);
}

// Send End of Interrupt (EOI) to Local APIC
// Works on both i386 (xAPIC) and x86_64 (will use x2APIC there)
void apic_send_eoi(void) {
#ifdef __x86_64__
    // x2APIC: Write to MSR 0x80B
    write_msr(0x80B, 0, 0);
#else
    // xAPIC: Write to memory-mapped EOI register
    volatile uint32_t *apic_eoi = (volatile uint32_t *)(APIC_BASE + APIC_EOI);
    *apic_eoi = 0;
#endif
}
