#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

#include <stdint.h>

/*
 * Architecture-agnostic type definitions
 *
 * These types are defined based on the target architecture.
 * Architecture-specific headers should define these appropriately.
 */

// this is very hacky - let's bootstrap errno.h in the future instead
// TODO XXX
#define ENODEV 19

#ifdef __i386__
typedef uint32_t virt_addr_t;  // Virtual address type for 32-bit
typedef uint32_t phys_addr_t;  // Physical address type for 32-bit
#endif

#ifdef __x86_64__
typedef uint64_t virt_addr_t;  // Virtual address type for 64-bit
typedef uint64_t phys_addr_t;  // Physical address type for 64-bit
#endif

#endif /* KERNEL_TYPES_H */
