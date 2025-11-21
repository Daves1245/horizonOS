#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/*
 * x86-64 4-Level Paging Structure
 *
 * x86-64 uses 4-level paging with 48-bit virtual addresses:
 * - PML4 (Page Map Level 4) - 512 entries, top level
 * - PDPT (Page Directory Pointer Table) - 512 entries per PML4 entry
 * - PD (Page Directory) - 512 entries per PDPT entry
 * - PT (Page Table) - 512 entries per PD entry
 *
 * Virtual address breakdown (48-bit):
 * bits 47-39: PML4 index (9 bits, 512 entries)
 * bits 38-30: PDPT index (9 bits, 512 entries)
 * bits 29-21: PD index (9 bits, 512 entries)
 * bits 20-12: PT index (9 bits, 512 entries)
 * bits 11-0:  offset within 4KB page
 */

// Page table entry flags (same across all 4 levels)
#define PAGE_PRESENT        (1UL << 0)   // Page is in physical memory
#define PAGE_WRITE          (1UL << 1)   // Page is writable
#define PAGE_USER           (1UL << 2)   // Page accessible by user mode
#define PAGE_WRITE_THROUGH  (1UL << 3)   // Write-through caching
#define PAGE_CACHE_DISABLE  (1UL << 4)   // Disable caching
#define PAGE_ACCESSED       (1UL << 5)   // Set by CPU when accessed
#define PAGE_DIRTY          (1UL << 6)   // Set by CPU when written (PT only)
#define PAGE_SIZE           (1UL << 7)   // 2MB/1GB page (PD/PDPT only)
#define PAGE_GLOBAL         (1UL << 8)   // Global page (not flushed on CR3 load)
#define PAGE_NX             (1UL << 63)  // No-execute bit

// Physical address mask for 4KB pages (bits 51-12)
#define PAGE_FRAME_MASK     0x000FFFFFFFFFF000UL

// Get physical address from page table entry
#define PAGE_GET_ADDR(entry) ((entry) & PAGE_FRAME_MASK)

// Page table entry type (all levels use same 64-bit format)
typedef uint64_t page_entry_t;

// Page table structures (512 entries each, 4KB total)
typedef struct {
    page_entry_t entries[512];
} __attribute__((aligned(4096))) page_table_t;

// For clarity in code
typedef page_table_t pml4_t;    // Page Map Level 4
typedef page_table_t pdpt_t;    // Page Directory Pointer Table
typedef page_table_t pd_t;      // Page Directory
typedef page_table_t pt_t;      // Page Table

// Extract indices from virtual address
#define PML4_INDEX(vaddr)  (((vaddr) >> 39) & 0x1FF)
#define PDPT_INDEX(vaddr)  (((vaddr) >> 30) & 0x1FF)
#define PD_INDEX(vaddr)    (((vaddr) >> 21) & 0x1FF)
#define PT_INDEX(vaddr)    (((vaddr) >> 12) & 0x1FF)

// Function declarations
void init_paging(void);
void map_physical_range(uint64_t phys_addr, uint32_t size, int iskernel, int writeable);
page_entry_t *get_page_entry(uint64_t vaddr, int create);
void map_page(uint64_t vaddr, uint64_t paddr, int iskernel, int writeable);

// TLB management
static inline void invalidate_page(uint64_t vaddr) {
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static inline uint64_t read_cr3(void) {
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline void write_cr3(uint64_t cr3) {
    asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

#endif
