#ifndef UAPIC_KERNEL_API_H
#define UAPIC_KERNEL_API_H

#include <uacpi/types.h>
#include <uacpi/status.h>
#include <uacpi/platform/arch_helpers.h>

// Function declarations for uACPI kernel API
uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address);
void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len);
void uacpi_kernel_unmap(void *addr, uacpi_size len);

#endif
