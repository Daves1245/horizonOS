#include <stdint.h>
#include "rsdp.h"
#include "madt.h"
#include <string.h>
#include <stdio.h>
#include <kernel/logger.h>

// deliberately keep separate to emphasize x86_64 should not be handling paging
#ifdef __i386__
#include <i386/memory/paging.h>
#else
#include <x86_64/memory/paging.h>
#endif

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
    log_debug("[rsdp::find_madt]: RSDP revision: %d\n", rsdp->revision);

    // determine if we use RSDT (v1.0) or XSDT (v2.0+)
    if (rsdp->revision >= 2) {
        // use XSDT for ACPI v2.0+
        struct xsdp_t *xsdp = (struct xsdp_t *) rsdp_addr;
        log_debug("[rsdp::find_madt]: XSDT address: 0x%x%x\n", (uint32_t)(xsdp->xsdt_addr >> 32), (uint32_t)xsdp->xsdt_addr);

        // check if XSDT address is reasonable (< 4GB for 32-bit system)
        if (xsdp->xsdt_addr >= 0x100000000ULL) {
            log_error("ERROR: XSDT address too high for 32-bit system\n");
            return (void *) 0;
        }

        struct xsdt_t *xsdt = (struct xsdt_t *) (uint32_t) xsdp->xsdt_addr;
        log_debug("[rsdp::find_madt]: Accessing XSDT at: 0x%x\n", (uint32_t) xsdt);

        // calculate number of entries
        uint32_t num_entries = (xsdt->header.length - sizeof(struct apic_header)) / 8;
        log_debug("[rsdp::find_madt]: XSDT entries: %d\n", num_entries);

        // search for MADT ("APIC" signature)
        for (uint32_t i = 0; i < num_entries; i++) {
            uint32_t table_addr = (uint32_t) xsdt->entry_ptrs[i];
            log_debug("[rsdp::find_madt]: Checking table %d at address: 0x%x\n", i, table_addr);

            // safety check for table address
            if (table_addr < 0x1000 || table_addr >= 0x40000000) {
                log_debug("[rsdp::find_madt]: WARNING: Skipping invalid table address: 0x%x\n", table_addr);
                continue;
            }

            // Map the table page before accessing it
            map_physical_range(table_addr, 4096, 1, 1); // kernel, writable

            struct apic_header *table = (struct apic_header *) table_addr;

            if (strncmp(table->signature, "APIC", 4) == 0) {
                log_debug("[rsdp::find_madt]: Found APIC table!\n");
                return (uint32_t *) table;
            }
        }
    } else {
        // use RSDT for ACPI v1.0
        log_debug("[rsdp::find_madt]: RSDT address: 0x%x\n", rsdp->rsdt_addr);

        // safety check for RSDT address
        if (rsdp->rsdt_addr < 0x1000 || rsdp->rsdt_addr >= 0x40000000) {
            log_error("ERROR: Invalid RSDT address: 0x%x\n", rsdp->rsdt_addr);
            return (void *) 0;
        }

        // Map the RSDT page before accessing it
        log_debug("[rsdp::find_madt]: Mapping RSDT at: 0x%x\n", rsdp->rsdt_addr);
        map_physical_range(rsdp->rsdt_addr, 4096, 1, 1); // kernel, writable

        struct rsdt_t *rsdt = (struct rsdt_t *) rsdp->rsdt_addr;
        log_debug("[rsdp::find_madt]: Accessing RSDT at: 0x%x\n", (uint32_t) rsdt);

        // calculate number of entries
        uint32_t num_entries = (rsdt->header.length - sizeof(struct apic_header)) / 4;
        log_debug("[rsdp::find_madt]: RSDT entries: %d\n", num_entries);

        // search for MADT ("APIC" signature)
        for (uint32_t i = 0; i < num_entries; i++) {
            uint32_t table_addr = rsdt->entry_ptrs[i];
            log_debug("[rsdp::find_madt]: Checking table %d at address: 0x%x\n", i, table_addr);

            // safety check for table address
            if (table_addr < 0x1000 || table_addr >= 0x40000000) {
                log_debug("[rsdp::find_madt]: WARNING: Skipping invalid table address: 0x%x\n", table_addr);
                continue;
            }

            // Map the table page before accessing it
            map_physical_range(table_addr, 4096, 1, 1); // kernel, writable

            struct apic_header *table = (struct apic_header *) table_addr;

            if (strncmp(table->signature, "APIC", 4) == 0) {
                log_debug("[rsdp::find_madt]: Found APIC table!\n");
                return (uint32_t *) table;
            }
        }
    }

    return (void *) 0; // MADT not found
}

void initialize_apic() {
    log_debug("[rsdp::init_apic]: starting ACPI/APIC initialization...\n");

    // find RSDP
    log_debug("[rsdp::init_apic]: searching for RSDP...\n");
    uint32_t *rsdp = find_rsdp();
    if (!rsdp) {
        log_error("ERROR: RSDP not found!\n");
        return;
    }
    log_debug("[rsdp::init_apic]: RSDP found at address: 0x%x\n", (uint32_t) rsdp);

    // validate RSDP checksum
    log_debug("[rsdp::init_apic]: validating RSDP checksum...\n");
    uint32_t rsdp_result = validate_rsdp_checksum(rsdp);
    if (rsdp_result != 0) {
        log_error("ERROR: RSDP checksum validation failed (code: %d)\n", rsdp_result);
        return;
    }
    log_debug("[rsdp::init_apic]: RSDP checksum valid!\n");

    // find madt
    log_debug("[rsdp::init_apic]: searching for MADT table...\n");
    uint32_t *madt = find_madt(rsdp);
    if (!madt) {
        log_error("ERROR: MADT not found!\n");
        return;
    }
    log_debug("[rsdp::init_apic]: madt found at address: 0x%x\n", (uint32_t) madt);

    // parse madt
    log_debug("[rsdp::init_apic]: parsing MADT table...\n");
    parse_madt(madt);
    log_debug("[rsdp::init_apic]: MADT parsing complete!\n");

    // display results
    log_debug("[rsdp::init_apic]: I/O APIC Address: 0x%x\n", get_ioapic_address());
    log_debug("[rsdp::init_apic]: I/O APIC ID: %d\n", get_ioapic_id());
    log_debug("[rsdp::init_apic]: Keyboard IRQ: %d (flags: 0x%x)\n", get_keyboard_global_irq(), get_keyboard_irq_flags());

    log_debug("[rsdp::init_apic]: ACPI/APIC initialization complete!\n");
}
