#ifndef PAGING_H
#define PAGING_H

#include "interrupts/isr.h"
#include <stdint.h>

/*
 * x86-32 Page Directory Structure
 *
 * the page directory is the top-level structure in x86-32 paging. it contains
 * 1024 page directory entries (PDEs), each covering 4mb of virtual address space.
 * the page directory is loaded into CR3 and must be 4kb-aligned.
 *
 * each PDE can either:
 * - point to a 4kb page table (PS=0) containing 1024 page table entries
 * - point directly to a 4mb page (PS=1) if PSE is enabled in CR4
 *
 * virtual address translation:
 * bits 11-0:  offset within 4KB page
 * bits 21-12: index into page table (1024 entries, if PS=0)
 * bits 31-22: index into page directory (1024 entries)
 */

// page directory entry bit masks
#define PDE_PRESENT          (1 << 0)   // if set, page is in physical memory. page fault occurs if not present
#define PDE_READ_WRITE       (1 << 1)   // if set, page is read/write. otherwise read-only. WP bit in CR0 affects kernel access
#define PDE_USER_SUPERVISOR  (1 << 2)   // if set, page accessible by all. otherwise supervisor only. must be set for user pages
#define PDE_WRITE_THROUGH    (1 << 3)   // if set, write-through caching enabled. otherwise write-back caching
#define PDE_CACHE_DISABLE    (1 << 4)   // if set, page will not be cached
#define PDE_ACCESSED         (1 << 5)   // set by cpu when PDE is read during address translation. OS must clear
#define PDE_DIRTY            (1 << 6)   // set by cpu when page is written to. used to determine if page needs writeback
#define PDE_PAGE_SIZE        (1 << 7)   // if set, PDE maps 4MB page. otherwise maps 4KB page table. requires PSE enabled
#define PDE_GLOBAL           (1 << 8)   // if set, TLB entry not invalidated on CR3 load. requires PGE bit in CR4
#define PDE_AVAILABLE_1      (1 << 9)   // available for OS use - not used by processor.
#define PDE_AVAILABLE_2      (1 << 10)  // available for OS use - not used by processor.
#define PDE_AVAILABLE_3      (1 << 11)  // available for OS use - not used by processor.

// page table base address mask (bits 31-12)
#define PDE_PAGE_TABLE_BASE_MASK    0xFFFFF000

// cr0 control register bits
#define CR0_PG                      (1 << 31)  // paging enable bit

// page table entry bit masks (same as PDE)
#define PTE_PRESENT          (1 << 0)
#define PTE_READ_WRITE       (1 << 1)
#define PTE_USER_SUPERVISOR  (1 << 2)
#define PTE_WRITE_THROUGH    (1 << 3)
#define PTE_CACHE_DISABLE    (1 << 4)
#define PTE_ACCESSED         (1 << 5)
#define PTE_DIRTY            (1 << 6)
#define PTE_PAGE_ATTRIBUTE   (1 << 7)
#define PTE_GLOBAL           (1 << 8)
#define PTE_FRAME_MASK       0xFFFFF000

// pimple typedefs for page directory and page table entries
typedef uint32_t page_directory_t;
typedef uint32_t page_table_entry_t;

// macro helpers for page table entry manipulation
#define PTE_IS_PRESENT(pte)       ((pte) & PTE_PRESENT)
#define PTE_SET_PRESENT(pte)      ((pte) |= PTE_PRESENT)
#define PTE_CLEAR_PRESENT(pte)    ((pte) &= ~PTE_PRESENT)

#define PTE_IS_WRITABLE(pte)      ((pte) & PTE_READ_WRITE)
#define PTE_SET_WRITABLE(pte)     ((pte) |= PTE_READ_WRITE)
#define PTE_CLEAR_WRITABLE(pte)   ((pte) &= ~PTE_READ_WRITE)

#define PTE_IS_USER(pte)          ((pte) & PTE_USER_SUPERVISOR)
#define PTE_SET_USER(pte)         ((pte) |= PTE_USER_SUPERVISOR)
#define PTE_CLEAR_USER(pte)       ((pte) &= ~PTE_USER_SUPERVISOR)

#define PTE_GET_FRAME(pte)        (((pte) & PTE_FRAME_MASK) >> 12)
#define PTE_SET_FRAME(pte, frame) ((pte) = ((pte) & ~PTE_FRAME_MASK) | (((frame) & 0xFFFFF) << 12))

// global variables
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;

// function declarations
void init_paging(void);
void switch_page_directory(page_directory_t *new_pd);
page_table_entry_t *get_page(uint32_t addr, int make, page_directory_t *dir);
void page_fault(struct interrupt_context *regs);
void alloc_frame(page_table_entry_t *page, int iskernel, int writeable);
void free_frame(page_table_entry_t *page);
void map_physical_range(uint32_t phys_start, uint32_t length, int iskernel, int writeable);

#endif
