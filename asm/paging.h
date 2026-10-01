/*
 * Paging contract. Both architectures must implement this as an interface/API.
 *
 * - PAGE_SIZE           bytes per smallest page
 * - PAGE_GET_ADDR()    physical frame address held in an entry
 * - pgtable_t           top-level page table that cr3 points at
 * - read_cr3()          current cr3 value
 * - invalidate_page()   drop a TLB entry
 * - map_page()          map one page into the table rooted at cr3
 * - map_range()         map a physical range of pages, rooted at cr3
 * - unmap_page()        unmap one page, and hand the frame back to the caller
 *                       (0/null if nothing was mapped). do not free the frame.
 *
 * cr3 is passed around as a phys_addr_t, which is the register's width on
 * both archs. The declarations below repeat each arch's own in arch-neutral
 * types, so an arch that drifts from the contract fails to compile here
 * instead of in whichever file first uses the drifting piece.
 */

#ifndef ASM_PAGING_H
#define ASM_PAGING_H

#ifdef __x86_64__
#include <x86_64/memory/paging.h>
#else
#include <i386/memory/paging.h>
#endif

static inline phys_addr_t read_cr3(void);
static inline void invalidate_page(virt_addr_t vaddr);
void map_page(virt_addr_t vaddr, phys_addr_t paddr, int iskernel,
	      int writeable, phys_addr_t cr3);
phys_addr_t unmap_page(virt_addr_t vaddr, phys_addr_t cr3);
void map_physical_range(phys_addr_t phys_start, uint32_t length, int iskernel, int writeable, uint32_t cr3);


#endif
