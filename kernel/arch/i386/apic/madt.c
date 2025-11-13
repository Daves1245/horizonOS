#include "../memory/paging.h"
#include "madt.h"
#include <stdio.h>

// store found I/O APIC info
static uint32_t ioapic_address = 0;
static uint32_t ioapic_irq_base = 0;
static uint8_t ioapic_id = 0;

// store IRQ overrides
static struct {
    uint8_t source_irq;
    uint32_t global_irq;
    uint16_t flags;
} irq_overrides[16];
static int num_overrides = 0;

void parse_madt(void *madt_addr) {
    // ensure the madt is mapped
    map_physical_range((uint32_t)madt_addr,  0x4000, 1, 1);

    struct madt_extended_header *header = (struct madt_extended_header *) madt_addr;
    uint32_t length = header->header.length;

    uint8_t *entry = (uint8_t *) madt_addr + sizeof(struct madt_extended_header);
    uint8_t *end = (uint8_t *) madt_addr + length;

    while (entry < end) {
        struct madt_record_header *entry_header = (struct madt_record_header *) entry;
        uint8_t entry_length = entry_header->record_lenght;

        switch (entry_header->entry_type) {
            case 0: { // processor Local APIC
                struct processor_local_apic_record *local_apic =
                    (struct processor_local_apic_record *)(entry + sizeof(struct madt_record_header));
                printf("[madt::parse_madt]: Local APIC: Processor ID=%d, APIC ID=%d,\nFlags=0x%x\n",
                       local_apic->acpi_processor_id, local_apic->apic_id, local_apic->flags);
                break;
            }
            case 1: { // I/O APIC
                struct ioapic *io_apic =
                    (struct ioapic *)(entry + sizeof(struct madt_record_header));
                printf("[madt::parse_madt]: I/O APIC: ID=%d, Address=0x%x, IRQ Base=%d\n",
                       io_apic->ioapic_id, io_apic->io_apic_address, io_apic->global_system_interrupt_base);

                // store the first I/O APIC we find (usually covers IRQ 0-23)
                if (ioapic_address == 0) {
                    ioapic_address = io_apic->io_apic_address;
                    ioapic_irq_base = io_apic->global_system_interrupt_base;
                    ioapic_id = io_apic->ioapic_id;
                }
                break;
            }
            case 2: { // Interrupt Source Override
                struct ioapic_interrupt_source_override *override =
                    (struct ioapic_interrupt_source_override *)(entry + sizeof(struct madt_record_header));
                uint16_t flags = *(uint16_t*)override->flags;
                printf("[madt::parse_madt]: IRQ Override: Bus=%d, Source IRQ=%d -> Global IRQ=%d,\nFlags=0x%x\n",
                       override->bus_source, override->irq_source,
                       override->global_system_interrupt, flags);

                // store override info for later use
                if (num_overrides < 16) {
                    irq_overrides[num_overrides].source_irq = override->irq_source;
                    irq_overrides[num_overrides].global_irq = override->global_system_interrupt;
                    irq_overrides[num_overrides].flags = flags;
                    num_overrides++;
                }
                break;
            }
            case 3: { // I/O APIC Non-maskable interrupt source
                struct ioapic_nonmaskable_interrupt_source *nmi =
                    (struct ioapic_nonmaskable_interrupt_source *)(entry + sizeof(struct madt_record_header));
                printf("[madt::parse_madt]: I/O APIC NMI: Source=%d, Global IRQ=%d\n",
                       nmi->nmi_source, nmi->global_system_interrupt);
                break;
            }
            case 4: { // local APIC Non-maskable interrupts
                struct local_apic_nonmaskable_interrupts *local_nmi =
                    (struct local_apic_nonmaskable_interrupts *)(entry + sizeof(struct madt_record_header));
                printf("[madt::parse_madt]: Local APIC NMI: Processor ID=%d, LINT=%d\n",
                       local_nmi->acpi_processor_id, local_nmi->lint_number);
                break;
            }
            case 5: { // local APIC Address Override
                struct local_apic_address_override *addr_override =
                    (struct local_apic_address_override *)(entry + sizeof(struct madt_record_header));
                printf("[madt::parse_madt]: Local APIC Address Override: 0x%x%x\n",
                       addr_override->dqword_local_apic_physical_address[1],
                       addr_override->dqword_local_apic_physical_address[0]);
                break;
            }
            case 9: { // processor Local x2APIC
                struct processor_local_x2apic *x2apic =
                    (struct processor_local_x2apic *)(entry + sizeof(struct madt_record_header));
                printf("[madt::parse_madt]: Local x2APIC: ID=0x%x, Flags=0x%x, ACPI ID=%d\n",
                       x2apic->processor_local_x2apic_id, x2apic->flags, x2apic->acpi_id);
                break;
            }
            default:
                printf("[madt::parse_madt]: Unknown MADT entry type: %d\n", entry_header->entry_type);
                break;
        }

        entry += entry_length;
    }
}

// access parsed MADT data
uint32_t get_ioapic_address(void) {
    return ioapic_address;
}

uint8_t get_ioapic_id(void) {
    return ioapic_id;
}

uint32_t get_keyboard_global_irq(void) {
    // check if IRQ 1 (keyboard) has an override
    for (int i = 0; i < num_overrides; i++) {
        if (irq_overrides[i].source_irq == 1) {
            return irq_overrides[i].global_irq;
        }
    }
    // use default IRQ 1 if no override is found
    return 1;
}

uint16_t get_keyboard_irq_flags(void) {
    // check for IRQ 1 (keyboard) has an override with specific flags
    for (int i = 0; i < num_overrides; i++) {
        if (irq_overrides[i].source_irq == 1) {
            return irq_overrides[i].flags;
        }
    }
    // none found, use defaults: active high, edge triggered
    return 0;
}
