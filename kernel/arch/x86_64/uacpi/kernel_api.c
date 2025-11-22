#include <uacpi/types.h>
#include <uacpi/status.h>
#include <uacpi/log.h>
#include <uacpi/platform/arch_helpers.h>
#include "kernel_api.h"
#include <apic/rsdp.h>
#include <x86_64/memory/paging.h>
#include <kernel/logger.h>
#include <limine.h>

extern volatile struct limine_hhdm_request hhdm_request;

// Returns the PHYSICAL address of the RSDP structure via *out_rsdp_address.
uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    *out_rsdp_address = get_rsdp_phys();
    return UACPI_STATUS_OK;
}

/*
 * Map a physical memory range starting at 'addr' with length 'len', and return
 * a virtual address that can be used to access it.
 *
 * NOTE: 'addr' may be misaligned, in this case the host is expected to round it
 *       down to the nearest page-aligned boundary and map that, while making
 *       sure that at least 'len' bytes are still mapped starting at 'addr'. The
 *       return value preserves the misaligned offset.
 *
 *       Example for uacpi_kernel_map(0x1ABC, 0xF00):
 *           1. Round down the 'addr' we got to the nearest page boundary.
 *              Considering a PAGE_SIZE of 4096 (or 0x1000), 0x1ABC rounded down
 *              is 0x1000, offset within the page is 0x1ABC - 0x1000 => 0xABC
 *           2. Requested 'len' is 0xF00 bytes, but we just rounded the address
 *              down by 0xABC bytes, so add those on top. 0xF00 + 0xABC => 0x19BC
 *           3. Round up the final 'len' to the nearest PAGE_SIZE boundary, in
 *              this case 0x19BC is 0x2000 bytes (2 pages if PAGE_SIZE is 4096)
 *           4. Call the VMM to map the aligned address 0x1000 (from step 1)
 *              with length 0x2000 (from step 3). Let's assume the returned
 *              virtual address for the mapping is 0xF000.
 *           5. Add the original offset within page 0xABC (from step 1) to the
 *              resulting virtual address 0xF000 + 0xABC => 0xFABC. Return it
 *              to uACPI.
 */
void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    // map_physical_range() will handle alignment internally, so just use as is
    map_physical_range(addr, len, 1, 1);

    // we have the physical range, with limine we're mapped
    // into hhdm so just add the offset to get the virtual address
    uint64_t hhdm_offset = hhdm_request.response->offset;
    void *virt_addr = (void *)(addr + hhdm_offset);

    return virt_addr;
}

/*
 * Unmap a virtual memory range at 'addr' with a length of 'len' bytes.
 *
 * NOTE: 'addr' may be misaligned, see the comment above 'uacpi_kernel_map'.
 *       Similar steps to uacpi_kernel_map can be taken to retrieve the
 *       virtual address originally returned by the VMM for this mapping
 *       as well as its true length.
 */
void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    // for now, we don't actually unmap since we're using HHDM which is
    // a direct physical memory mapping
    (void) addr;
    (void) len;
}

/*
 * logging for uacpi
 * in barebones mode, this receives pre-formatted strings.
 */
void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *msg) {
    // Map uACPI log levels to your kernel's log levels
    switch (level) {
        case UACPI_LOG_DEBUG:
            log_debug("[uACPI] %s", msg);
            break;
        case UACPI_LOG_INFO:
            log_info("[uACPI] %s", msg);
            break;
        case UACPI_LOG_WARN:
            log_warning("[uACPI] %s", msg);
            break;
        case UACPI_LOG_ERROR:
            log_error("[uACPI] %s", msg);
            break;
        default:
            log_info("[uACPI] %s", msg);
            break;
    }
}
