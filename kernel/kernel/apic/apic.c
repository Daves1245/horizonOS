#include <apic/apic.h>
#include <drivers/io.h>
#include <stdint.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/logger.h>
#include <mm.h>

uintptr_t lapic_virt_base = APIC_BASE;

extern volatile uint32_t *ioapic_addr;

void apic_set_base(uintptr_t virt) {
    lapic_virt_base = virt;
}

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
    volatile uint32_t *spurious_reg = (uint32_t *)(lapic_virt_base + 0xF0);
    *spurious_reg |= (1 << 8); // apic software enable
    *spurious_reg |= 0xFF;
}

// use of volatile is necessary to ensure optimizations don't optimize away memory
// accesses or reorder operations
uint32_t ioapic_read(uint32_t reg) {
    ioapic_addr[0] = (reg & 0xff);
    return ioapic_addr[4];
}

// same thing about volatile
void ioapic_write(uint32_t reg, uint32_t value) {
    ioapic_addr[0] = (reg & 0xff);
    ioapic_addr[4] = value;
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
    volatile uint32_t *apic_id_reg = (volatile uint32_t *)(lapic_virt_base + APIC_ID);
    // APIC ID is in bits 24-31 of the APIC ID register
    return (*apic_id_reg >> 24) & 0xFF;
}

// configure IOAPIC pin to route to specific vector with flags
void configure_ioapic_irq_with_flags(uint8_t irq, uint8_t vector, uint8_t dest_apic_id, uint16_t flags) {
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

    log_debug("configuring IOAPIC: IRQ %d -> vector %d (dest=%d, flags=0x%x)\n", irq, vector, dest_apic_id, flags);

    ioapic_write(high_reg, high_val);
    ioapic_write(low_reg, low_val);

    log_success("IOAPIC IRQ configured: %d\n", irq);
}

// configure IOAPIC pin to route to specific vector (legacy wrapper)
void configure_ioapic_irq(uint8_t irq, uint8_t vector, uint8_t dest_apic_id) {
    // default flags: active high, edge triggered
    configure_ioapic_irq_with_flags(irq, vector, dest_apic_id, 0);
}

// Send End of Interrupt (EOI) to Local APIC
// Works on both i386 (xAPIC) and x86_64 (will use x2APIC there)
void apic_send_eoi(void) {
    volatile uint32_t *apic_eoi = (volatile uint32_t *)(lapic_virt_base + APIC_EOI);
    *apic_eoi = 0;
}

#define PIT_FREQ 1193182

/* use PIT channel 2 (no IRQ) to spin-wait for a given number of milliseconds */
static void pit_wait_ms(uint32_t ms) {
    uint32_t count = (PIT_FREQ * ms) / 1000;

    /* gate channel 2 on, speaker off */
    uint8_t tmp = inb(0x61);
    outb(0x61, (tmp & ~0x02) | 0x01);

    /* channel 2, lobyte/hibyte, mode 0 (one-shot), binary */
    outb(0x43, 0xB0);
    outb(0x42, count & 0xFF);
    outb(0x42, (count >> 8) & 0xFF);

    /* wait for output bit (bit 5 of port 0x61) to go high */
    while (!(inb(0x61) & 0x20));
}

void apic_timer_init(uint32_t hz) {
    volatile uint32_t *divide   = (volatile uint32_t *)(lapic_virt_base + APIC_TIMER_DIVIDE);
    volatile uint32_t *initcnt  = (volatile uint32_t *)(lapic_virt_base + APIC_TIMER_INITCNT);
    volatile uint32_t *currcnt  = (volatile uint32_t *)(lapic_virt_base + APIC_TIMER_CURRCNT);
    volatile uint32_t *lvt      = (volatile uint32_t *)(lapic_virt_base + APIC_LVT_TIMER);

    /* divide by 1 */
    *divide = 0x0B;

    /* mask + one-shot mode for calibration, vector 32 */
    *lvt = APIC_TIMER_MASKED | APIC_TIMER_ONESHOT | 32;
    *initcnt = 0xFFFFFFFF;

    pit_wait_ms(10);

    uint32_t ticks_per_10ms = 0xFFFFFFFF - *currcnt;
    uint32_t ticks_per_tick = (ticks_per_10ms / 10) * (1000 / hz);

    log_debug("[apic]: timer calibrated: %d ticks per ms\n", ticks_per_10ms / 10);

    /* set up periodic timer at hz */
    *divide  = 0x0B;
    *lvt     = APIC_TIMER_PERIODIC | 32;
    *initcnt = ticks_per_tick;
}
