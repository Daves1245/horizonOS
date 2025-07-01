#include "common.h"

struct registers {
    u32int ds;
    u32int edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
    u32int int_no, err_code;                       // isr_common_stub
    u32int eip, cs, eflags, useresp, ss;           // cpu on interrupt
} __attribute__((packed));
