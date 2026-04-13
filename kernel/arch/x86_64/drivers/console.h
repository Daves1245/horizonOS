#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdarg.h>
#include <stdint.h>
#include <limine.h>

/*
 * framebuffer text console — renders anti-aliased bitmap-font text
 * onto the Limine framebuffer via the graphics backbuffer.
 */

void console_init(struct limine_framebuffer *fb);
void console_putchar(char c);
void console_puts(const char *s);
void console_vprintf(const char *fmt, va_list args);
void console_printf(const char *fmt, ...);
void console_set_fg(uint32_t color);
void console_set_bg(uint32_t color);
void console_clear(void);

#endif
