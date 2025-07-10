#include "../include/common.h"
#include "paging.h"
#include "../interrupts/isr.h"
#include <string.h>
#include <stdio.h>
#include "../../../kernel/kheap.h"

u32int *frames;
u32int nframes;

// Defined in kheap.c
extern u32int placement_address;

// Global page directories
page_directory_t *kernel_directory;
page_directory_t *current_directory;

#define BITSET_INDEX(a) ((a) / (8 * 4))
#define BITSET_OFFSET(a) ((a) % (8 * 4))

// set a bit
static void set_frame(u32int frame_addr) {
    u32int frame = frame_addr / 0x1000;
    u32int idx = BITSET_INDEX(frame);
    u32int off = BITSET_OFFSET(frame);
    frames[idx] |= (1 << off);
}

// clear a bit
static void clear_frame(u32int frame_addr) {
    u32int frame = frame_addr / 0x1000;
    u32int idx = BITSET_INDEX(frame);
    u32int off = BITSET_OFFSET(frame);
    frames[idx] &= ~(1 << off);
}

static u32int test_frame(u32int frame_addr) {
    u32int frame = frame_addr / 0x1000;
    u32int idx = BITSET_INDEX(frame);
    u32int off = BITSET_OFFSET(frame);
    return frames[idx] & (1 << off);
}

static u32int first_frame() {
    u32int i, j;
    for (i = 0; i < BITSET_INDEX(nframes); i++) {
        if (frames[i] == 0xFFFFFFFF) continue;
        for (j = 0; j < 32; j++) {
            u32int mask = 1 << j;
            if (!(frames[i] & mask)) return i * 32 + j;
        }
    }
    return (u32int)-1; // No free frames found
}

void alloc_frame(page_table_entry_t *page, int iskernel, int writeable) {
    if (PTE_GET_FRAME(*page) != 0) return; // already allocated
    u32int idx = first_frame();
    if (idx == (u32int) - 1) {
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
    u32int frame = PTE_GET_FRAME(*page);
    if (!frame) return;
    clear_frame(frame * 0x1000);
    *page = 0; // clear entire page table entry
}

void init_paging() {
    // assumes 16MB memory
    u32int mem_end_page = 0x1000000;
    nframes = mem_end_page / 0x1000;
    frames = (u32int *) kmalloc(BITSET_INDEX(nframes) * sizeof(u32int));
    memset(frames, 0, BITSET_INDEX(nframes) * sizeof(u32int));

    // Allocate page directory (1024 entries * 4 bytes each = 4KB, page-aligned)
    kernel_directory = (page_directory_t *) kmalloc_a(1024 * sizeof(page_directory_t));
    memset(kernel_directory, 0, 1024 * sizeof(page_directory_t));
    current_directory = kernel_directory;

    // Identity map from 0x0 to the end of used memory
    // Map physical address X to virtual address X for kernel memory
    u32int i = 0;
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
    u32int phys_addr = (u32int)dir;
    asm volatile("movl %0, %%cr3" : : "r"(phys_addr));
    
    // enable paging by setting the pg bit in cr0
    u32int cr0;
    asm volatile("movl %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_PG;
    asm volatile("movl %0, %%cr0" : : "r"(cr0));
}

page_table_entry_t *get_page(u32int addr, int make, page_directory_t *dir) {
    // Extract page directory index (bits 31-22)
    u32int page_dir_index = addr >> 22;
    
    // Get the page directory entry
    u32int pde = dir[page_dir_index];
    
    // Check if page table exists
    if (!(pde & PDE_PRESENT)) {
        if (!make) {
            return 0; // Page table doesn't exist and we're not creating it
        }
        
        // Allocate a new page table (4KB, page-aligned)
        u32int page_table_phys = (u32int) kmalloc_a(4096);
        memset((void*)page_table_phys, 0, 4096);
        
        // Set up the page directory entry
        dir[page_dir_index] = page_table_phys | PDE_PRESENT | PDE_READ_WRITE;
        // Note: PDE_USER_SUPERVISOR should be set based on the page's intended use,
        // not the make parameter. For now, kernel pages don't set this bit.
    }
    
    // Get the page table physical address
    u32int page_table_phys = dir[page_dir_index] & PDE_PAGE_TABLE_BASE_MASK;
    page_table_entry_t *page_table = (page_table_entry_t*)page_table_phys;
    
    // Extract page table index (bits 21-12)
    u32int page_table_index = (addr >> 12) & 0x3FF;
    
    // Return pointer to the page table entry
    return &page_table[page_table_index];
}

void page_fault(struct interrupt_context *regs) {
    // Get the faulting address from CR2
    u32int faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    // Decode the error code
    int present = regs->err_code & 0x1;    // Page not present
    int rw = regs->err_code & 0x2;         // Write operation?
    int us = regs->err_code & 0x4;         // User mode?
    int reserved = regs->err_code & 0x8;   // Reserved bits set?
    int id = regs->err_code & 0x10;        // Instruction fetch?
    
    // Print error information
    printf("\nPage fault! ( ");
    if (!present) printf("not present ");
    if (rw) printf("read-only ");
    if (us) printf("user-mode ");
    if (reserved) printf("reserved ");
    if (id) printf("instruction-fetch ");
    printf(") at 0x%x\n", faulting_address);
    
    printf("EIP: 0x%x\n", regs->eip);
    
    // For now, halt the system
    printf("System halted.\n");
    asm volatile("cli; hlt");
}
