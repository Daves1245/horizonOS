#ifndef MADT_H
#define MADT_H

/*
 * Headers for parsing the MADT.
 * See https://osdev.wiki/wiki/MADT#Table_Structure
 *
 */

#include <stdint.h>

struct apic_header {
    char signature[4]; // always "APIC"
    uint32_t length;
    char revision;
    char checksum;
    char omeid[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

struct apic_addr_and_flags {
    uint32_t apic_address;
    uint32_t flags; // 1 = dual 8259 Legacy PICs installed
} __attribute__((packed));

struct madt_extended_header {
    struct apic_header header;
    struct apic_addr_and_flags addr_flags;
} __attribute__((packed));

struct madt_record_header {
    char entry_type;
    char record_lenght;
} __attribute__((packed));

// entry type 0: processor local APIC
struct processor_local_apic_record {
    char acpi_processor_id;
    char apic_id;
    uint32_t flags; // 0 = processor enabled. 1 = online capable
} __attribute__((packed));

// entry type 1: I/O APIC
struct ioapic {
    char ioapic_id;
    char reserved; // unused
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed));

// entry type 2: I/O APIC Interrupt Source Override
struct ioapic_interrupt_source_override {
    char bus_source;
    char irq_source;
    uint32_t global_system_interrupt;
    char flags[2];
} __attribute__((packed));

// entry type 3: I/O APIC Non-maskable interrupt source
struct ioapic_nonmaskable_interrupt_source {
    char nmi_source;
    char reserved; // unused
    char flags[2];
    uint32_t global_system_interrupt;
} __attribute__((packed));

// entry type 4: local APIC Non-maskable interrupts
struct local_apic_nonmaskable_interrupts {
    char acpi_processor_id; // 0xFF -> all processors
    char flags[2];
    char lint_number; // 0 or 1
} __attribute__((packed));
// entry type 5: local apic address override
struct local_apic_address_override {
    char reserved[2];
    uint32_t dqword_local_apic_physical_address[2];
} __attribute__((packed));

// entry type 9: processor local x2APIC
struct processor_local_x2apic {
    char reserved[2];
    uint32_t processor_local_x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed));

/*
entry type 2, 3 and 4 have a flags field, which is useful for settings up the I/O APIC redirection entry or local vector table entry respectively. if (flags & 2) then the interrupt is active when low, and if (flags & 8) then interrupt is level-triggered.
these describe the trigger mode of an interrupt. if an interrupt is activated when
the signal goes low->high, it's high active. low active is when voltage goes high->low.
edge triggered is when interrupts are fired on the transition of a signal (either way, low to high or vice versa). level triggered are fired when a signal maintains
a specific voltage level. in the case of level triggers, they must be explicitly reset
or cleared, otherwise the interrupt will keep firing. these are implementation
specific for each IRQ, and we need to know this in order to set up the IOAPIC to be
configured properly
*/

void parse_madt(void *madt_addr);
uint32_t get_ioapic_address(void);
uint8_t get_ioapic_id(void);
uint32_t get_keyboard_global_irq(void);
uint16_t get_keyboard_irq_flags(void);

#endif
