#include <stdio.h>
#include <stdint.h>

#include <kernel/tty.h>

#include "memory/paging.h"
#include "gdt/descriptor_tables.h"
#include "common/common.h"
#include <halt.h>
#include <kernel/logger.h>
#include <i386/drivers/shell/hush.h>
#include <apic/apic.h>
#include <apic/madt.h>
#include <apic/rsdp.h>
#include "drivers/keyboard/keyboard.h"
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <jury/i386/test_paging.h>
#include <jury/i386/test_vm.h>
#include "multiboot_info.h"
#include <drivers/graphics.h>
#include <drivers/console.h>
#include <drivers/ac97.h>
#include <games/pong.h>

// declare kernel_end from linker script
extern uint32_t kernel_end;

// from kheap.c
extern uint32_t placement_address;

uint32_t ioapic_addr;
uint8_t local_apic_id;

void kernel_main(uint32_t multiboot_info_p) {
    // debug output w/ qemu
    init_serial();
    log_debug("serial port initialized for debugging\n");

    log_demo();

    struct multiboot_info *mb_info = (struct multiboot_info *) multiboot_info_p;

    log_info("[multiboot]: info pointer: 0x%x\n", multiboot_info_p);
    log_info("[multiboot]: flags: 0x%x\n", mb_info->flags);

    if (mb_info->flags & MB_FLAG_MMAP)
        log_success("[multiboot]: memory map present (mmap_addr=0x%x len=%d)\n",
            mb_info->mmap_addr, mb_info->mmap_length);
    else
        log_warn("[multiboot]: no memory map\n");

    if (mb_info->flags & MB_FLAG_FRAMEBUFFER) {
        log_success("[multiboot]: framebuffer info present\n");
        log_info("[multiboot]: type=%d addr=0x%x pitch=%d %dx%d bpp=%d\n",
            mb_info->framebuffer_type,
            (uint32_t) mb_info->framebuffer_addr,
            mb_info->framebuffer_pitch,
            mb_info->framebuffer_width,
            mb_info->framebuffer_height,
            mb_info->framebuffer_bpp);
    } else {
        log_warn("[multiboot]: no framebuffer info — vbe request likely ignored by grub\n");
    }

    // we'll use this to test paging some amount after the end of the kernel
    placement_address = (virt_addr_t) &kernel_end;

    // IRS and segmentation (we implement paging later)
    init_descriptor_tables();
    terminal_initialize();

    init_paging();
    log_success("paging enabled\n");

    extern void graphics_init(struct multiboot_info *mbi);
    graphics_init(mb_info);
    if (gfx_get_fb()->address) {
        console_init();
        logger_init_console();
        console_clear();
        console_puts("Horizon OS - i386\n");
        console_printf("framebuffer: %ux%u bpp=%u pitch=%u\n",
            gfx_get_fb()->width, gfx_get_fb()->height,
            gfx_get_fb()->bpp,   gfx_get_fb()->pitch);
        pong_init(gfx_get_fb()->width, gfx_get_fb()->height);
    }

#ifdef DEBUG
    log_debug("[kernel]: check_msr (apic base msr): %d", check_msr());
#endif

    // Initialize ACPI/APIC system
    log_info("ACPI/APIC Initialization\n");
    initialize_apic();

    // APIC base is already mapped during init_paging()

    // mask legacy PIC interrupts
    disable_pic();

    // configure IOAPIC for keyboard interrupt
#ifdef DEBUG
    log_debug("Configuring Keyboard Interrupt\n");
#endif
    ioapic_addr = get_ioapic_address();
    if (ioapic_addr == 0) {
        log_error("[kernel]: ERROR: No IOAPIC found in MADT\n");
        halt();
    } else {
        log_info("[kernel]: IOAPIC address: 0x%x\n", ioapic_addr);

        // IOAPIC is already mapped during init_paging()

        // get keyboard IRQ information from MADT
        uint32_t kbd_gsi = get_keyboard_global_irq();
        uint16_t kbd_flags = get_keyboard_irq_flags();
        log_info("[kernel]: Keyboard GSI: %d, Flags: 0x%x\n", kbd_gsi, kbd_flags);

        // get Local APIC ID for this processor
        local_apic_id = get_local_apic_id();
        log_info("[kernel]: Local APIC ID: %d\n", local_apic_id);

        // configure timer interrupt (IRQ 0 -> vector 32)
        // according to MADT: IRQ 0 is overridden to GSI 2
        uint32_t timer_gsi = 2;  // from MADT IRQ override
        uint16_t timer_flags = 0x0;  // from MADT itself
        log_info("[kernel]: configuring timer (GSI %d -> vector 32)\n", timer_gsi);
        configure_ioapic_irq_with_flags(timer_gsi, 32, local_apic_id, timer_flags);
        log_info("[kernel]: timer interrupt configured via IOAPIC\n");

        // Configure IOAPIC redirection entry for keyboard with proper flags
        // IRQ 1 (keyboard) -> vector 33, with polarity/trigger from MADT
        configure_ioapic_irq_with_flags(kbd_gsi, 33, local_apic_id, kbd_flags);
        log_info("[kernel]: keyboard interrupt configured via IOAPIC\n");
    }

    log_info("timer initialization\n");
    init_timer();

    log_info("keyboard initialization\n");
    init_keyboard();

#ifdef DEBUG
    // tests
    test_paging();
    test_vm();
#endif

    // everything is now setup and we are ready to enable interrupts again
    asm volatile("sti");
    log_success("[kernel]: interrupts re-enabled\n");

    log_success("[kernel]: system ready\n");

    if (ac97_init()) {
        log_warn("[kernel]: ac97 not available\n");
    } else {
        log_success("[kernel]: ac97 initialized\n");
    }

    log_info("try typing :)\n");

    hush_init();

    // interrupts will fire and be handled while halted
    halt();
}
