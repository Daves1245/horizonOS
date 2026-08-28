#include "mm.h"

uint64_t kernel_phys_base = 0;
uint64_t kernel_virt_base = 0;

phys_addr_t virt_to_phys(virt_addr_t addr) {
#ifdef __x86_64__
	// kernel higher-half: translate using the actual physical load base from Limine
	if (addr >= kernel_virt_base) {
		return (addr - kernel_virt_base) + kernel_phys_base;
	}
	// HHDM range: physical memory mapped at hhdm_offset
	return addr - hhdm_offset;
#else
	return (phys_addr_t)addr; // identity mapped in i386
#endif
}

virt_addr_t phys_to_virt(phys_addr_t addr) {
#ifdef __x86_64__
	return addr + hhdm_offset; // mapped at hhdm offset
#else
	return (virt_addr_t)addr; // identity mapped in i386
#endif
}
