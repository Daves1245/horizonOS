#include "ac97.h"
#include <drivers/io.h>
#include <drivers/pci.h>

static void controller_reset(uint32_t);
static void ready_codec(uint32_t);

int ac97_init() {
    struct pci_address_t ac97_bdf = pci_find_device(AC97_VENDOR_ID, AC97_DEVICE_ID);
    if (!ac97_bdf.valid) {
        return 1;
    }

    uint32_t nambar_addr = pci_read(ac97_bdf, AC97_NAMBAR);
    controller_reset(nambar_addr);
    ready_codec(nambar_addr);

    return 0;
}

static void controller_reset(uint32_t nambar_addr) {
    // write the global reset bit to NAMBAR + 0x2C

    // wait for it to clear

    // wait some amount for the codec
}

static void ready_codec(uint32_t nambar_addr) {
    // poll until the codec ready bit in NAMBAR + 0x30 is set (global status register)
}

void ac97_setup_bdl() {
    // allocate a ring buffer of 32 audio buffer entries

    // fill the ring buffer with audio data

    // write the physical address and sample count for each entry

    // set IOC flag on entries from which we want interrupts from
}

void ac97_start_playback() {
    // write an initial LVI to NAMBAR + 0x15 (PCM out last valid index)

    // write the run/pause bit (0x1) to NAMBAR + 0x1B (pcm out transfer control)
}

void ac97_runtime_loop() {
    // handle interrupts from NAMBAR + 0x16 (PCM out status register) for buffer completion

    // refill the consumed buffer with new audio data

    // advance LVI by 1 module 32 and write it to NAMBAR + 0x15

    // loop
}
