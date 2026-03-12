#include "ac97.h"
#include <drivers/io.h>
#include <drivers/pci.h>
#include <drivers/timer.h>
#include <drivers/serial.h>
#include <halt.h>

static void controller_reset(uint32_t);
static void ready_codec(uint32_t);
static void abort();

int ac97_init() {
    struct pci_address_t ac97_bdf = pci_find_device(AC97_VENDOR_ID, AC97_DEVICE_ID);
    if (!ac97_bdf.valid) {
        return 1;
    }

    // nambar holds codec mixer registers
    uint32_t nambar = pci_read(ac97_bdf, AC97_NAMBAR);
    // nabmbar holds bus master / dma control
    uint32_t nabmbar = pci_read(ac97_bdf, AC97_NABMBAR);

    controller_reset(nabmbar);
    ready_codec(nambar);

    return 0;
}

static void controller_reset(uint32_t nabmbar) {
    /* write the global reset bit to NABMBAR + 0x2C */
    // there's two resets - cold completely resets the hardware, warm only resets the 'AC-link'
    outl(nabmbar + AC97_GLOBAL_CONTROL, AC97_GLOBAL_CONTROL_COLD_RESET);

    // wait at least one uS
    sleep_ms(1);

    /* unwrite the global reset bit */
    outl(nabmbar + AC97_GLOBAL_CONTROL, 0);
}

static void ready_codec(uint32_t nabmbar) {
    // poll until the codec ready bit in NAMBAR + 0x30 is set (global status register)

    // timeout and abort 1000ms if status register doesn't reflect codec is ready
    if (!timeout(10000, !(inl(nabmbar + AC97_GLOBAL_STATUS) & AC97_GLOBAL_STATUS_CODEC_READY))) {
        abort();
    }

    serial_write("[ OK ]: ac97.c: codec ready\n");
}

static void abort() {
    serial_write("[ERROR]: ac97.c: timeout while awaiting codec to be ready\n");
    hcf();
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
