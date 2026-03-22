#include "console.h"
#include "font.h"
#include "graphics.h"

#include <stdarg.h>

static const fontStyle_t *console_font;
static uint32_t fg_color;
static uint32_t bg_color;

static int cursor_col;
static int cursor_row;
static int max_cols;
static int max_rows;
/* character cell width  (pixels) */
static int cell_w;
/* character cell height (pixels) */
static int cell_h;

static uint32_t screen_w;
static uint32_t screen_h;

/*
 * backbuffer is the internal buffer that is used for drawing with
 * the graphics API. once the image is ready to be drawn, it is rendered
 * by copying the backbuffer into the limine framebuffer
 * pointer + pitch — needed for scrolling
 */
extern uint32_t *backbuffer;
extern uint32_t screen_size;
extern struct limine_framebuffer *fb;

void console_init(struct limine_framebuffer *framebuffer) {
    console_font = &FontStyle_MonaspaceArgonLight;
    // light gray
    fg_color = rgb(0xCC, 0xCC, 0xCC);
    // black
    bg_color = rgb(0x00, 0x00, 0x00);

    /* cell size from font metrics */
    cell_w = console_font->FixedWidth ? console_font->FixedWidth : console_font->GlyphWidth[0];
    cell_h = console_font->GlyphHeight;

    screen_w = framebuffer->width;
    screen_h = framebuffer->height;

    max_cols = screen_w / cell_w;
    max_rows = screen_h / cell_h;

    cursor_col = 0;
    cursor_row = 0;
}

void scroll_up(void) {
    // pixels per row
    uint32_t stride = fb->pitch / 4;
    uint32_t row_pixels = cell_h * stride;

    /* shift every row up by one cell_h */
    uint32_t *dst = backbuffer;
    uint32_t *src = backbuffer + row_pixels;
    uint32_t copy_count = stride * (screen_h - cell_h);

    for (uint32_t i = 0; i < copy_count; i++) {
        dst[i] = src[i];
    }

    /* clear last row */
    uint32_t *last = backbuffer + stride * (screen_h - cell_h);
    for (uint32_t i = 0; i < row_pixels; i++) {
        last[i] = bg_color;
    }

    cursor_row = max_rows - 1;
}

static void newline(void) {
    cursor_col = 0;
    cursor_row++;
    if (cursor_row >= max_rows) {
        scroll_up();
    }
}

void console_putchar(char c) {
    switch (c) {
    case '\n':
        newline();
        return;
    case '\t':
        // not lat two bits - like a page align
        cursor_col = (cursor_col + 4) & ~3;
        if (cursor_col >= max_cols)
            newline();
        return;
    default:
        break;
    }

    if (cursor_col >= max_cols) {
        newline();
    }

    int px = cursor_col * cell_w;
    int py = cursor_row * cell_h;

    /* clear cell background */
    gfx_fill_rect(px, py, px + cell_w - 1, py + cell_h - 1, bg_color);

    /* draw glyph */
    font_draw_char(console_font, c, px, py, fg_color);

    cursor_col++;
}

void console_puts(const char *s) {
    while (*s)
        console_putchar(*s++);
}

// basically a copy of serial_printf
static void console_write_uint(uint64_t val, int base) {
    char buf[21];
    int i = 0;
    if (val == 0) {
        console_putchar('0');
        return;
    }
    while (val) {
        int d = val % base;
        buf[i++] = d < 10 ? '0' + d : 'a' + d - 10;
        val /= base;
    }
    while (--i >= 0) {
        console_putchar(buf[i]);
    }
}

static void console_write_int(int64_t val) {
    if (val < 0) {
        console_putchar('-');
        val = -val;
    }
    console_write_uint((uint64_t) val, 10);
}

// copy of serial_printf
void console_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            console_putchar(*p);
            continue;
        }
        p++;
        switch (*p) {
            case 'l':
                p++;
                switch (*p) {
                    case 'd':
                        console_write_int(va_arg(args, int64_t));
                        break;
                    case 'u':
                        console_write_uint(va_arg(args, uint64_t), 10);
                        break;
                    case 'x':
                        console_write_uint(va_arg(args, uint64_t), 16);
                        break;
                    default:
                        console_putchar('%');
                        console_putchar('l');
                        console_putchar(*p);
                        break;
                }
                break;
            case 'd':
                console_write_int((int64_t)va_arg(args, int));
                break;
            case 'u':
                console_write_uint((uint64_t) va_arg(args, unsigned int), 10);
                break;
            case 'x':
                console_write_uint((uint64_t) va_arg(args, unsigned int), 16);
                break;
            case 'p':
                console_puts("0x");
                console_write_uint((uint64_t)(uintptr_t) va_arg(args, void *), 16);
                break;
            case 's':
                console_puts(va_arg(args, const char *));
                break;
            case 'c':
                console_putchar((char)va_arg(args, unsigned int));
                break;
            case '%':
                console_putchar('%');
                break;
            default:
                console_putchar('%');
                console_putchar(*p);
                break;
        }
    }

    va_end(args);
}

void console_set_fg(uint32_t color) {
    fg_color = color;
}

void console_set_bg(uint32_t color) {
    bg_color = color;
}

void console_clear(void) {
    gfx_clear_screen();
    cursor_col = 0;
    cursor_row = 0;
}
