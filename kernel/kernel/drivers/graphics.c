#include <drivers/graphics.h>
#include <drivers/serial.h>
#include <string.h>
#include <kheap.h>
#include <mm.h>

static gfx_framebuffer_t fb;
static uint32_t *backbuffer;
static uint32_t screen_size;

static enum gfx_target target = GFX_TARGET_BACKBUFFER;

const gfx_framebuffer_t *gfx_get_fb(void) { return &fb; }

void gfx_set_target(enum gfx_target t) { target = t; }
enum gfx_target gfx_get_target(void) { return target; }

static inline uint32_t *draw_buffer(void) {
    return target == GFX_TARGET_FRAMEBUFFER ? fb.address : backbuffer;
}

void gfx_init(const gfx_framebuffer_t *framebuffer) {
    fb = *framebuffer;
    screen_size = fb.pitch * fb.height;
    serial_printf("[graphics]: %ux%u pitch=%u bpp=%u addr=%p\n",
        fb.width, fb.height, fb.pitch, fb.bpp, fb.address);
    backbuffer = (uint32_t *) kmalloc(screen_size);
    memset(backbuffer, 0, screen_size);
}

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void gfx_put_pixel(uint32_t x, uint32_t y, uint32_t val) {
    if (x >= fb.width || y >= fb.height)
        return;
    draw_buffer()[y * (fb.pitch / 4) + x] = val;
}

/* optimized 32-bit-wide copy using rep movsd — works on both i386 and x86_64 */
static inline void fast_copy32(uint32_t *dst, const uint32_t *src, uint32_t count) {
    asm volatile (
        "rep movsl"
        : "+D"(dst), "+S"(src), "+c"(count)
        :
        : "memory"
    );
}

void gfx_render(void) {
    if (target != GFX_TARGET_BACKBUFFER) return;
    fast_copy32(fb.address, backbuffer, screen_size / 4);
}

void gfx_fill_rect(int x0, int y0, int x1, int y1, uint32_t val) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= (int)fb.width)  x1 = (int)fb.width  - 1;
    if (y1 >= (int)fb.height) y1 = (int)fb.height - 1;
    if (x0 > x1 || y0 > y1) return;
    uint32_t stride = fb.pitch / 4;
    uint32_t *buf = draw_buffer();
    for (int y = y0; y <= y1; y++) {
        uint32_t *row = &buf[y * stride];
        for (int x = x0; x <= x1; x++)
            row[x] = val;
    }
}

void gfx_clear_screen(void) {
    uint32_t count = screen_size / 4;
    uint32_t val = 0;
    uint32_t *dst = draw_buffer();
    asm volatile (
        "rep stosl"
        : "+D"(dst), "+c"(count)
        : "a"(val)
        : "memory"
    );
}
