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
static int bdl_entries_filled = 0;

static phys_addr_t audio_cur_pos;  /* next audio position to fill into BDL */
static phys_addr_t audio_data_end; /* physical end of audio data */

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

    serial_write_hex("[DEBUG]: ac97.c: nambar  = ", nambar);
    serial_write_hex("[DEBUG]: ac97.c: nabmbar = ", nabmbar);

    enable_bus_master(ac97_bdf);

    // verify bus master was enabled
    uint32_t pci_cmd = pci_read(ac97_bdf, AC97_PCI_COMMAND_REGISTER);
    serial_write_hex("[DEBUG]: ac97.c: PCI command register after enable_bus_master = ", pci_cmd);

    controller_reset();

    // dump global control register after reset
    uint32_t glob_cnt = inl(nabmbar + AC97_GLOBAL_CONTROL);
    serial_write_hex("[DEBUG]: ac97.c: GLOB_CNT after reset = ", glob_cnt);

    ready_codec();
    configure_codec();

    // read the PCI interrupt line to determine which IRQ the device is on
    uint8_t irq = pci_read(ac97_bdf, AC97_PCI_INTERRUPT_LINE) & 0xFF;
    uint8_t vector = 32 + irq;
    serial_write_hex("[DEBUG]: ac97.c: IRQ = ", irq);
    serial_write_hex("[DEBUG]: ac97.c: vector = ", vector);

    register_interrupt_handler(vector, ac97_irq_handler);

    // verify IDT entry for this vector is valid (diagnose the garbage-stub-address bug)
    // IDTR gives us the IDT base; each entry is 16 bytes
    struct { uint16_t limit; uint64_t base; } __attribute__((packed)) idtr;
    asm volatile("sidt %0" : "=m"(idtr));
    uint8_t *idt_entry_bytes = (uint8_t *)(idtr.base + vector * 16);
    // reconstruct handler address from the packed IDT gate fields
    uint16_t e_off_low  = *(uint16_t *)(idt_entry_bytes + 0);
    uint16_t e_sel      = *(uint16_t *)(idt_entry_bytes + 2);
    uint8_t  e_attr     = *(uint8_t  *)(idt_entry_bytes + 5);
    uint16_t e_off_mid  = *(uint16_t *)(idt_entry_bytes + 6);
    uint32_t e_off_high = *(uint32_t *)(idt_entry_bytes + 8);
    serial_write_hex("[DEBUG]: ac97.c: IDT[43] selector = ", e_sel);
    serial_write_hex("[DEBUG]: ac97.c: IDT[43] type_attr= ", e_attr);
    serial_write_hex("[DEBUG]: ac97.c: IDT[43] off_high = ", e_off_high);
    serial_write_hex("[DEBUG]: ac97.c: IDT[43] off_mid  = ", e_off_mid);
    serial_write_hex("[DEBUG]: ac97.c: IDT[43] off_low  = ", e_off_low);
    // expected: selector=0x0008, type_attr=0x8E, off_high=0xffffffff, off_mid=0x8000..., off_low=<stub>

    // MADT override for IRQ 11 specifies active-HIGH, level-triggered (flags=0xd: bits[1:0]=01=active-high, bits[3:2]=11=level).
    // QEMU's PCI model asserts interrupts active-HIGH internally, so active-low polarity in the IOAPIC
    // redirection entry would suppress delivery. Use 0x08 (level-triggered, active-high).
    configure_ioapic_irq_with_flags(ioapic_addr, irq, vector, local_apic_id,
        0x08); // level-triggered, active-high (matches MADT override flags=0xd for IRQ 11)

    // read back the IOAPIC redirection entry to verify it was written correctly
    // IRQ N entry: low=reg 0x10+N*2, high=reg 0x10+N*2+1
    // expected low: 0x0000802b (vector=43, level-triggered, active-high, unmasked, fixed delivery)
    // expected high: 0x00000000 (dest APIC id=0)
    uint32_t ioapic_low  = ioapic_read(ioapic_addr, 0x10 + irq * 2);
    uint32_t ioapic_high = ioapic_read(ioapic_addr, 0x10 + irq * 2 + 1);
    serial_write_hex("[DEBUG]: ac97.c: IOAPIC entry low  = ", ioapic_low);
    serial_write_hex("[DEBUG]: ac97.c: IOAPIC entry high = ", ioapic_high);

    // enable interrupts on the PCM out channel (IOC + FIFO error)
    outb(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_CONTROL_REGISTER,
        AC97_CONTROL_REGISTER_LAST_VALID_BUFFER_INTERRUPT_ENABLE |
        AC97_CONTROL_REGISTER_ERROR_INTERRUPT_ENABLE |
        AC97_CONTROL_REGISTER_INTERRUPT_ON_COMPLETION_ENABLE);

    uint8_t cr_after_irq_enable = inb(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_CONTROL_REGISTER);
    serial_write_hex("[DEBUG]: ac97.c: PCM out CR after enabling IRQs = ", cr_after_irq_enable);

    serial_write("[ OK ]: ac97.c: interrupt handler registered\n");

    return 0;
}

static void enable_bus_master(struct pci_address_t bdf) {
    uint32_t cmd = pci_read(bdf, AC97_PCI_COMMAND_REGISTER);
    cmd |= AC97_BUS_MASTER_ENABLE | (1 << 0); // bus master + I/O space enable
    pci_write(bdf, AC97_PCI_COMMAND_REGISTER, cmd);
}

static void controller_reset() {
    /* cold reset: CR bit (bit 1) = 0 asserts reset, = 1 deasserts it.
     * to perform a cold reset: clear CR (write 0 to bit 1), wait, then set CR to come out of reset.
     * AC97_GLOBAL_CONTROL_COLD_RESET = (1 << 1), so writing just GIE (bit 0) clears CR -> asserts reset.
     * then write CR | GIE to deassert reset and keep global interrupt enable. */
    uint32_t glob_cnt_before = inl(nabmbar + AC97_GLOBAL_CONTROL);
    serial_write_hex("[DEBUG]: ac97.c: GLOB_CNT before reset = ", glob_cnt_before);

    // assert cold reset: CR = 0
    outl(nabmbar + AC97_GLOBAL_CONTROL, AC97_GLOBAL_CONTROL_INTERRUPT_ENABLE);

    // wait at least one uS (using 1ms to be safe)
    sleep_ms(1);

    // deassert cold reset: set CR = 1, keep GIE
    outl(nabmbar + AC97_GLOBAL_CONTROL,
         AC97_GLOBAL_CONTROL_COLD_RESET | AC97_GLOBAL_CONTROL_INTERRUPT_ENABLE);
}

static void ready_codec() {
    // poll until the codec ready bit in NAMBAR + 0x30 is set (global status register)

    // timeout and abort 1000ms if status register doesn't reflect codec is ready
    if (!timeout(10000, inl(nabmbar + AC97_GLOBAL_STATUS) & AC97_GLOBAL_STATUS_CODEC_READY)) {
        abort();
    }

    uint32_t glob_status = inl(nabmbar + AC97_GLOBAL_STATUS);
    serial_write_hex("[DEBUG]: ac97.c: GLOB_STATUS = ", glob_status);
    serial_write("[ OK ]: ac97.c: codec ready\n");
}

static void configure_codec() {
    // unmute and set max volume: bits [5:0] = right attenuation, [13:8] = left attenuation (0 = 0dB), bit 15 = mute
    outw(nambar + AC97_NAM_MASTER_VOLUME, 0x0000);
    outw(nambar + AC97_NAM_PCM_OUT_VOLUME, 0x0000);

    // this is where we should set the sample rate. 4800 is the default - leave it unchanged
    outw(nambar + AC97_NAM_SAMPLE_RATE, 48000);

    // readback to verify codec accepted the writes
    uint16_t master_vol = inw(nambar + AC97_NAM_MASTER_VOLUME);
    uint16_t pcm_vol    = inw(nambar + AC97_NAM_PCM_OUT_VOLUME);
    uint16_t rate       = inw(nambar + AC97_NAM_SAMPLE_RATE);
    serial_write_hex("[DEBUG]: ac97.c: master volume readback = ", master_vol);
    serial_write_hex("[DEBUG]: ac97.c: PCM out volume readback = ", pcm_vol);
    serial_write_hex("[DEBUG]: ac97.c: sample rate readback = ", rate);

    serial_write("[ OK ]: ac97.c: codec configured\n");
}

static void abort() {
    serial_write("[ERROR]: ac97.c: timeout while awaiting codec to be ready\n");
    hcf();
}

// reference driver: https://github.com/klange/toaruos/blob/master/modules/ac97.c

void ac97_setup_bdl(phys_addr_t audio_start, phys_addr_t audio_end) {
    serial_write_hex("[DEBUG]: ac97.c: audio_start phys = ", (uint32_t)audio_start);
    serial_write_hex("[DEBUG]: ac97.c: audio_end phys = ", (uint32_t)audio_end);
    serial_write_hex("[DEBUG]: ac97.c: audio size (bytes) = ", (uint32_t)(audio_end - audio_start));

    if (audio_start > 0xFFFFFFFF || audio_end > 0xFFFFFFFF) {
        serial_write("[ERROR]: ac97.c: audio data not in 32-bit addressable memory\n");
        hcf();
    }

    // fill the ring buffer with audio data
    int i;
    for (i = 0; i < NUM_BDL_ENTRIES && audio_start < audio_end; i++) {
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
    bdl_entries_filled = i;
    /* audio_start has been advanced by the loop; save it as the next position to fill */
    audio_cur_pos  = audio_start;
    audio_data_end = audio_end;

    serial_write_hex("[DEBUG]: ac97.c: BDL entries filled = ", bdl_entries_filled);
    serial_write_hex("[DEBUG]: ac97.c: ring_buffer phys = ", (uint32_t)virt_to_phys((virt_addr_t)ring_buffer));
    if (bdl_entries_filled > 0) {
        serial_write_hex("[DEBUG]: ac97.c: BDL[0].buffer_addr_phys = ", ring_buffer[0].buffer_addr_phys);
        serial_write_hex("[DEBUG]: ac97.c: BDL[0].num_samples = ", ring_buffer[0].num_samples);
        serial_write_hex("[DEBUG]: ac97.c: BDL[0].flags = ", ring_buffer[0].flags);
    }
}

void ac97_start_playback() {
    uint32_t bdl_phys = (uint32_t)virt_to_phys((virt_addr_t)ring_buffer);
    serial_write_hex("[DEBUG]: ac97.c: writing BDL base addr = ", bdl_phys);

    // tell NABMBAR + BDL_BASE_ADDRESS where our data (ring_buffer) lies
    outl(nabmbar + AC97_PCM_OUT_BASE + AC97_BDL_BASE_ADDR, bdl_phys);

    // verify BDL address was accepted
    uint32_t bdl_readback = inl(nabmbar + AC97_PCM_OUT_BASE + AC97_BDL_BASE_ADDR);
    serial_write_hex("[DEBUG]: ac97.c: BDL base addr readback = ", bdl_readback);

    // set LVI to the last filled BDL entry
    last_valid_index = bdl_entries_filled - 1;
    outb(nabmbar + AC97_PCM_OUT_BASE + AC97_LAST_VALID_INDEX, last_valid_index);
    serial_write_hex("[DEBUG]: ac97.c: LVI set to             = ", last_valid_index);

    // check status register before starting
    uint16_t sr_before = inw(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_STATUS_REGISTER);
    serial_write_hex("[DEBUG]: ac97.c: status reg before start = ", sr_before);

    // start DMA: set run/pause bit while preserving interrupt enable bits in the control register
    uint8_t cr = inb(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_CONTROL_REGISTER);
    serial_write_hex("[DEBUG]: ac97.c: CR before RUN           = ", cr);
    outb(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_CONTROL_REGISTER,
        cr | AC97_CONTROL_REGISTER_RUN_PAUSE_BUS_MASTER);

    uint8_t cr_after = inb(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_CONTROL_REGISTER);
    serial_write_hex("[DEBUG]: ac97.c: CR after RUN            = ", cr_after);
}

void ac97_debug_status() {
    uint8_t civ = inb(nabmbar + AC97_PCM_OUT_BASE + AC97_CURRENT_INDEX_VALUE);
    uint8_t lvi = inb(nabmbar + AC97_PCM_OUT_BASE + AC97_LAST_VALID_INDEX);
    uint16_t sr = inw(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_STATUS_REGISTER);
    uint8_t cr = inb(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_CONTROL_REGISTER);
    uint16_t picb = inw(nabmbar + AC97_PCM_OUT_BASE + AC97_POSITION_IN_CUR_BUFFER);

    serial_write_hex("[DEBUG]: ac97_debug_status: CIV = ", civ);
    serial_write_hex("[DEBUG]: ac97_debug_status: LVI = ", lvi);
    serial_write_hex("[DEBUG]: ac97_debug_status: SR = ", sr);
    serial_write_hex("[DEBUG]: ac97_debug_status: CR = ", cr);
    serial_write_hex("[DEBUG]: ac97_debug_status: PICB = ", picb);

    if (sr & AC97_STATUS_REGISTER_DMA_CONTROLLER_HALTED) {
        serial_write("[DEBUG]: ac97_debug_status: -> DCH set (DMA halted)\n");
    } else {
        serial_write("[DEBUG]: ac97_debug_status: -> DCH clear (DMA running)\n");
    }
}

static void ac97_irq_handler(struct interrupt_context *regs) {
    (void)regs;

    // read per-channel status register for PCM out
    uint16_t status = inw(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_STATUS_REGISTER);
    uint8_t civ = inb(nabmbar + AC97_PCM_OUT_BASE + AC97_CURRENT_INDEX_VALUE);

    serial_write_hex("[DEBUG]: [ac97.c]: interrupt - status = ", status);
    serial_write_hex("[DEBUG]: [ac97.c]: CIV = ", civ);
    serial_write_hex("[DEBUG]: [ac97.c]: LVI = ", last_valid_index);

    int complete = status & AC97_STATUS_REGISTER_BUFFER_COMPLETION_INTERRUPT_STATUS;
    int is_fifo_error = status & AC97_STATUS_REGISTER_FIFO_ERROR;
    int is_halted = status & AC97_STATUS_REGISTER_DMA_CONTROLLER_HALTED;

    if (is_fifo_error) {
        serial_write("[ERROR]:[ac97.c]: unexpected fifo error\n");
        hcf();
    }

    if (is_halted) {
        serial_write("[DEBUG]: [ac97.c]: DMA controller halted\n");
    }

    // acknowledge status bits by writing them back (W1C)
    outw(nabmbar + AC97_PCM_OUT_BASE + AC97_CHANNEL_STATUS_REGISTER, status);

    if (!complete) {
        // spurious or last-valid-only interrupt - check if it's LVBCI (last valid buffer completion)
        int lvbci = status & AC97_STATUS_REGISTER_LAST_VALID_BUFFER_COMPLETION_INTERRUPT;
        if (lvbci) {
            serial_write("[DEBUG]: [ac97.c]: LVBCI (last valid buffer completion interrupt)\n");
        } else {
            serial_write("[DEBUG]: [ac97.c]: spurious interrupt (no BCIS or LVBCI)\n");
        }
        apic_send_eoi();
        return;
    }

    if (audio_cur_pos >= audio_data_end) {
        /* no more audio data - let DMA halt naturally when CIV reaches current LVI */
        apic_send_eoi();
        return;
    }

    /* fill the next BDL slot with the next chunk of audio data, then advance LVI */
    int next_slot = (last_valid_index + 1) & 31;
    uint32_t remaining = (uint32_t)(audio_data_end - audio_cur_pos);
    uint16_t samples = (remaining >= 0xFFE * 2) ? 0xFFE : (uint16_t)(remaining >> 1);
    samples &= ~1; /* must be even for stereo */

    ring_buffer[next_slot].buffer_addr_phys = (uint32_t)audio_cur_pos;
    ring_buffer[next_slot].num_samples = samples;
    ring_buffer[next_slot].flags = AC97_BDL_IOC;

    audio_cur_pos += (phys_addr_t)samples * 2;

    last_valid_index = next_slot;
    outb(nabmbar + AC97_PCM_OUT_BASE + AC97_LAST_VALID_INDEX, last_valid_index);

    apic_send_eoi();
}
