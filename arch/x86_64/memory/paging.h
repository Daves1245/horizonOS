#ifndef PAGING_H
#define PAGING_H

#include <asm/mm.h>
#include <stdint.h>
#include <kernel/compiler.h>
#include <stddef.h>

// lwn.net find: https://lwn.net/Articles/106177/

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

#define PAGE_SIZE 4096UL // bytes per 4KB page

// Page table entry flags (same across all 4 levels)
#define PAGE_PRESENT (1UL << 0) // Page is in physical memory
#define PAGE_WRITE (1UL << 1) // Page is writable
#define PAGE_USER (1UL << 2) // Page accessible by user mode
#define PAGE_WRITE_THROUGH (1UL << 3) // Write-through caching
#define PAGE_CACHE_DISABLE (1UL << 4) // Disable caching
#define PAGE_ACCESSED (1UL << 5) // Set by CPU when accessed
#define PAGE_DIRTY (1UL << 6) // Set by CPU when written (PT only)
#define PAGE_PS (1UL << 7) // 2MB/1GB page (PD/PDPT only). the PS
// bit, named PAGE_SIZE until it got
// multiplied into KSTACK_SIZE by
// mistake. i386 calls it PDE_PAGE_SIZE.
#define PAGE_GLOBAL (1UL << 8) // Global page (not flushed on CR3 load)
#define PAGE_NX (1UL << 63) // No-execute bit

// CR4 bits we care about here
#define CR4_PGE (1UL << 7) // global page enable

// the page frame number (PFN) is found in bits
// 51-12, with the rest being zeroed from either flag
// bits or being reserved. remember, pages are 4KiB aligned.
// this is hex too, so those 3 zeros are actually twelve 0 bits.
#define PAGE_FRAME_MASK 0x000FFFFFFFFFF000UL

// Get physical address from page table entry
#define PAGE_GET_ADDR(entry) ((entry) & PAGE_FRAME_MASK)

// TODO(5-level paging) check and enable the la57 bit
// on cr4 (bit 12)
// from the intel handbook:
/*
 Software can thus use the following algorithm to enter IA-32e mode with 5-level
paging.
1. Use the MOV CR instruction to set CR4.PAE and CR4.LA57.
2. Use the WRMSR instruction to set IA32_EFER.LME.
3. Use the MOV CR instruction to load CR3 with the address of a PML5 table (see
Section 2.4).
4. Use the MOV CR instruction to set CR0.PG.
The processor allows software to modify CR4.LA57 only outside of IA-32e mode. In
IA-32e mode, an attempt to modify CR4.LA57 using the MOV CR instruction causes a
general-protection exception (#GP).
*/

/* Paging */

typedef uint64_t page_entry_t;
typedef uint64_t page_dir_entry_t;
typedef uint64_t page_upper_entry_t;
typedef uint64_t page_l4_dir_entry_t;
typedef uint64_t page_global_dir_entry_t;

/* Level 1 - Page table entry */
typedef struct {
	page_entry_t pte;
} pte_t;

// TODO(inlining): we might want to move these accessor functions
// to macros to force inlining instead of relying on the compiler.
// should be fine since we know all of these at compile time.
static inline page_entry_t pte_val(pte_t pte) {
	return pte.pte;
}
static inline page_entry_t *pte_ptr(pte_t *pte) {
	return &pte->pte;
}

typedef struct page_table_t {
	pte_t entries[512];
} pt_t __aligned(4096);

/* Level 2 - Page middle directory */
typedef uint64_t page_directory_t;

typedef struct {
	page_dir_entry_t pde;
} pde_t;

static inline page_directory_t pde_val(pde_t pde) {
	return pde.pde;
}
static inline page_directory_t *pde_ptr(pde_t *pde) {
	return &pde->pde;
}

typedef struct {
	pde_t entries[512];
} pd_t __aligned(4096);

/* Level 3 - Page upper directory */
typedef struct {
	page_upper_entry_t pue;
} pue_t;

typedef struct {
	pue_t entries[512];
} pud_t __attribute((aligned(4096)));

static inline page_upper_entry_t pue_val(pue_t pue) {
	return pue.pue;
}
static inline page_upper_entry_t *pue_ptr(pue_t *pue) {
	return &pue->pue;
}

/* Level 4 - Page 4 directory */
typedef struct {
	page_l4_dir_entry_t pml4e;
} p4e_t;

typedef struct {
	p4e_t entries[512];
} p4d_t __attribute((aligned(4096)));

static inline page_l4_dir_entry_t p4e_val(p4e_t p4e) {
	return p4e.pml4e;
}
static inline page_l4_dir_entry_t *p4e_ptr(p4e_t *p4e) {
	return &p4e->pml4e;
}

/* Level 5 - Global page directory */
typedef struct {
	page_global_dir_entry_t pge;
} pge_t;

typedef struct {
	pge_t entries[512];
} pgd_t __aligned(4096);

static inline page_global_dir_entry_t pge_val(pge_t pge) {
	return pge.pge;
}
static inline page_global_dir_entry_t *pge_ptr(pge_t *pge) {
	return &pge->pge;
}

// Extract indices from virtual address
#define PML4_INDEX(vaddr) (((vaddr) >> 39) & 0x1FF)
#define PDPT_INDEX(vaddr) (((vaddr) >> 30) & 0x1FF)
#define PD_INDEX(vaddr) (((vaddr) >> 21) & 0x1FF)
#define PT_INDEX(vaddr) (((vaddr) >> 12) & 0x1FF)

// Function declarations
void init_paging(void);
void map_physical_range(phys_addr_t phys_addr, uint32_t size, int iskernel,
			int writeable, uint64_t cr3);
pte_t *get_page_entry(virt_addr_t vaddr, int create, uint64_t cr3);
void map_page(virt_addr_t vaddr, phys_addr_t phys_addr, int iskernel,
	      int writeable, uint64_t cr3);
phys_addr_t unmap_page(virt_addr_t vaddr, uint64_t cr3);

// TLB management
static inline void invalidate_page(virt_addr_t vaddr) {
	asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static inline uint64_t read_cr3(void) {
	uint64_t cr3;

	asm volatile("mov %%cr3, %0" : "=r"(cr3));
	return cr3;
}

static inline uint64_t read_cr4(void) {
	uint64_t cr4;

	asm volatile("mov %%cr4, %0" : "=r"(cr4));
	return cr4;
}

// again - the memory clobber tells gcc to assume this
// instruction touches arbitrary memory, forcing it to flush
// all intermediate registers (in gcc, not the cpu). this avoids
// not setting cr3/cr4 between instructions, possibly leading
// to a fault or worse.
static inline void write_cr3(uint64_t cr3) {
	asm volatile("mov %0, %%cr3" ::"r"(cr3) : "memory");
}

static inline void write_cr4(uint64_t cr4) {
	asm volatile("mov %0, %%cr4" ::"r"(cr4) : "memory");
}

// invlpg drops the entry for a single page, which is no good after we have
// rewritten a whole 1 gb /2 mb translation. reloading cr3 flushes the rest,
// except for entries marked PAGE_GLOBAL, nd limine is free to mark the kernel
// and HHDM mappings global. clearing and restoring CR4.PGE is the way to drop
// those too (SDM Vol. 3A, 4.10.4.1).
static inline void flush_tlb_all(void) {
	uint64_t cr4 = read_cr4();

	if (cr4 & CR4_PGE) {
		write_cr4(cr4 & ~CR4_PGE);
		write_cr4(cr4);
		return;
	}

	write_cr3(read_cr3());
}

#endif
