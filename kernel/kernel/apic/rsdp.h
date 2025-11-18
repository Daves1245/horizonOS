#ifndef RSDP_H
#define RSDP_H

#include <stdint.h>
#include "madt.h"

// in version 1.0
struct rsdp_t {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_addr;
} __attribute__((packed));

// version 2.0
struct xsdp_t {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_addr; // deprecated since version 2.0
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute((packed));


// RSDT structure (ACPI v1.0) 
struct rsdt_t {
    struct apic_header header;
    uint32_t entry_ptrs[];
} __attribute__((packed));

// XSDT structure (ACPI v2.0+)
struct xsdt_t {
    struct apic_header header;  
    uint64_t entry_ptrs[];
} __attribute__((packed));

uint32_t *find_rsdp();
uint32_t validate_rsdp_checksum(uint32_t *rsdp_addr);
uint32_t *find_madt(uint32_t *rsdp_addr);
void initialize_apic();

#endif
