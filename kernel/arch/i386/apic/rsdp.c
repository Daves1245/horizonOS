#include <stdint.h>
#include "rsdp.h"
#include <string/string.h>

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
