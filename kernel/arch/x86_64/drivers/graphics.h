#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <limine.h>

void put_pixel(struct limine_framebuffer *framebuffer, int x, int y, uint32_t val);

void print_framebuffer(struct limine_framebuffer *framebuffer);
void print_framebuffer_mode(struct limine_framebuffer *framebuffer);

#endif
