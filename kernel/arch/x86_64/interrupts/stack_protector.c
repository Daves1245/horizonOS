#include <stdint.h>

// Stack protector implementation for x86_64
// Required when using -fstack-protector

#if defined(__x86_64__)
__attribute__((used))
uintptr_t __stack_chk_guard = 0x12345678DEADBEEF;
#else
__attribute__((used))
uintptr_t __stack_chk_guard = 0xDEADBEEF;
#endif

__attribute__((noreturn))
void __stack_chk_fail(void) {
    // halt for now - useful for debugging
    asm volatile("cli; hlt" ::: "memory");
    __builtin_unreachable();
}
