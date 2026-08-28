#ifndef _KERNEL_COMPILER_H
#define _KERNEL_COMPILER_H

/*
 * Named macros for gcc/clangfunction and type attributes we use. Copied
 * from linux kernel:
 *   https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html
 *   https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html
 */

#define __aligned(x)		__attribute__((__aligned__(x)))
#define __aligned_largest	__attribute__((__aligned__))
#define __always_inline		inline __attribute__((__always_inline__))
#define __attribute_const__	__attribute__((__const__))
#define __cold			__attribute__((__cold__))
#define __malloc		__attribute__((__malloc__))
#define __maybe_unused		__attribute__((__unused__))
#define __must_check		__attribute__((__warn_unused_result__))
#define __noinline		__attribute__((__noinline__))
#define __noreturn		__attribute__((__noreturn__))
#define __packed		__attribute__((__packed__))
#define __printf(a, b)		__attribute__((__format__(__printf__, a, b)))
#define __pure			__attribute__((__pure__))
#define __section(section)	__attribute__((__section__(section)))
#define __used			__attribute__((__used__))
#define __weak			__attribute__((__weak__))

#endif /* _KERNEL_COMPILER_H */
