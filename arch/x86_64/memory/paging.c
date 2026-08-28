#include "paging.h"
#include "../../kernel/kheap.h"
#include <string.h>
#include <drivers/serial.h>
#include <kernel/logger.h>
#include <limine.h>

// We'll use Limine's page tables initially, then can create our own if needed
// Limine provides the HHDM which we need to access physical memory
extern struct limine_hhdm_request hhdm_request;

void init_paging(void) {
    // Limine already set up paging for us, nothing to do here
    // We're using base revision 3, so we'll manually map what we need
    log_debug("[paging]: Using Limine's page tables (4-level paging)\n");

    uint64_t cr3 = read_cr3();
    log_debug("[paging]: CR3 = 0x%x%x\n", (uint32_t)(cr3 >> 32), (uint32_t)cr3);
}

/**
 * Break a huge page into the next lebel
 *
 * Limine maps the HHDM (and the kernel image) with the largest pages it can
 * for some reason, so walking down to the PT level runs into a PDPT or
 * PD entry while we have PAGE_PS set. These entries are 1 GiB/2 MiB *frames*,
 * not the address of the next table down. Following one gives us a bogus/garbage
 * frame, and writing to it can corrupt unrelated memory / is UB (i do declare).
 *
 * The fix is we replace the entry with a real table whose 512 entries
 * reproduce the same translation one level down. The mapping is left
 * unchanged, and the walk below is allowed to do this whether or not
 * the caller passed create.
 *
 * @param huge The PDPT/PD huge entry
 * @param step How many bytes the new entry should have: 2 MiB when splitting a 1 GiB page,
 *                                                    PAGE_SIZE when splitting a 2 MiB one
 * @return Physical address of the new table one level down
 */
static phys_addr_t split_huge_entry(uint64_t huge, uint64_t step) {
    // flags live in bits 11-0 and 63-52, so the huge-page PAT bit (bit 12) is
    // dropped here rather than translated into its PT-level position (bit 7).
    // TODO(pat) revisit once we map device memory with a non-WB type
    uint64_t flags = huge & ~PAGE_FRAME_MASK;

    // bit 12 is part of PAGE_FRAME_MASK but is *not* part of a huge frame's
    // address, so align down to the size the entry actually maps
    phys_addr_t base = PAGE_GET_ADDR(huge) & ~((step * 512) - 1);

    // at the PT level bit 7 means PAT, not PS. a 4 KiB entry must not carry it
    if (step == PAGE_SIZE) {
        flags &= ~PAGE_PS;
    }

    uint64_t *table = (uint64_t *) kmalloc_az(PAGE_SIZE);

    for (uint64_t i = 0; i < 512; i++) {
        table[i] = (base + i * step) | flags;
    }

    return virt_to_phys((virt_addr_t) table);
}

/**
 * Get or create a page table entry for a virtual address
 *
 * @param vaddr Virtual address
 * @param create If 1, create intermediate tables if they don't exist
 * @return Pointer to the page table entry, or NULL if not found and create=0
 */
pte_t *get_page_entry(virt_addr_t vaddr, int create, uint64_t cr3) {

    /* walk down the tree */
    phys_addr_t pml4_phys = PAGE_GET_ADDR(cr3);
    p4d_t *pml4 = (p4d_t *) phys_to_virt(pml4_phys);

    uint64_t pml4_idx = PML4_INDEX(vaddr);
    p4e_t *pml4e = &pml4->entries[pml4_idx];

    // no PAGE_PS check at this level: with 4-level paging bit 7 of a PML4
    // entry is reserved and must be 0, so a PML4 entry is always a table

    pud_t *pud;
    if (!(p4e_val(*pml4e) & PAGE_PRESENT)) {
        if (!create) {
            return NULL;
        }

        pud = (pud_t *) kmalloc_az(sizeof(pud_t));

        phys_addr_t pud_phys = virt_to_phys((virt_addr_t) pud);
        *p4e_ptr(pml4e) = pud_phys | PAGE_PRESENT | PAGE_WRITE;
    } else {
        phys_addr_t pud_phys = PAGE_GET_ADDR(p4e_val(*pml4e));
        pud = (pud_t *) phys_to_virt(pud_phys);
    }

    uint64_t pud_idx = PDPT_INDEX(vaddr);
    pue_t *pue = &pud->entries[pud_idx];

    pd_t *pd;
    if (!(pue_val(*pue) & PAGE_PRESENT)) {
        if (!create) {
            return NULL;
        }

        pd = (pd_t *) kmalloc_az(sizeof(pd_t));

        phys_addr_t pd_phys = virt_to_phys((virt_addr_t) pd);
        *pue_ptr(pue) = pd_phys | PAGE_PRESENT | PAGE_WRITE;
    } else {
        if (pue_val(*pue) & PAGE_PS) {
            // 1 GiB page: swap it for a page directory of 512 2 MiB pages.
            phys_addr_t split = split_huge_entry(pue_val(*pue), 0x200000UL);
            *pue_ptr(pue) = split | PAGE_PRESENT | PAGE_WRITE |
                            (pue_val(*pue) & (PAGE_USER | PAGE_NX));

            if (cr3 == read_cr3()) {
                flush_tlb_all();
            }
        }

        phys_addr_t pd_phys = PAGE_GET_ADDR(pue_val(*pue));
        pd = (pd_t *) phys_to_virt(pd_phys);
    }

    uint64_t pd_idx = PD_INDEX(vaddr);
    pde_t *pde = &pd->entries[pd_idx];

    pt_t *pt;
    if (!(pde_val(*pde) & PAGE_PRESENT)) {
        if (!create) {
            return NULL;
        }

        pt = (pt_t *) kmalloc_az(sizeof(pt_t));

        phys_addr_t pt_phys = virt_to_phys((virt_addr_t) pt);
        *pde_ptr(pde) = pt_phys | PAGE_PRESENT | PAGE_WRITE;
    } else {
        if (pde_val(*pde) & PAGE_PS) {
            // 2 MiB page: swap it for a page table of 512 4 KiB pages
            phys_addr_t split = split_huge_entry(pde_val(*pde), PAGE_SIZE);
            *pde_ptr(pde) = split | PAGE_PRESENT | PAGE_WRITE |
                            (pde_val(*pde) & (PAGE_USER | PAGE_NX));

            if (cr3 == read_cr3()) {
                flush_tlb_all();
            }
        }

        phys_addr_t pt_phys = PAGE_GET_ADDR(pde_val(*pde));
        pt = (pt_t *) phys_to_virt(pt_phys);
    }

    // physical address of table entry
    return &pt->entries[PT_INDEX(vaddr)];
}

/**
 * Map a virtual page to a physical page
 *
 * @param vaddr Virtual address (will be page-aligned)
 * @param paddr Physical address (will be page-aligned)
 * @param iskernel 1 for kernel pages, 0 for user pages
 * @param writeable 1 for writable pages, 0 for read-only
 */
void map_page(virt_addr_t virt_addr, uint64_t phys_addr, int iskernel, int writeable, uint64_t cr3) {
    // align addresses to page boundaries
    virt_addr &= ~0xFFFUL;
    phys_addr &= ~0xFFFUL;

    pte_t *pte = get_page_entry(virt_addr, 1, cr3);
    if (!pte) {
        log_error("[paging]: Failed to get page entry for vaddr 0x%x%x\n",
                (uint32_t)(virt_addr >> 32), (uint32_t)virt_addr);
        return;
    }

    // set up the page table entry
    *pte_ptr(pte) = phys_addr | PAGE_PRESENT;

    if (writeable) {
        *pte_ptr(pte) |= PAGE_WRITE;
    }

    if (!iskernel) {
        *pte_ptr(pte) |= PAGE_USER;
    }

    // flush TLB for this page
    invalidate_page(virt_addr);
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
void map_physical_range(phys_addr_t phys_addr, uint32_t size, int iskernel, int writeable, uint64_t cr3) {
    // Align to page boundaries
    phys_addr_t start = phys_addr & ~0xFFFUL;
    phys_addr_t end = (phys_addr + size + 0xFFF) & ~0xFFFUL;

    log_debug("[paging]: mapping phys 0x%x%x - 0x%x%x\n",
            (uint32_t)(start >> 32), (uint32_t)start,
            (uint32_t)(end >> 32), (uint32_t)end);

    // Map using HHDM offset
    uint64_t hhdm_offset = hhdm_request.response->offset;

    for (phys_addr_t phys_it = start; phys_it < end; phys_it += 0x1000) {
        virt_addr_t virt = phys_to_virt(phys_it);

        // Check if already mapped
        pte_t *pte = get_page_entry(virt, 0, cr3);
        if (pte && (pte_val(*pte) & PAGE_PRESENT)) {
            // Already mapped, skip
            continue;
        }

        map_page(virt, phys_it, iskernel, writeable, cr3);
    }
}

/**
 *
 * Unmap a page rooted in cr3
 *
 * @return the physical frame that was unmapped, or 0 if it was dangling.
 * Note that this function does not free the returned frame, that decision
 * is left to the caller.
 */
phys_addr_t unmap_page(virt_addr_t addr, uint64_t cr3) {
    // page align
    addr &= ~0xFFFUL;

    pte_t *pte = get_page_entry(addr, 0, cr3);
    if (!pte || !(pte_val(*pte) & PAGE_PRESENT)) {
        return 0;
    }

    phys_addr_t frame = PAGE_GET_ADDR(pte_val(*pte));

    *pte_ptr(pte) = 0;

    // only invalidate the TLB if we benefit from it, i.e. the owning
    // cr3 is the same one as our currently running process.
    if (cr3 == read_cr3()) {
        invalidate_page(addr);
    }

    return frame;
}
