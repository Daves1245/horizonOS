/* font.c — 1bpp bitmap font renderer for fontStyle_t (bmfont2c.py output).
 * Bitmaps are packed MSB-first, each glyph is GlyphBytesWidth * GlyphHeight bytes. */

#include "font.h"
#include "graphics.h"

/* draw one character at (x,y), return advance width */
int font_draw_char(const fontStyle_t *font, char c, int x, int y, uint32_t color) {
    uint8_t code = (uint8_t) c;
    if (code < font->FirstAsciiCode || code >= font->FirstAsciiCode + font->GlyphCount)
        return font->FixedWidth ? font->FixedWidth
            : (font->GlyphWidth ? font->GlyphWidth[0] : 0);

    uint8_t idx = code - font->FirstAsciiCode;
    int adv = font->FixedWidth ? font->FixedWidth : font->GlyphWidth[idx];

    uint32_t glyph_size = font->GlyphBytesWidth * font->GlyphHeight;
    const uint8_t *bmp = &font->GlyphBitmaps[idx * glyph_size];

    /* scan each row of the glyph (GlyphBytesWidth bytes per row) */
    for (int row = 0; row < font->GlyphHeight; row++) {
        const uint8_t *row_data = &bmp[row * font->GlyphBytesWidth];
        /* scan each pixel (bit) across the row: bytes * 8 = total pixel columns */
        for (int col = 0; col < font->GlyphBytesWidth * 8; col++) {
            /* col>>3 selects the byte, 0x80>>(col&7) masks the bit within it */
            if (row_data[col >> 3] & (0x80 >> (col & 7))) {
                gfx_put_pixel(x + col, y + row, color);
            }
        }
    }

    return adv;
}

/* draw a null-terminated string, advancing x by each glyph's width */
void font_draw_string(const fontStyle_t *font, const char *s, int x, int y, uint32_t color) {
    while (*s) {
        x += font_draw_char(font, *s, x, y, color);
        s++;
    }
}
