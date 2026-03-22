#include "font.h"
#include "graphics.h"

/* ------------------------------------------------------------------ */
/*  Drawing (1bpp packed bitmaps, MSB-first)                          */
/* ------------------------------------------------------------------ */

int font_draw_char(const fontStyle_t *font, char c,
                   int x, int y, uint32_t color)
{
    uint8_t code = (uint8_t)c;
    if (code < font->FirstAsciiCode ||
        code >= font->FirstAsciiCode + font->GlyphCount)
        return font->FixedWidth ? font->FixedWidth
                                : (font->GlyphWidth ? font->GlyphWidth[0] : 0);

    uint8_t idx = code - font->FirstAsciiCode;
    int adv = font->FixedWidth ? font->FixedWidth : font->GlyphWidth[idx];

    uint32_t glyph_size = font->GlyphBytesWidth * font->GlyphHeight;
    const uint8_t *bmp = &font->GlyphBitmaps[idx * glyph_size];

    for (int row = 0; row < font->GlyphHeight; row++) {
        const uint8_t *row_data = &bmp[row * font->GlyphBytesWidth];
        for (int col = 0; col < font->GlyphBytesWidth * 8; col++) {
            if (row_data[col >> 3] & (0x80 >> (col & 7)))
                gfx_put_pixel(x + col, y + row, color);
        }
    }

    return adv;
}

void font_draw_string(const fontStyle_t *font, const char *s,
                      int x, int y, uint32_t color)
{
    while (*s) {
        x += font_draw_char(font, *s, x, y, color);
        s++;
    }
}
