#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdarg.h>

void init_serial();
void serial_putchar(char c);
void serial_write(const char *str);
void serial_write_hex(const char *prefix, uint32_t val);
void serial_printf(const char *fmt, ...);
void serial_vprintf(const char *fmt, va_list args);

#endif
