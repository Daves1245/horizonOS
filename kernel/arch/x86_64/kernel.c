/* here be dragons; make sure nobody loves you before proceeding. */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <limine.h>
#include <kernel/tty.h>
#include <common.h>
#include <mm.h>
#include <apic/rsdp.h>
#include <drivers/serial.h>
#include <uacpi/types.h>
#include <uacpi/uacpi_init.h>
#include <kernel/panic.h>
#include <x86_64/acpi/acpi_bus.h>

#include <x86_64/interrupts/descriptor_tables.h>
#include <x86_64/memory/paging.h>
#include <apic/apic.h>
#include <apic/madt.h>
#include <halt.h>
#include <drivers/timer.h>
#include <drivers/graphics.h>
#include <drivers/console.h>

#include <drivers/ac97.h>
#include <games/pong.h>

#include <keyboard.h>
#include <log.h>
#include <shell.h>

extern void ps2k_register(void);
extern void halt_without_apic();
extern void hcf(void);

/* global so that drivers may route IRQ through ioapic (virtual address) */
volatile uint32_t *ioapic_addr;
uint8_t local_apic_id;

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
volatile struct limine_hhdm_request hhdm_request = {
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

// Request kernel physical/virtual base addresses from Limine
__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
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
volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

// Define the start and end markers for the Limine requests.
__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

extern uint64_t kernel_end;
extern uint64_t placement_address;

// mm.h requires this for virt->phys translation
uint64_t hhdm_offset;

virt_addr_t rsdp_addr;

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

    init_serial();

    // walk the memory map and find a usable region for the bump allocator
    if (memmap_request.response == NULL) {
        serial_write("FATAL: no memory map from Limine\n");
        hcf();
    }

    hhdm_offset = hhdm_request.response->offset;

    if (kernel_address_request.response == NULL) {
        serial_write("FATAL: no kernel address response from Limine\n");
        hcf();
    }
    kernel_phys_base = kernel_address_request.response->physical_base;
    kernel_virt_base = kernel_address_request.response->virtual_base;
    uint64_t bump_phys = 0;

    serial_write("Limine memory map:\n");
    struct limine_memmap_response *memmap = memmap_request.response;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        log_info("  [%d] base=0x%x%x len=0x%x%x type=%d\n",
                (int) i,
                (uint32_t)(e->base >> 32), (uint32_t) e->base,
                (uint32_t)(e->length >> 32), (uint32_t) e->length,
                (int) e->type);
        // LIMINE_MEMMAP_USABLE == 0; pick first usable region >= 4MB
        if (bump_phys == 0 && e->type == LIMINE_MEMMAP_USABLE && e->length >= 0x400000) {
            bump_phys = e->base;
        }
    }

    if (bump_phys == 0) {
        serial_write("FATAL: no suitable usable memory region found\n");
        hcf();
    }

    // point the bump allocator at this region via HHDM (already mapped by Limine)
    placement_address = hhdm_offset + bump_phys;
    log_info("bump allocator base: phys=0x%x%x virt=0x%x%x\n",
            (uint32_t)(bump_phys >> 32), (uint32_t)bump_phys,
            (uint32_t)(placement_address >> 32), (uint32_t)placement_address);

    /* bring up the framebuffer console early so printk is visible from here on */
    if (framebuffer_request.response == NULL ||
            framebuffer_request.response->framebuffer_count < 1) {
        serial_write("[kernel.c]: did not receive a framebuffer from limine. exiting\n");
        hcf();
    }
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    graphics_init(framebuffer);
    console_init(framebuffer);
    console_clear();
    pong_init(framebuffer->width, framebuffer->height);

    init_descriptor_tables();

    halt_without_apic();
    printk(KERN_OK, "APIC supported\n");

    printk(KERN_INFO, "checking rsdp response\n");
    if (rsdp_request.response == NULL) {
        printk(KERN_FATAL, "error getting rsdp address\n");
        halt();
    }

    uint64_t rsdp_phys = (uint64_t)rsdp_request.response->address;

    printk(KERN_INFO, "RSDP phys: 0x%x\n", (uint32_t)rsdp_phys);
    printk(KERN_INFO, "HHDM offset: 0x%x%x\n", (uint32_t)(hhdm_offset >> 32), (uint32_t)hhdm_offset);

    // Test HHDM access - try reading from physical address 0x1000 via HHDM
    printk(KERN_DEBUG, "Testing HHDM access at offset+0x1000...\n");
    volatile uint8_t *test_addr = (uint8_t *)(hhdm_offset + 0x1000);
    uint8_t test_val = *test_addr;  // Should not crash if HHDM works
    printk(KERN_OK, "HHDM test read successful, value: 0x%x\n", test_val);

    rsdp_addr = rsdp_phys + hhdm_offset;
    printk(KERN_INFO, "RSDP virt: 0x%x%x\n", (uint32_t)(rsdp_addr >> 32), (uint32_t)rsdp_addr);
    printk(KERN_OK, "rsdp response OK\n");

    // Map RSDP page (base revision 3 doesn't map ACPI/reserved memory)
    printk(KERN_INFO, "Mapping RSDP page...\n");
    map_physical_range(rsdp_phys, 4096, 1, 1);  // Map 4KB, kernel, writable
    printk(KERN_OK, "RSDP page mapped\n");

    int acpi_result = acpi_init();
    printk(KERN_INFO, "acpi_init returned: %d\n", acpi_result);

    printk(KERN_INFO, "initializing apic\n");
    initialize_apic();
    printk(KERN_OK, "apic initialized\n");

    printk(KERN_INFO, "disabling pic\n");
    disable_pic();
    printk(KERN_OK, "pic disabled\n");

    // get physical addresses from MADT
    uint32_t lapic_phys = get_lapic_address();
    uint32_t ioapic_phys = get_ioapic_address();

    if (ioapic_phys == 0) {
        printk(KERN_FATAL, "ioapic not found\n");
        halt();
    }

    // map MMIO regions and set virtual (HHDM) base addresses
    map_physical_range(lapic_phys, 4096, 1, 1);
    uintptr_t lapic_virt = hhdm_offset + lapic_phys;
    apic_set_base(lapic_virt);
    enable_api_hardware();
    enable_apic_software();
    log_info("local APIC: phys=0x%x virt=0x%x%x\n",
            lapic_phys, (uint32_t)(lapic_virt >> 32), (uint32_t)lapic_virt);

    map_physical_range(ioapic_phys, 4096, 1, 1);
    ioapic_addr = (void *)(hhdm_offset + ioapic_phys);
    log_info("I/O APIC: phys=0x%x virt=0x%x%x\n",
            ioapic_phys, (uint32_t)((uintptr_t)ioapic_addr >> 32), (uint32_t)(uintptr_t)ioapic_addr);

    local_apic_id = get_local_apic_id();
    log_info("local APIC id: %d\n", (int)local_apic_id);

    // configure timer interrupt (IRQ 0 -> vector 32)
    // according to MADT: IRQ 0 is overriden to GSI 2
    uint32_t timer_gsi = 2; // madt override
    uint16_t timer_flags = 0x0; // madt itself
    printk(KERN_INFO, "configuring timer\n");
    configure_ioapic_irq_with_flags(timer_gsi, 32, local_apic_id, timer_flags);
    printk(KERN_OK, "timer interrupt configured via IOAPIC\n");
    printk(KERN_INFO, "timer initialization\n");
    init_timer();

    /* register drivers */
    ps2k_register();
    acpi_bus_enumerate();

    asm volatile("sti");
    printk(KERN_OK, "interrupts re-enabled\n");

    printk(KERN_INFO, "sleep test:\n");
    sleep(1);
    printk(KERN_OK, "sleep test done\n");

    if (ac97_init()) {
        printk(KERN_ERROR, "could not initialize ac97 driver\n");
        hcf();
    } else {
        printk(KERN_OK, "initialized AC97\n");
    }

    console_puts("\nHello, world!\n");
    console_puts("abcdefghijklmnopqrstuvwxyz0123456789\n");

    printk(KERN_DEBUG, "hello, world!\n");
    printk(KERN_OK, "hello, world!\n");
    printk(KERN_INFO, "hello, world!\n");
    printk(KERN_WARN, "hello, world!\n");
    printk(KERN_ERROR, "hello, world!\n");
    printk(KERN_FATAL, "goodbye, world!\n");

    extern struct key_event_t keyboard_multilevel_queue[KEYBOARD_QUEUE_LEVELS][RING_BUFFER_SIZE];
    extern struct keyboard_queue_state keyboard_queue_state[KEYBOARD_QUEUE_LEVELS];

    printk(KERN_DEBUG, "keyboard queue ringbuffer: %p\n", (volatile virt_addr_t) &keyboard_multilevel_queue);
    printk(KERN_DEBUG, "keyboard queue state: %p\n", (volatile virt_addr_t) &keyboard_queue_state);

    shell_init();
    printk(KERN_OK, "shell initialized\n");

    shell_run();

    for (;;)
        asm volatile("hlt");

    halt();
}
