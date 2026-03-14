#include "ac97.h"
#include <drivers/io.h>
#include <drivers/pci.h>
#include <drivers/timer.h>
#include <drivers/serial.h>
#include <x86_64/interrupts/isr.h>
#include <apic/apic.h>
#include <halt.h>
#include <kheap.h>
#include <mm.h>

#define AC97_PCI_INTERRUPT_LINE 0x3C

extern void *ioapic_addr;
extern uint8_t local_apic_id;

static void controller_reset();
static void enable_bus_master(struct pci_address_t);
static void ready_codec();
static void configure_codec();
static void ac97_irq_handler(struct interrupt_context *regs);
static void abort();

static struct bdl_entry ring_buffer[NUM_BDL_ENTRIES] = {0};

static int last_valid_index = 0;

static void get_embedded_audio_addr() {

}

static uint32_t nambar, nabmbar;

int ac97_init() {
    struct pci_address_t ac97_bdf = pci_find_device(AC97_VENDOR_ID, AC97_DEVICE_ID);
    if (!ac97_bdf.valid) {
        return 1;
    }

    // nambar holds codec mixer registers
    nambar = pci_read(ac97_bdf, AC97_NAMBAR);
    // nabmbar holds bus master / dma control
    nabmbar = pci_read(ac97_bdf, AC97_NABMBAR);

    // BAR values are masked from pci_read, and bits 0 and 1 are type indicators, not part of the
    // I/O base. we need to mask them out.
    nambar &= ~0x3;
    nabmbar &= ~0x3;

    enable_bus_master(ac97_bdf);
    controller_reset();
    ready_codec();
    configure_codec();

    // read the PCI interrupt line to determine which IRQ the device is on
    uint8_t irq = pci_read(ac97_bdf, AC97_PCI_INTERRUPT_LINE) & 0xFF;
    uint8_t vector = 32 + irq;

    register_interrupt_handler(vector, ac97_irq_handler);
    configure_ioapic_irq_with_flags(ioapic_addr, irq, vector, local_apic_id,
        0x08 | 0x02); // level-triggered, active-low (PCI convention)

    // enable interrupts on the PCM out channel (IOC + FIFO error)
    outb(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_CONTROL_REGISTER,
        AC97_CONTROL_REGISTER_LAST_VALID_BUFFER_INTERRUPT_ENABLE |
        AC97_CONTROL_REGISTER_ERROR_INTERRUPT_ENABLE |
        AC97_CONTROL_REGISTER_INTERRUPT_ON_COMPLETION_ENABLE);

    serial_write("[ OK ]: ac97.c: interrupt handler registered\n");

    return 0;
}

static void enable_bus_master(struct pci_address_t bdf) {
    uint32_t cmd = pci_read(bdf, AC97_PCI_COMMAND_REGISTER);
    cmd |= AC97_BUS_MASTER_ENABLE;
    pci_write(bdf, AC97_PCI_COMMAND_REGISTER, cmd);
}

static void controller_reset() {
    /* write the global reset bit to NABMBAR + 0x2C */
    // there's two resets - cold completely resets the hardware, warm only resets the 'AC-link'
    outl(nabmbar + AC97_GLOBAL_CONTROL, AC97_GLOBAL_CONTROL_COLD_RESET);

    // wait at least one uS
    sleep_ms(1);

    /* unwrite the global reset bit */
    outl(nabmbar + AC97_GLOBAL_CONTROL, 0);
}

static void ready_codec() {
    // poll until the codec ready bit in NAMBAR + 0x30 is set (global status register)

    // timeout and abort 1000ms if status register doesn't reflect codec is ready
    if (!timeout(10000, inl(nabmbar + AC97_GLOBAL_STATUS) & AC97_GLOBAL_STATUS_CODEC_READY)) {
        abort();
    }

    serial_write("[ OK ]: ac97.c: codec ready\n");
}

static void configure_codec() {
    // unmute and set max volume: bits [5:0] = right attenuation, [13:8] = left attenuation (0 = 0dB), bit 15 = mute
    outw(nambar + AC97_NAM_MASTER_VOLUME, 0x0000);
    outw(nambar + AC97_NAM_PCM_OUT_VOLUME, 0x0000);

    // this is where we should set the sample rate. 4800 is the default - leave it unchanged
    outw(nambar + AC97_NAM_SAMPLE_RATE, 48000);

    serial_write("[ OK ]: ac97.c: codec configured\n");
}

static void abort() {
    serial_write("[ERROR]: ac97.c: timeout while awaiting codec to be ready\n");
    hcf();
}

// reference driver: https://github.com/klange/toaruos/blob/master/modules/ac97.c

void ac97_setup_bdl(phys_addr_t audio_start, phys_addr_t audio_end) {
    if (audio_start > 0xFFFFFFFF || audio_end > 0xFFFFFFFF) {
        serial_write("[ERROR]: ac97.c: audio data not in 32-bit addressable memory\n");
        hcf();
    }

    // fill the ring buffer with audio data
    for (int i = 0; i < NUM_BDL_ENTRIES && audio_start < audio_end; i++) {
        ring_buffer[i].buffer_addr_phys = audio_start;
        ring_buffer[i].num_samples = 0xFFE; // max. sample number should always be even for some
                                            // reason (check notes?)

        // set IOC flag on entries from which we want interrupts from
        ring_buffer[i].flags = AC97_BDL_IOC; // ideally I suppose we'll only want to flag the last
                                             // entry as we fill the buffer - along with the last
                                             // entry in the data. for now, we can just interrupt
                                             // and handle each individually - this is less complicated
                                             // code for now
        audio_start += 0xFFE * 2;
    }
}

void ac97_start_playback() {
    // tell NABMBAR + BDL_BASE_ADDRESS where our data (ring_buffer) lies
    outl(nabmbar + AC97_PCM_OUT_BASE + AC97_BDL_BASE_ADDR, (uint32_t)virt_to_phys((virt_addr_t)ring_buffer));

    // write an initial LVI to NABMBAR + 0x15 (PCM out last valid index)
    outb(nabmbar + AC97_PCM_OUT_BASE + AC97_LAST_VALID_INDEX, last_valid_index++);

    // write the run/pause bit (0x1) to NABMBAR + 0x1B (pcm out transfer control)
    outb(nabmbar + AC97_PCM_OUT_TRANSFER_CONTROL, AC97_CONTROL_REGISTER_RUN_PAUSE_BUS_MASTER);
}

static void ac97_irq_handler(struct interrupt_context *regs) {
    (void)regs;

    // read per-channel status register for PCM out
    uint16_t status = inw(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_STATUS_REGISTER);

    serial_write("[DEBUG]: [ac97.c]: interrupt\n");

    int complete = status & AC97_STATUS_REGISTER_BUFFER_COMPLETION_INTERRUPT_STATUS;
    int is_last_valid = status & AC97_STATUS_REGISTER_CURRENT_EQUALS_LAST_VALID;
    int is_fifo_error = status & AC97_STATUS_REGISTER_FIFO_ERROR;

    if (is_fifo_error) {
        serial_write("[ERROR]:[ac97.c]: unexpected fifo error\n");
        hcf();
    }

    // acknowledge status bits by writing them back (W1C)
    outw(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_STATUS_REGISTER, status);

    if (!complete) {
        apic_send_eoi();
        return;
    }

    if (!is_last_valid) {
        serial_write("[ERROR]: [ac97.c]: not last index but completed buffer\n");
        hcf();
    }

    // advance LVI by 1 modulo 32 and write it to NABMBAR + 0x15
    last_valid_index = (last_valid_index + 1) & 31;
    outb(nabmbar + AC97_PCM_OUT_BASE + AC97_LAST_VALID_INDEX, last_valid_index);

    apic_send_eoi();
}
