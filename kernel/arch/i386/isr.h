#include "common.h"

struct registers {
    u32int ds;
		// this is the pusha order, verified by the intel 64 manual
		// we write it in reverse since this is the way it appears
		// in assembly?? TODO
		u32int edi, esi, ebp, esp, ebx, edx, ecx, eax;
		u32int int_no, err_code;                       // isr_common_stub
    u32int eip, cs, eflags, useresp, ss;           // cpu on interrupt
} __attribute__((packed));
