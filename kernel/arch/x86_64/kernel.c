#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include <kernel/tty.h>
#include <common.h>

#ifdef __x86_64__
#include "../arch/x86_64/interrupts/descriptor_tables.h"
#include "../arch/x86_64/memory/paging.h"
#else
#include "../arch/i386/interrupts/descriptor_tables.h"
#include "../arch/i386/memory/paging.h"
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
static volatile struct limine_paging_mode_request rsdp_request = {
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

    // printf("Initializing descriptor tables...\n");
    //init_descriptor_tables();
    // printf("Descriptor tables initialized\n");

    //printf("Limine base revision supported\n");

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

    // initialize paging
    //printf("initializing paging...\n");
    //init_paging();
    //printf("paging enabled!\n");

    // test for APIC support
    // halt_without_apic();

    //printf("APIC supported\n");
    //printf("check_msr (apic base msr): %d", check_msr());

    //printf("\ntesting paging...\n");

    // paging test 1: access kernel memory (should work)
    //volatile int *kernel_mem = (volatile int *) 0x100000;  // 1MB mark
    //*kernel_mem = 0x12345678;
    //printf("write to kernel memory: 0x%x\n", *kernel_mem);

    // test 2: verify we can read back the value
    /*
    if (*kernel_mem == 0x12345678) {
        printf("paging test PASSED: memory read/write working\n");
    } else {
        printf("paging test FAILED: got 0x%x, expected 0x12345678\n", *kernel_mem);
    }
    */

    // test 3: test another mapped location within kernel space
    //printf("\ntesting another mapped location...\n");
    //volatile int *another_kernel_mem = (volatile int *) (placement_address - 0x1000);
    //*another_kernel_mem = 0xDEADBEEF;
    //if (*another_kernel_mem == 0xDEADBEEF) {
        //printf("paging test PASSED: Second memory location OK (0x%lx)\n", (uintptr_t)another_kernel_mem);
    //} else {
        //printf("paging test FAILED: Second memory location write failed\n");
    //}

    // test 4: display memory mapping info
    //printf("\nmemory mapping info:\n");
    //printf("  kernel start: 0x100000 (1MB) \n");
//#ifdef __x86_64__
    //printf("  kernel end: 0x%lx\n", (uint64_t) &kernel_end);
    //printf("  placement address: 0x%lx\n", placement_address);
    //printf("  identity mapped up to: 0x%lx\n", placement_address + 0x1000);
//#else
    //printf("  kernel end: 0x%x\n", (u32int) &kernel_end);
    //printf("  placement address: 0x%x\n", placement_address);
    //printf("  identity mapped up to: 0x%x\n", placement_address + 0x1000);
//#endif

    // test 5: page fault test (uncomment to test page fault handler)
    // printf("\ntesting page fault handler...\n");
    // volatile int *unmapped = (volatile int *)0xA0000000;  // high unmapped address
    // *unmapped = 42;  // this should trigger a page fault

    //printf("\nall paging tests passed\n");

    hcf();
}
