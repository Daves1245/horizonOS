#ifndef PIC_H
#define PIC_H

#include <stdint.h>

struct pci_address_t {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    int valid; /* not part of BDF, externally added to check if value exists */
};

// we only need to find the AC97 chip for now,
// so we can just scan for it manually. TODO replace with proper lookup
struct pci_address_t pci_find_device(int vendor, int device);
uint32_t pci_read(struct pci_address_t addr, uint8_t reg_offset);
void pci_write(struct pci_address_t addr, uint8_t reg_offset, uint32_t value);

#endif
