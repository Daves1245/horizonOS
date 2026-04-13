#ifndef FONT_H
#define FONT_H

#include <fonts/fontlibrary.h>

int font_draw_char(const fontStyle_t *font, char c, int x, int y, uint32_t color);
void font_draw_string(const fontStyle_t *font, const char *s, int x, int y, uint32_t color);

#endif
