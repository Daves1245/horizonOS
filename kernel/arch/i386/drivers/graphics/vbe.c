#include "vbe.h"

int vbe_get_controller_info(struct vbe_controller_info *info) {
    uint16_t result;

    asm volatile("int 0x10"
            : "=a" (result) // output: eax register
            : "a" (0x4F00), // input: ax 4F00h
              "D" (info), // input: DI = pointer to info struct
              "S" (0) // input: si = 0 (some bios(es?) need this)
            : "memory", "cc" // clobbers: memory and condition codes
    );

    return (result == 0x004F) ? 0 : -1;
}
