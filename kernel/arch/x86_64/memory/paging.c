#include "paging.h"
#include "../../kernel/kheap.h"
#include <string.h>
#include <drivers/serial.h>
#include <kernel/logger.h>
#include <limine.h>

// We'll use Limine's page tables initially, then can create our own if needed
// Limine provides the HHDM which we need to access physical memory
extern struct limine_hhdm_request hhdm_request;

// Get virtual address from physical address using HHDM
static inline void* phys_to_virt(uint64_t phys) {
    return (void*)(phys + hhdm_request.response->offset);
}

// Get physical address from virtual address using HHDM
static inline uint64_t virt_to_phys(void* virt) {
    uint64_t addr = (uint64_t)virt;
    uint64_t hhdm_offset = hhdm_request.response->offset;

    // If address is in HHDM range, subtract offset
    if (addr >= hhdm_offset) {
        return addr - hhdm_offset;
    }

    // If address is in kernel range (0xffffffff80000000+), convert to physical
    if (addr >= 0xffffffff80000000UL) {
        return addr - 0xffffffff80000000UL;
    }

    // Otherwise return as-is (shouldn't happen)
    return addr;
}

void init_paging(void) {
    // Limine already set up paging for us, nothing to do here
    // We're using base revision 3, so we'll manually map what we need
    log_debug("[paging]: Using Limine's page tables (4-level paging)\n");

    uint64_t cr3 = read_cr3();
    log_debug("[paging]: CR3 = 0x%x%x\n", (uint32_t)(cr3 >> 32), (uint32_t)cr3);
}

/**
 * Get or create a page table entry for a virtual address
 *
 * @param vaddr Virtual address
 * @param create If 1, create intermediate tables if they don't exist
 * @return Pointer to the page table entry, or NULL if not found and create=0
 */
page_entry_t *get_page_entry(uint64_t vaddr, int create) {
    uint64_t cr3 = read_cr3();
    uint64_t pml4_phys = cr3 & PAGE_FRAME_MASK;
    pml4_t *pml4 = (pml4_t*)phys_to_virt(pml4_phys);

    uint64_t pml4_idx = PML4_INDEX(vaddr);
    page_entry_t *pml4e = &pml4->entries[pml4_idx];

    pdpt_t *pdpt;
    if (!(*pml4e & PAGE_PRESENT)) {
        if (!create) return NULL;

        pdpt = (pdpt_t*)kmalloc_a(sizeof(pdpt_t));
        memset(pdpt, 0, sizeof(pdpt_t));

        uint64_t pdpt_phys = virt_to_phys(pdpt);
        *pml4e = pdpt_phys | PAGE_PRESENT | PAGE_WRITE;
    } else {
        uint64_t pdpt_phys = PAGE_GET_ADDR(*pml4e);
        pdpt = (pdpt_t*)phys_to_virt(pdpt_phys);
    }

    uint64_t pdpt_idx = PDPT_INDEX(vaddr);
    page_entry_t *pdpte = &pdpt->entries[pdpt_idx];

    pd_t *pd;
    if (!(*pdpte & PAGE_PRESENT)) {
        if (!create) return NULL;

        pd = (pd_t*)kmalloc_a(sizeof(pd_t));
        memset(pd, 0, sizeof(pd_t));

        uint64_t pd_phys = virt_to_phys(pd);
        *pdpte = pd_phys | PAGE_PRESENT | PAGE_WRITE;
    } else {
        uint64_t pd_phys = PAGE_GET_ADDR(*pdpte);
        pd = (pd_t*)phys_to_virt(pd_phys);
    }

    uint64_t pd_idx = PD_INDEX(vaddr);
    page_entry_t *pde = &pd->entries[pd_idx];

    pt_t *pt;
    if (!(*pde & PAGE_PRESENT)) {
        if (!create) return NULL;

        pt = (pt_t *) kmalloc_a(sizeof(pt_t));
        memset(pt, 0, sizeof(pt_t));

        uint64_t pt_phys = virt_to_phys(pt);
        *pde = pt_phys | PAGE_PRESENT | PAGE_WRITE;
    } else {
        uint64_t pt_phys = PAGE_GET_ADDR(*pde);
        pt = (pt_t*) phys_to_virt(pt_phys);
    }

    // Get PT entry
    uint64_t pt_idx = PT_INDEX(vaddr);
    return &pt->entries[pt_idx];
}

/**
 * Map a virtual page to a physical page
 *
 * @param vaddr Virtual address (will be page-aligned)
 * @param paddr Physical address (will be page-aligned)
 * @param iskernel 1 for kernel pages, 0 for user pages
 * @param writeable 1 for writable pages, 0 for read-only
 */
void map_page(uint64_t vaddr, uint64_t paddr, int iskernel, int writeable) {
    // Align addresses to page boundaries
    vaddr &= ~0xFFFUL;
    paddr &= ~0xFFFUL;

    serial_write("C");
    // Get or create page table entry
    page_entry_t *pte = get_page_entry(vaddr, 1);
    serial_write("D");
    if (!pte) {
        log_error("[paging]: Failed to get page entry for vaddr 0x%x%x\n",
                  (uint32_t)(vaddr >> 32), (uint32_t)vaddr);
        return;
    }

    // Set up the page table entry
    *pte = paddr | PAGE_PRESENT;

    if (writeable) {
        *pte |= PAGE_WRITE;
    }

    if (!iskernel) {
        *pte |= PAGE_USER;
    }

    // Flush TLB for this page
    invalidate_page(vaddr);
}

/**
 * Map a range of physical memory to virtual memory using HHDM offset
 * This is used to map ACPI tables and other reserved regions
 *
 * @param phys_addr Physical address to start mapping
 * @param size Size in bytes
 * @param iskernel 1 for kernel pages, 0 for user pages
 * @param writeable 1 for writable pages, 0 for read-only
 */
void map_physical_range(uint64_t phys_addr, uint32_t size, int iskernel, int writeable) {
    // Align to page boundaries
    uint64_t start = phys_addr & ~0xFFFUL;
    uint64_t end = (phys_addr + size + 0xFFF) & ~0xFFFUL;

    serial_write("[paging]: Mapping physical range 0x");
    log_info("%x%x", (uint32_t)(start >> 32), (uint32_t)start);
    serial_write(" to 0x");
    log_info("%x%x", (uint32_t)(end >> 32), (uint32_t)end);
    serial_write(" (");
    log_info("%u", (uint32_t)(end - start));
    serial_write(" bytes)\n");

    // Map using HHDM offset
    uint64_t hhdm_offset = hhdm_request.response->offset;

    for (uint64_t phys = start; phys < end; phys += 0x1000) {
        uint64_t virt = phys + hhdm_offset;

        // Check if already mapped
        page_entry_t *pte = get_page_entry(virt, 0);
        if (pte && (*pte & PAGE_PRESENT)) {
            // Already mapped, skip
            continue;
        }

        serial_write("A");
        // Map the page
        map_page(virt, phys, iskernel, writeable);
        serial_write("B");
    }

    serial_write("[paging]: Range mapping complete\n");
}
