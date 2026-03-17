#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <limine.h>

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
void put_pixel(struct limine_framebuffer *framebuffer, uint64_t x, uint64_t y, uint32_t val);
void fill_rect(struct limine_framebuffer *framebuffer, uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint32_t val);
void print_framebuffer(struct limine_framebuffer *framebuffer);
void print_framebuffer_mode(struct limine_framebuffer *framebuffer);

#endif
