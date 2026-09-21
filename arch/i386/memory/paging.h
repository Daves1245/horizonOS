#ifndef PAGING_H
#define PAGING_H

#include "interrupts/isr.h"
#include <kernel/compiler.h>
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
#define PDE_PRESENT \
	(1          \
	 << 0) // if set, page is in physical memory. page fault occurs if not present
#define PDE_READ_WRITE \
	(1             \
	 << 1) // if set, page is read/write. otherwise read-only. WP bit in CR0 affects kernel access
#define PDE_USER_SUPERVISOR \
	(1                  \
	 << 2) // if set, page accessible by all. otherwise supervisor only. must be set for user pages
#define PDE_WRITE_THROUGH \
	(1                \
	 << 3) // if set, write-through caching enabled. otherwise write-back caching
#define PDE_CACHE_DISABLE (1 << 4) // if set, page will not be cached
#define PDE_ACCESSED \
	(1           \
	 << 5) // set by cpu when PDE is read during address translation. OS must clear
#define PDE_DIRTY \
	(1        \
	 << 6) // set by cpu when page is written to. used to determine if page needs writeback
#define PDE_PAGE_SIZE \
	(1            \
	 << 7) // if set, PDE maps 4MB page. otherwise maps 4KB page table. requires PSE enabled
#define PDE_GLOBAL \
	(1         \
	 << 8) // if set, TLB entry not invalidated on CR3 load. requires PGE bit in CR4
#define PDE_AVAILABLE_1 \
	(1 << 9) // available for OS use - not used by processor.
#define PDE_AVAILABLE_2 \
	(1 << 10) // available for OS use - not used by processor.
#define PDE_AVAILABLE_3 \
	(1 << 11) // available for OS use - not used by processor.

// page table base address mask (bits 31-12)
#define PDE_PAGE_TABLE_BASE_MASK 0xFFFFF000

// cr0 control register bits
#define CR0_PG (1 << 31) // paging enable bit

// page table entry bit masks (same as PDE)
#define PTE_PRESENT (1 << 0)
#define PTE_READ_WRITE (1 << 1)
#define PTE_USER_SUPERVISOR (1 << 2)
#define PTE_WRITE_THROUGH (1 << 3)
#define PTE_CACHE_DISABLE (1 << 4)
#define PTE_ACCESSED (1 << 5)
#define PTE_DIRTY (1 << 6)
#define PTE_PAGE_ATTRIBUTE (1 << 7)
#define PTE_GLOBAL (1 << 8)
#define PTE_FRAME_MASK 0xFFFFF000

typedef uint32_t __page_directory_t;
typedef uint32_t __page_table_entry_t;

typedef uint32_t __page_dir_entry_t;

typedef struct {
	__page_table_entry_t pte;
} pte_t;

// TODO(inlining): like in x86_64, we might want to move
// these accessor functions into macros in order to
// force inlining instead of relying on the compiler.
static inline __page_table_entry_t pte_val(pte_t pte) {
	return pte.pte;
}
static inline __page_table_entry_t *pte_ptr(pte_t *pte) {
	return &pte->pte;
}

typedef struct {
	__page_dir_entry_t pde;
} pde_t;

static inline __page_dir_entry_t pde_val(pde_t pde) {
	return pde.pde;
}
static inline __page_dir_entry_t *pde_ptr(pde_t *pde) {
	return &pde->pde;
}

// non-PAE i386: both tables hold 1024 4-byte entries (4KiB)
typedef struct page_table_t {
	pte_t entries[1024];
} pt_t __aligned(4096);

typedef struct page_directory_t {
	pde_t entries[1024];
} pd_t __aligned(4096);

typedef uint32_t __page_upper_entry_t;

typedef struct {
	__page_upper_entry_t pue;
} pue_t;

static inline __page_upper_entry_t pue_val(pue_t pue) {
	return pue.pue;
}

static inline __page_upper_entry_t *pue_ptr(pue_t *pue) {
	return &pue->pue;
}

typedef struct {
	pue_t entries[512];
} pud_t __aligned(4096);

// macro helpers for page table entry manipulation
#define PTE_IS_PRESENT(pte) ((pte) & PTE_PRESENT)
#define PTE_SET_PRESENT(pte) ((pte) |= PTE_PRESENT)
#define PTE_CLEAR_PRESENT(pte) ((pte) &= ~PTE_PRESENT)

#define PTE_IS_WRITABLE(pte) ((pte) & PTE_READ_WRITE)
#define PTE_SET_WRITABLE(pte) ((pte) |= PTE_READ_WRITE)
#define PTE_CLEAR_WRITABLE(pte) ((pte) &= ~PTE_READ_WRITE)

#define PTE_IS_USER(pte) ((pte) & PTE_USER_SUPERVISOR)
#define PTE_SET_USER(pte) ((pte) |= PTE_USER_SUPERVISOR)
#define PTE_CLEAR_USER(pte) ((pte) &= ~PTE_USER_SUPERVISOR)

#define PTE_GET_FRAME(pte) (((pte) & PTE_FRAME_MASK) >> 12)
#define PTE_SET_FRAME(pte, frame) \
	((pte) = ((pte) & ~PTE_FRAME_MASK) | (((frame) & 0xFFFFF) << 12))

// global variables
extern pd_t *kernel_directory;
extern pd_t *current_directory;

// function declarations
void init_paging(void);
void switch_page_directory(pd_t *new_pd);
pte_t *get_page(uint32_t addr, int make, uint32_t cr3);
void page_fault(struct interrupt_context *regs);
void alloc_frame(pte_t *page, int iskernel, int writeable);
void free_frame(pte_t *page);
void map_physical_range(uint32_t phys_start, uint32_t length, int iskernel,
			int writeable, uint32_t cr3);

// Virtual memory API
void map_page(uint32_t virt_addr, uint32_t phys_addr, int iskernel,
	      int writeable, uint32_t cr3);
void unmap_page(uint32_t virt_addr, uint32_t cr3);
int is_page_mapped(uint32_t virt_addr, uint32_t cr3);

// TLB management
static inline void invalidate_page(uint32_t virt_addr) {
	asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

static inline uint32_t read_cr3() {
	uint32_t cr3;
	asm volatile("mov %%cr3, %0" : "=r"(cr3));
	return cr3;
}

static inline pd_t *cr3_to_directory(uint32_t cr3) {
	return (pd_t *) (cr3 & PDE_PAGE_TABLE_BASE_MASK);
}

#endif
