#include <stdint.h>
#include <kernel/compiler.h>

// Stack protector implementation for x86_64
// Required when using -fstack-protector

#if defined(__x86_64__)
__used uintptr_t __stack_chk_guard = 0x12345678DEADBEEF;
#else
__used uintptr_t __stack_chk_guard = 0xDEADBEEF;
#endif

__noreturn void __stack_chk_fail(void) {
	// halt for now - useful for debugging
	asm volatile("cli; hlt" ::: "memory");
	__builtin_unreachable();
}
