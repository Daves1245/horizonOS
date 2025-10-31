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

// Color printing helpers
void terminal_setcolor(uint8_t color);
void print_colored(const char* str, uint8_t fg, uint8_t bg);

// Logger functions (from i386-specific logger.h)
#include "common/logger.h"

#endif
