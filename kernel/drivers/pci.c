#include <drivers/pci.h>
#include <drivers/io.h>

/* find location of a device, if it exists.
 * has nonstandard return - must check that struct type is valid, i.e.
 * not all -1 values. we should return a heap-allocated object and check for
 * NULL instead, but I'm too lazy TODO
 */
struct pci_address_t pci_find_device(int vendor, int device) {
    struct pci_address_t res = {-1, -1, -1};

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t function = 0; function < 32; function++) {
                struct pci_address_t addr = {bus, dev, function, 0};

                // register 0x00 holds vendor and device ID in high and low bits
                uint32_t ids = pci_read(addr, 0x00);
                if (ids == 0xFFFFFFFF) {
                    continue;
                }

                uint16_t vendor_id = ids & 0xFFFF;
                uint16_t device_id = ids >> 16;

                if ((vendor_id == vendor) && (device_id == device)) {
                    res = addr;
                    res.valid = 1;
                    return res;
                }
            }
        }
    }

    return res;
}

uint32_t pci_read(struct pci_address_t addr, uint8_t reg_offset) {
    uint32_t config_addr = (1 << 31) | (addr.bus << 16) | (addr.device << 11) | (addr.function << 8) |
        (reg_offset & 0xFC);
    outl(0xCF8, config_addr);
    return inl(0xCFC);
}

void pci_write(struct pci_address_t addr, uint8_t reg_offset, uint32_t value) {
    uint32_t config_addr = (1 << 31) | (addr.bus << 16) | (addr.device << 11) | (addr.function << 8) |
        (reg_offset & 0xFC);
    outl(0xCF8, config_addr);
    outl(0xCFC, value);
}
