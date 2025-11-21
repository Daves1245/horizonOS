#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include "common.h"
#include <string.h>
#include <stdint.h>
#include <kernel/logger.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_putchar_at(char c, int x, int y);
void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y);
void terminal_write(const char *data, size_t size);
void terminal_write_dec(uint32_t n);
void terminal_writestring(const char *data);
void terminal_scroll();
void terminal_scrolln(size_t nrows);
void terminal_set_cursor(size_t x, size_t y);
void update_hardware_cursor(void);

void terminal_setcolor(uint8_t color);
void print_colored(const char* str, uint8_t fg, uint8_t bg);

#endif
