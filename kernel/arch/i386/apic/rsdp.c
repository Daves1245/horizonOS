#include <stdint.h>
#include "rsdp.h"
#include "madt.h"
#include "../../../libc/string/string.h"
#include <stdio.h>

uint32_t *find_rsdp() {
    uint32_t *rsdp_addr;

    // EBDA (Extended BIOS Data Area)
    uint16_t *ebda_ptr = (uint16_t *) 0x40E;
    uint32_t ebda_addr = (*ebda_ptr) << 4;

    /* We need to do the casting to uint8_t and add 16 for proper pointer arithmetic */
    /* Note: (uint32_t *) + 16 = (char *)(uint32_t *) + 16 * sizeof(uint32_t) */

    // first 1KB of EBDA on 16-byte boundaries
    for (rsdp_addr = (uint32_t *) ebda_addr;
            rsdp_addr < (uint32_t *) (ebda_addr + 1024);
            rsdp_addr = (uint32_t *) ((uint8_t *) rsdp_addr + 16)) {
        if (strncmp((char *) rsdp_addr, "RSD PTR ", 8) == 0) {
            if (validate_rsdp_checksum(rsdp_addr) == 0) {
                return rsdp_addr;
            }
        }
    }

    // if not found in EBDA, search BIOS ROM area on 16-byte boundaries
    for (rsdp_addr = (uint32_t *) 0x000E0000;
            rsdp_addr < (uint32_t *) 0x00100000;
            rsdp_addr = (uint32_t *) ((uint8_t *) rsdp_addr + 16)) {
        if (strncmp((char *) rsdp_addr, "RSD PTR ", 8) == 0) {
            if (validate_rsdp_checksum(rsdp_addr) == 0) {
                return rsdp_addr;
            }
        }
    }

    return (void *) 0;
}

uint32_t validate_rsdp_checksum(uint32_t *rsdp_addr) {
    if (!rsdp_addr) {
        return 1; // null pointer error
    }

    struct rsdp_t *rsdp = (struct rsdp_t *) rsdp_addr;
    uint8_t *bytes = (uint8_t *) rsdp_addr;
    uint8_t checksum = 0;

    // calculate checksum for first 20 bytes (RSDP v1.0)
    for (uint32_t i = 0; i < 20; i++) {
        checksum += bytes[i];
    }

    if (checksum != 0) {
        return 2; // checksum validation failed
    }

    // if RSDP revision is 2.0 or higher, validate extended checksum
    if (rsdp->revision >= 2) {
        struct xsdp_t *xsdp = (struct xsdp_t *) rsdp_addr;
        uint8_t extended_checksum = 0;

        // calculate checksum for entire structure
        for (uint32_t i = 0; i < xsdp->length; i++) {
            extended_checksum += bytes[i];
        }

        if (extended_checksum != 0) {
            return 3; // extended checksum validation failed
        }
    }

    return 0; // validation passed
}

uint32_t *find_madt(uint32_t *rsdp_addr) {
    if (!rsdp_addr) {
        return (void *) 0;
    }

    struct rsdp_t *rsdp = (struct rsdp_t *) rsdp_addr;

    // determine if we use RSDT (v1.0) or XSDT (v2.0+)
    if (rsdp->revision >= 2) {
        // use XSDT for ACPI v2.0+
        struct xsdp_t *xsdp = (struct xsdp_t *) rsdp_addr;
        struct xsdt_t *xsdt = (struct xsdt_t *) (uint32_t) xsdp->xsdt_addr;

        // calculate number of entries
        uint32_t num_entries = (xsdt->header.length - sizeof(struct apic_header)) / 8;

        // search for MADT ("APIC" signature)
        for (uint32_t i = 0; i < num_entries; i++) {
            struct apic_header *table = (struct apic_header *) (uint32_t) xsdt->entry_ptrs[i];

            if (strncmp(table->signature, "APIC", 4) == 0) {
                return (uint32_t *) table;
            }
        }
    } else {
        // use RSDT for ACPI v1.0
        struct rsdt_t *rsdt = (struct rsdt_t *) rsdp->rsdt_addr;

        // calculate number of entries
        uint32_t num_entries = (rsdt->header.length - sizeof(struct apic_header)) / 4;

        // search for MADT ("APIC" signature)
        for (uint32_t i = 0; i < num_entries; i++) {
            struct apic_header *table = (struct apic_header *) rsdt->entry_ptrs[i];

            if (strncmp(table->signature, "APIC", 4) == 0) {
                return (uint32_t *) table;
            }
        }
    }

    return (void *) 0; // MADT not found
}

void initialize_apic() {
    printf("starting ACPI/APIC initialization...\n");

    // find RSDP
    printf("searching for RSDP...\n");
    uint32_t *rsdp = find_rsdp();
    if (!rsdp) {
        printf("ERROR: RSDP not found!\n");
        return;
    }
    printf("RSDP found at address: 0x%x\n", (uint32_t) rsdp);

    // validate RSDP checksum
    printf("validating RSDP checksum...\n");
    uint32_t rsdp_result = validate_rsdp_checksum(rsdp);
    if (rsdp_result != 0) {
        printf("ERROR: RSDP checksum validation failed (code: %d)\n", rsdp_result);
        return;
    }
    printf("RSDP checksum valid!\n");

    // find madt
    printf("searching for MADT table...\n");
    uint32_t *madt = find_madt(rsdp);
    if (!madt) {
        printf("ERROR: MADT not found!\n");
        return;
    }
    printf("madt found at address: 0x%x\n", (uint32_t) madt);

    // parse madt
    printf("parsing MADT table...\n");
    parse_madt(madt);
    printf("MADT parsing complete!\n");

    // display results
    printf("I/O APIC Address: 0x%x\n", get_ioapic_address());
    printf("I/O APIC ID: %d\n", get_ioapic_id());
    printf("Keyboard IRQ: %d (flags: 0x%x)\n", get_keyboard_global_irq(), get_keyboard_irq_flags());

    printf("ACPI/APIC initialization complete!\n");
}
