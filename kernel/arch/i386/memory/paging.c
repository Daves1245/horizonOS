#include "memory/paging.h"
#include "interrupts/isr.h"
#include <string.h>
#include <stdio.h>
#include "../../kernel/kheap.h"

uint32_t *frames;
uint32_t nframes;

// defined in kheap.c
extern uint32_t placement_address;

// global bage direbtories
page_directory_t *kernel_directory;
page_directory_t *current_directory;

#define BITSET_INDEX(a) ((a) / (8 * 4))
#define BITSET_OFFSET(a) ((a) % (8 * 4))

// set a bit
static void set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / 0x1000;
    uint32_t idx = BITSET_INDEX(frame);
    uint32_t off = BITSET_OFFSET(frame);
    frames[idx] |= (1 << off);
}

// clear a bit
static void clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / 0x1000;
    uint32_t idx = BITSET_INDEX(frame);
    uint32_t off = BITSET_OFFSET(frame);
    frames[idx] &= ~(1 << off);
}

static uint32_t test_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / 0x1000;
    uint32_t idx = BITSET_INDEX(frame);
    uint32_t off = BITSET_OFFSET(frame);
    return frames[idx] & (1 << off);
}

static uint32_t first_frame() {
    uint32_t i, j;
    for (i = 0; i < BITSET_INDEX(nframes); i++) {
        if (frames[i] == 0xFFFFFFFF) continue;
        for (j = 0; j < 32; j++) {
            uint32_t mask = 1 << j;
            if (!(frames[i] & mask)) return i * 32 + j;
        }
    }

    // no free frames found
    return (uint32_t) -1;
}

void alloc_frame(page_table_entry_t *page, int iskernel, int writeable) {
    if (PTE_GET_FRAME(*page) != 0) return; // already allocated
    uint32_t idx = first_frame();
    if (idx == (uint32_t) - 1) {
        printf("kernel panic: No free frames\n");
        asm volatile("cli; hlt");
        while (1);
    }

    set_frame(idx * 0x1000);
    PTE_SET_PRESENT(*page);
    if (writeable) PTE_SET_WRITABLE(*page);
    if (!iskernel) PTE_SET_USER(*page);
    PTE_SET_FRAME(*page, idx);
}

void free_frame(page_table_entry_t *page) {
    uint32_t frame = PTE_GET_FRAME(*page);
    if (!frame) return;
    clear_frame(frame * 0x1000);
    *page = 0; // clear entire page table entry
}

void init_paging() {
    // assumes 64MB memory
    uint32_t mem_end_page = 0x4000000;
    nframes = mem_end_page / 0x1000;
    frames = (uint32_t *) kmalloc(BITSET_INDEX(nframes) * sizeof(uint32_t));
    memset(frames, 0, BITSET_INDEX(nframes) * sizeof(uint32_t));

    // Allocate page directory (1024 entries * 4 bytes each = 4KB, page-aligned)
    kernel_directory = (page_directory_t *) kmalloc_a(1024 * sizeof(page_directory_t));
    memset(kernel_directory, 0, 1024 * sizeof(page_directory_t));
    current_directory = kernel_directory;

    // Identity map from 0x0 to the end of used memory
    // Map physical address X to virtual address X for kernel memory
    uint32_t i = 0;
    while (i < placement_address + 0x1000) {
        page_table_entry_t *page = get_page(i, 1, kernel_directory);
        alloc_frame(page, 1, 1); // kernel=1, writeable=1
        i += 0x1000;
    }

    // Register page fault handler
    register_interrupt_handler(14, page_fault);

    switch_page_directory(kernel_directory);
}

void switch_page_directory(page_directory_t *dir) {
    current_directory = dir;

    // load the page directory physical address into cr3
    uint32_t phys_addr = (uint32_t)dir;
    asm volatile("movl %0, %%cr3" : : "r"(phys_addr));

    // enable paging by setting the pg bit in cr0
    uint32_t cr0;
    asm volatile("movl %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_PG;
    asm volatile("movl %0, %%cr0" : : "r"(cr0));
}

page_table_entry_t *get_page(uint32_t addr, int make, page_directory_t *dir) {
    // extract page directory index (bits 31-22)
    uint32_t page_dir_index = addr >> 22;

    // get the page directory entry
    uint32_t pde = dir[page_dir_index];

    // check if page table exists
    if (!(pde & PDE_PRESENT)) {
        if (!make) {
            return 0; // page table doesn't exist and we're not creating it
        }

        // allocate a new page table (4KB, page-aligned)
        uint32_t page_table_phys = (uint32_t) kmalloc_a(4096);
        memset((void*)page_table_phys, 0, 4096);

        // set up the page directory entry
        dir[page_dir_index] = page_table_phys | PDE_PRESENT | PDE_READ_WRITE;
        // note: PDE_USER_SUPERVISOR should be set based on the page's intended use,
        // not the make parameter. For now, kernel pages don't set this bit.
    }

    // get the page table physical address
    uint32_t page_table_phys = dir[page_dir_index] & PDE_PAGE_TABLE_BASE_MASK;
    page_table_entry_t *page_table = (page_table_entry_t*)page_table_phys;

    // extract page table index (bits 21-12)
    uint32_t page_table_index = (addr >> 12) & 0x3FF;

    // return pointer to the page table entry
    return &page_table[page_table_index];
}

void page_fault(struct interrupt_context *regs) {
    // Get the faulting address from CR2
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    int present = regs->err_code & 0x1;    // Page not present
    int rw = regs->err_code & 0x2;         // Write operation?
    int us = regs->err_code & 0x4;         // User mode?
    int reserved = regs->err_code & 0x8;   // Reserved bits set?
    int id = regs->err_code & 0x10;        // Instruction fetch?

    // print error information
    printf("\nPage fault! ( ");
    if (!present) printf("not present ");
    if (rw) printf("read-only ");
    if (us) printf("user-mode ");
    if (reserved) printf("reserved ");
    if (id) printf("instruction-fetch ");
    printf(") at 0x%x\n", faulting_address);

    printf("EIP: 0x%x\n", regs->eip);

    printf("System halted.\n");
    asm volatile("cli; hlt");
}

void map_physical_range(uint32_t phys_start, uint32_t length, int iskernel, int writeable) {
    // Align start address to page boundary
    uint32_t start = phys_start & 0xFFFFF000;
    // Align end address to page boundary (round up)
    uint32_t end = (phys_start + length + 0xFFF) & 0xFFFFF000;

    printf("Mapping physical range 0x%x to 0x%x (length: %d bytes)\n", start, end, end - start);

    // Identity map each page in the range
    for (uint32_t addr = start; addr < end; addr += 0x1000) {
        page_table_entry_t *page = get_page(addr, 1, kernel_directory);
        if (page && !PTE_IS_PRESENT(*page)) {
            // Map virtual address to same physical address (identity mapping)
            uint32_t frame = addr / 0x1000;
            PTE_SET_PRESENT(*page);
            if (writeable) PTE_SET_WRITABLE(*page);
            if (!iskernel) PTE_SET_USER(*page);
            PTE_SET_FRAME(*page, frame);

            // Mark frame as used in our frame bitmap
            set_frame(addr);
        }
    }
}
