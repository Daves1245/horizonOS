#ifndef BIOS_H
#define BIOS_H

#include <stdint.h>

struct registers_t {
    uint16_t ax, bx, cx, dx;
    uint16_t si, di, bp, sp;
    uint16_t es, ds, fs, gs;
    uint16_t flags;
};

void bios_interrupt(uint8_t int_no, struct registers_t *regs);

#endif
