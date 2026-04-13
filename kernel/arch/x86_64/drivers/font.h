#ifndef FONT_H
#define FONT_H

#include <fonts/fontlibrary.h>

/*
 * draws glyphs via gfx_put_pixel; whether that goes to the backbuffer
 * or straight to the framebuffer depends on the current gfx target
 * (see gfx_set_target in graphics.h).
 */
int font_draw_char(const fontStyle_t *font, char c, int x, int y, uint32_t color);
void font_draw_string(const fontStyle_t *font, const char *s, int x, int y, uint32_t color);

#endif
