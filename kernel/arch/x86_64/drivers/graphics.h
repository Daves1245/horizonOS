#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <limine.h>

void graphics_init(struct limine_framebuffer *);
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
void gfx_put_pixel(uint64_t x, uint64_t y, uint32_t val);
void gfx_fill_rect(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint32_t val);
void gfx_print_framebuffer(struct limine_framebuffer *framebuffer);
void gfx_print_framebuffer_mode(struct limine_framebuffer *framebuffer);
void gfx_clear_screen();
void gfx_render();

#endif
