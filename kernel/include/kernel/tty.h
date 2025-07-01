#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include "common.h"
#include <string.h>
#include <stdint.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char *data, size_t size);
void terminal_write_dec(uint32_t n);
void terminal_writestring(const char *data);
void terminal_scroll();

#endif
