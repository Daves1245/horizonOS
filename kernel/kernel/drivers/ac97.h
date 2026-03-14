#ifndef AC97_H
#define AC97_H

#include <stdint.h>
#include <drivers/pci.h>
#include <mm.h>

/* PCI BAR offsets */
#define AC97_NAMBAR  0x10  /* native audio mixer base address (NAM) */
#define AC97_NABMBAR 0x14  /* native audio bus master base address (NABM) */

/* NABM global registers */
#define AC97_GLOBAL_CONTROL 0x2C  /* global control */
#define AC97_GLOBAL_STATUS 0x30  /* global status */

/* global control bits */
#define AC97_GLOBAL_CONTROL_INTERRUPT_ENABLE (1 << 0)
#define AC97_GLOBAL_CONTROL_COLD_RESET (1 << 1)
#define AC97_GLOBAL_CONTROL_WARM_RESET (1 << 2)

/* global status bits */
#define AC97_GLOBAL_STATUS_PCM_IN_INTERRUPT (1 << 3)
#define AC97_GLOBAL_STATUS_PCM_OUT_INTERRUPT (1 << 4)
#define AC97_GLOBAL_STATUS_CODEC_READY (1 << 8)
#define AC97_GLOBAL_STATUS_SECONDARY_CODEC_READY (1 << 9)

/* NABM per-channel register offsets (add to channel base) */
#define AC97_BDL_BASE_ADDR              0x00  /* Buffer Descriptor List Base Address (32-bit) */
#define AC97_CURRENT_INDEX_VALUE        0x04  /* Current Index Value (8-bit) */
#define AC97_LAST_VALID_INDEX           0x05  /* Last Valid Index (8-bit) */
#define AC97_CHANNEL_STATUS_REGISTER    0x06  /* Channel Status Register (16-bit) */
#define AC97_POSITION_IN_CUR_BUFFER     0x08  /* Position In Current Buffer (16-bit) */
#define AC97_PREFETCHED_INDEX_VALUE     0x0A  /* Prefetched Index Value (8-bit) */
#define AC97_CHANNEL_CONTROL_REGISTER   0x0B  /* Channel Control Register (8-bit) */

/* NABM channel base addresses */
#define AC97_PCM_IN_BASE 0x00  /* PCM In */
#define AC97_PCM_OUT_BASE 0x10  /* PCM Out */
#define AC97_MIC_IN_BASE 0x20  /* Mic In */

/* per-channel control register (CR) bits */
#define AC97_CONTROL_REGISTER_RUN_PAUSE_BUS_MASTER  (1 << 0)  /* Run/Pause Bus Master */
#define AC97_CONTROL_REGISTER_RESET_REGISTERS       (1 << 1)  /* Reset Registers */
#define AC97_CONTROL_REGISTER_LAST_VALID_BUFFER_INTERRUPT_ENABLE (1 << 2)  /* Last Valid Buffer Interrupt Enable */
#define AC97_CONTROL_REGISTER_ERROR_INTERRUPT_ENABLE (1 << 3)  /* FIFO Error Interrupt Enable */
#define AC97_CONTROL_REGISTER_INTERRUPT_ON_COMPLETION_ENABLE (1 << 4)  /* Interrupt on Completion Enable */

/* per-channel status register (SR) bits */
#define AC97_STATUS_REGISTER_DMA_CONTROLLER_HALTED (1 << 0)  /* DMA Controller Halted */
#define AC97_STATUS_REGISTER_CURRENT_EQUALS_LAST_VALID (1 << 1)  /* Current Equals Last Valid */
#define AC97_STATUS_REGISTER_LAST_VALID_BUFFER_COMPLETION_INTERRUPT (1 << 2)  /* Last Valid Buffer Completion Interrupt */
#define AC97_STATUS_REGISTER_BUFFER_COMPLETION_INTERRUPT_STATUS (1 << 3)  /* Buffer Completion Interrupt Status */
#define AC97_STATUS_REGISTER_FIFO_ERROR (1 << 4)  /* FIFO Error */

/* NAM (mixer/codec) registers */
#define AC97_NAM_RESET          0x00  /* Reset */
#define AC97_NAM_MASTER_VOLUME  0x02  /* Master Volume */
#define AC97_NAM_PCM_OUT_VOLUME 0x18  /* PCM Out Volume */
#define AC97_NAM_SAMPLE_RATE    0x2C  /* PCM Front DAC Rate */
#define AC97_NAM_ADC_RATE       0x32  /* PCM LR ADC Rate */

/* BDL */
#define AC97_BDL_MAX_ENTRIES 32

/* BDL entry flags */
#define AC97_BDL_IOC (1 << 15)  /* Interrupt on Completion */
#define AC97_BDL_BUP (1 << 14)  /* Buffer Underrun Policy */

/* vendor and device IDs (pic lookup) */
#define AC97_VENDOR_ID 0x8086
#define AC97_DEVICE_ID 0x2415

#define NUM_BDL_ENTRIES 32

/* wrirte a run/pause bit here to start/pause playback */
#define AC97_PCM_OUT_TRANSFER_CONTROL 0x1B

/* enable the bus master */
#define AC97_PCI_COMMAND_REGISTER 0x04
#define AC97_BUS_MASTER_ENABLE (1 << 2)

struct bdl_entry {
    uint32_t buffer_addr_phys;
    uint16_t num_samples;
    uint16_t flags;
};

/* linker-provided symbols for embedded audio data */
extern uint8_t _binary_audio_start[];
extern uint8_t _binary_audio_end[];

int ac97_init();
void ac97_setup_bdl(phys_addr_t audio_start, phys_addr_t audio_end);
void ac97_start_playback();
void ac97_runtime_loop();

#endif
