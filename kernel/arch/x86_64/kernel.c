#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include <kernel/tty.h>
#include <common.h>
#include <drivers/serial.h>

#ifdef __x86_64__
#include <x86_64/interrupts/descriptor_tables.h>
#include <x86_64/memory/paging.h>
#else
#include "../arch/i386/interrupts/descriptor_tables.h"
#include <i386/memory/paging.h>
#endif

extern void halt_without_apic();
extern void hcf(void);

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// Request framebuffer from Limine
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
};

// Request HHDM (Higher Half Direct Map) from Limine
__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
};

// Request memory map from Limine
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

// Request kernel file info from Limine
__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_file_request kernel_file_request = {
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0,
};

// Request specific paging mode from Limine
__attribute__((used, section(".limine_requests")))
static volatile struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .mode = LIMINE_PAGING_MODE_X86_64_4LVL,
    .max_mode = LIMINE_PAGING_MODE_X86_64_4LVL,
    .min_mode = LIMINE_PAGING_MODE_X86_64_4LVL,
};

// instead of searching the BIOS area we can just request the RSDP location from limine directly
__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

// Define the start and end markers for the Limine requests.
__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

/*
int check_msr() {
    uint32_t eax, ebx, ecx, edx;
    asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    return (edx & (1 << 5)) != 0;
}
*/

extern uint64_t kernel_end;
extern uint64_t placement_address;

void kernel_main(void) {
    // Early initialization logging
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
        //printf("ERROR: Limine base revision not supported!\n");
    }

    // Check HHDM response
    if (hhdm_request.response == NULL) {
        //printf("ERROR: No HHDM response - this is critical for x86_64!\n");
        hcf();
    } else {
        //printf("HHDM offset: 0x%lx\n", hhdm_request.response->offset);
    }

    // Check paging mode response
    if (paging_mode_request.response == NULL) {
        //printf("ERROR: No paging mode response!\n");
        hcf();
    } else {
        //printf("Paging mode: %d\n", paging_mode_request.response->mode);
    }

    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        //printf("No limine framebuffer available\n");
    } else {
        // Fetch first framebuffer
        struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

        //printf("Limine framebuffer detected:\n");

        for (size_t i = 0; i < 100; i++) {
            volatile uint32_t *fb_ptr = framebuffer->address;
            fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
        }
    }

    // initialize placement address to end of kernel
    //placement_address = (uintptr_t) &kernel_end;

    init_serial();

    halt_without_apic();
    serial_write("APIC supported\n");

    serial_write("checking rsdp response\n");
    if (rsdp_request.response == NULL) {
        serial_write("error getting rsdp address\n");
        halt();
    }
    serial_write("rsdp response OK\n");

    serial_write("initializing apic\n");
    initialize_apic();
    serial_write("apic initialized\n");

    serial_write("disabling pic\n");
    disable_pic();
    serial_write("pic disabled\n");

    serial_write("getting ioapic address\n");
    uint32_t ioapic_addr = get_ioapic_address();

    if (ioapic_addr == 0) {
        serial_write("ioapic not found\n");
        halt();
    }
    serial_write("ioapic address obtained\n");

    serial_write("getting keyboard irq info\n");
    uint32_t kbd_gsi = get_keyboard_global_irq();
    uint16_t kbd_flags = get_keyboard_irq_flags();
    serial_write("keyboard irq info obtained\n");

    serial_write("getting local apic id\n");
    uint8_t local_apic_id = get_local_apic_id();
    serial_write("local apic id obtained\n");

    // configure timer interrupt (IRQ 0 -> vector 32)
    // according to MADT: IRQ 0 is overriden to GSI 2
    uint32_t timer_gsi = 2; // madt override
    uint16_t timer_flags = 0x0; // madt itself
    serial_write("configuring timer\n");
    configure_ioapic_irq_with_flags((void *) ioapic_addr, timer_gsi, 32, local_apic_id, timer_flags);
    serial_write("timer interrupt configured via IOAPIC\n");
    configure_ioapic_irq_with_flags((void *) ioapic_addr, kbd_gsi, 33, local_apic_id, kbd_flags);

    serial_write("timer initialization\n");
    init_timer();

    serial_write("keyboard initialization\n");
    init_keyboard();

    // ready to enable interrupts again
    asm volatile("sti");
    serial_write("interrupts re-enabled");
    //printf("check_msr (apic base msr): %d", check_msr());

    hcf();
}
