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
#include "drivers/timer.h"
#include <jury/i386/test_paging.h>
#include <jury/i386/test_vm.h>

// declare kernel_end from linker script
extern uint32_t kernel_end;

// from kheap.c
extern uint32_t placement_address;

void kernel_main(void) {
    // debug output w/ qemu
    init_serial();
    log_debug("serial port initialized for debugging\n");

    log_demo();

    // we'll use this to test paging some amount after the end of the kernel
    placement_address = (virt_addr_t) &kernel_end;

    // IRS and segmentation (we implement paging later)
    init_descriptor_tables();
    terminal_initialize();

    init_paging();
    log_success("paging enabled\n");

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
    uint32_t ioapic_addr = get_ioapic_address();
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
        uint8_t local_apic_id = get_local_apic_id();
        log_info("[kernel]: Local APIC ID: %d\n", local_apic_id);

        // configure timer interrupt (IRQ 0 -> vector 32)
        // according to MADT: IRQ 0 is overridden to GSI 2
        uint32_t timer_gsi = 2;  // from MADT IRQ override
        uint16_t timer_flags = 0x0;  // from MADT itself
        log_info("[kernel]: configuring timer (GSI %d -> vector 32)\n", timer_gsi);
        configure_ioapic_irq_with_flags((void *) ioapic_addr, timer_gsi, 32, local_apic_id, timer_flags);
        log_info("[kernel]: timer interrupt configured via IOAPIC\n");

        // Configure IOAPIC redirection entry for keyboard with proper flags
        // IRQ 1 (keyboard) -> vector 33, with polarity/trigger from MADT
        configure_ioapic_irq_with_flags((void *) ioapic_addr, kbd_gsi, 33, local_apic_id, kbd_flags);
        log_info("[kernel]: keyboard interrupt configured via IOAPIC\n");
    }

    log_info("timer initialization\n");
    init_timer();

    log_info("keyboard initialization\n");
    init_keyboard();

    // tests
    test_paging();
    test_vm();

    // everything is now setup and we are ready to enable interrupts again
    asm volatile("sti");
    log_success("[kernel]: interrupts re-enabled\n");

    log_success("[kernel]: system ready\n");

    log_info("try typing :)\n");

    hush_init();

    // interrupts will fire and be handled while halted
    halt();
}
