#include <stdint.h>
#include <stdlib.h>

uintptr_t __stack_chk_guard = 0x595e9fbd42d4b592;

__attribute__((noreturn)) void __stack_chk_fail(void) {
    const char *message = "*** STACK SMASHING DETECTED ***";
    volatile uint16_t *vga = (uint16_t *) 0xB8000;

    for (int i = 0; message[i]; i++) {
        vga[i] = (uint16_t) message[i] | 0x4F00; // Red on white
    }

    asm volatile("cli; hlt");
    while (1);
}
