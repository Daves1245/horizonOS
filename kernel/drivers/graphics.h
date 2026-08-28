#ifndef KERNEL_GRAPHICS_H
#define KERNEL_GRAPHICS_H

#include <stdint.h>

typedef struct {
    uint32_t *address;  /* virtual address — caller must map before passing */
    uint32_t  pitch;    /* bytes per row */
    uint32_t  width;    /* pixels */
    uint32_t  height;   /* pixels */
    uint8_t   bpp;
} gfx_framebuffer_t;

/*
 * GFX_TARGET_BACKBUFFER — writes stage in the backbuffer; call gfx_render()
 *   to make them visible. use when tearing matters (animated scenes, pong).
 * GFX_TARGET_FRAMEBUFFER — writes go straight to the fb. no render step.
 *   use for text / low-rate output where per-update blits would be wasteful.
 */
enum gfx_target {
    GFX_TARGET_BACKBUFFER  = 0,
    GFX_TARGET_FRAMEBUFFER = 1,
};

void                     gfx_init(const gfx_framebuffer_t *fb);
const gfx_framebuffer_t *gfx_get_fb(void);

void            gfx_set_target(enum gfx_target t);
enum gfx_target gfx_get_target(void);

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);

void gfx_put_pixel(uint32_t x, uint32_t y, uint32_t val);
void gfx_fill_rect(int x0, int y0, int x1, int y1, uint32_t val);
void gfx_clear_screen(void);
void gfx_render(void);  /* no-op when target == GFX_TARGET_FRAMEBUFFER */

#endif
