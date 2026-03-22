#ifndef FONT_H
#define FONT_H

#include <fonts/fontlibrary.h>

/* Draw a single character. Returns advance width in pixels. */
int font_draw_char(const fontStyle_t *font, char c,
                   int x, int y, uint32_t color);

/* Draw a NUL-terminated string starting at pixel position (x, y). */
void font_draw_string(const fontStyle_t *font, const char *s,
                      int x, int y, uint32_t color);

#endif /* FONT_H */
