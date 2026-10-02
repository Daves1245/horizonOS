#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

#include <stdint.h>

/*
 * Architecture-agnostic type definitions
 *
 * These types are defined based on the target architecture.
 * Architecture-specific headers should define these appropriately.
 *
 * In general, we try to avoid adding new types, for reasons similar to the
 * linux kernel, including confusion (goobly goobly goo) around the underlying
 * type and how these values should be used and/or accessed.
 *
 * Newly added types should then be:
 * 1) obvious of the underlying type (primitive, typesafe accessor, etc.)
 * 2) strictly serving a singular purpose
 * 3) necessary, i.e., no existing type already exists with the same width
 *    and meaning, and a primitive would demonstratively and obviously be
 * confusing, ambiguous, or otherwise be cumbersome to maintain.
 *
 * We strive to adhere by the following goals, unless there is a clear
 * exception, which is explicitly stated and defended, and reviewed (by me,
 * to me, and for me, Amen)).
 *
 * - maintainability
 * - readability
 * - correctness
 * - simplicity
 * - one source of truth
 * - type safety (in C LOL)
 * - consistency (names, patterns, etc.)
 */

// this is very hacky - let's bootstrap errno.h in the future instead
// TODO XXX
#define ENODEV 19

#ifdef __i386__
typedef uint32_t virt_addr_t; // Virtual address type for 32-bit
typedef uint32_t phys_addr_t; // Physical address type for 32-bit
typedef uint32_t register_t; // Register width
#endif

#ifdef __x86_64__
typedef uint64_t virt_addr_t; // Virtual address type for 64-bit
typedef uint64_t phys_addr_t; // Physical address type for 64-bit
typedef uint64_t register_t; // Register width
#endif

#endif /* KERNEL_TYPES_H */
