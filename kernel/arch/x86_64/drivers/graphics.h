#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <limine.h>

void graphics_init(struct limine_framebuffer *);
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);

/*
 * drawing target:
 *   GFX_TARGET_BACKBUFFER — writes stage in the backbuffer. call
 *     gfx_render() to make them visible. use when tearing matters
 *     (animated scenes, pong).
 *   GFX_TARGET_FRAMEBUFFER — writes go straight to the limine fb.
 *     no render step. use for text / low-rate output where the
 *     per-update blit would be wasteful.
 *
 * the target is a process-wide mode; callers set it before drawing
 * and leave it set as long as they own the screen.
 */
enum gfx_target {
    GFX_TARGET_BACKBUFFER = 0,
    GFX_TARGET_FRAMEBUFFER = 1,
};

void gfx_set_target(enum gfx_target target);
enum gfx_target gfx_get_target(void);

void gfx_put_pixel(uint64_t x, uint64_t y, uint32_t val);
void gfx_fill_rect(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint32_t val);
void gfx_clear_screen(void);
/* no-op when target == FRAMEBUFFER */
void gfx_render(void);

void gfx_print_framebuffer(struct limine_framebuffer *framebuffer);
void gfx_print_framebuffer_mode(struct limine_framebuffer *framebuffer);

#endif
