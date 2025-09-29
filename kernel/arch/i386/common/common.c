#include "common.h"

// write a byte out to the specified port.
void outb(u16int port, u8int value) {
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}
