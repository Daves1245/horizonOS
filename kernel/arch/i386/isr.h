/*
 * Interface and structures for high level interrupt service routines.
 * Part of this code is modified from Bran's kernel development tutorials. Rewritten for JamesM's kernel development tutorials.
 *
 */

#include "common.h"

struct registers {
    u32int ds;
    u32int edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
    u32int int_no, err_code;                       // isr_common_stub
    u32int eip, cs, eflags, useresp, ss;           // cpu on interrupt
} __attribute__((packed));
