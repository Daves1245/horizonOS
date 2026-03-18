#include "graphics.h"
#include <drivers/serial.h>
#include <string.h>
#include <kheap.h>
#include <mm.h>

uint32_t *backbuffer;
uint32_t *screen;
uint32_t screen_size;
struct limine_framebuffer *fb;

void graphics_init(struct limine_framebuffer *framebuffer) {
    fb = framebuffer;
    screen = fb->address;
    screen_size = fb->pitch * fb->height;
    serial_printf("[graphics.c][init]: screen_size = %u\n", screen_size);
    virt_addr_t raw = kmalloc(screen_size);
    serial_printf("[graphics.c][init]: raw kmalloc addr: %p\n", raw);
    backbuffer = (uint32_t *) raw;
    memset(backbuffer, 0, screen_size);
    serial_printf("[graphics.c][init]: backbuffer: %p\n", backbuffer);
}

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}

void gfx_put_pixel(uint64_t x, uint64_t y, uint32_t val) {
    if (x >= fb->width || y >= fb->height) {
        return;
    }
    backbuffer[y * (fb->pitch / 4) + x] = val;
}

/* optimized 32-bit-wide copy using rep movsd for disabled SSE */
static inline void fast_copy32(uint32_t *dst, const uint32_t *src, uint32_t count) {
    asm volatile (
        "rep movsl"
        : "+D"(dst), "+S"(src), "+c"(count)
        :
        : "memory"
    );
}

void gfx_render() {
    fast_copy32(screen, backbuffer, screen_size / 4);
}

void gfx_fill_rect(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint32_t val) {
    /* clamp to screen bounds */
    uint64_t x0 = a < fb->width  ? a : fb->width;
    uint64_t y0 = b < fb->height ? b : fb->height;
    uint64_t x1 = c < fb->width  ? c : fb->width - 1;
    uint64_t y1 = d < fb->height ? d : fb->height - 1;
    uint32_t stride = fb->pitch / 4;

    for (uint64_t y = y0; y <= y1; y++) {
        uint32_t *row = &backbuffer[y * stride];
        for (uint64_t x = x0; x <= x1; x++) {
            row[x] = val;
        }
    }
}

// same trick as fast copy - fill in 4 bytes
// at a time
void gfx_clear_screen() {
    uint32_t count = screen_size / 4;
    uint32_t val = 0;
    uint32_t *dst = backbuffer;
    asm volatile (
        "rep stosl"
        : "+D"(dst), "+c"(count)
        : "a"(val)
        : "memory"
    );
}

void gfx_print_framebuffer(struct limine_framebuffer *framebuffer) {
    serial_printf("framebuffer:\n");
    serial_printf("address        = %p\n", framebuffer->address);
    serial_printf("width          = %u\n", framebuffer->width);
    serial_printf("height         = %u\n", framebuffer->height);
    serial_printf("pitch          = %u\n", framebuffer->pitch);
    serial_printf("bpp            = %u\n", framebuffer->bpp);
    serial_printf("memory_model   = %u\n", framebuffer->memory_model);
    serial_printf("red_mask_size  = %u\n", framebuffer->red_mask_size);
    serial_printf("red_mask_shift = %u\n", framebuffer->red_mask_shift);
    serial_printf("green_mask_size  = %u\n", framebuffer->green_mask_size);
    serial_printf("green_mask_shift = %u\n", framebuffer->green_mask_shift);
    serial_printf("blue_mask_size   = %u\n", framebuffer->blue_mask_size);
    serial_printf("blue_mask_shift  = %u\n", framebuffer->blue_mask_shift);
    serial_printf("edid_size      = %u\n", framebuffer->edid_size);
    serial_printf("edid           = %p\n", framebuffer->edid);
    serial_printf("mode_count     = %u\n", framebuffer->mode_count);
}

void gfx_print_framebuffer_mode(struct limine_framebuffer *framebuffer) {
    if (!framebuffer) {
        serial_printf("[graphics.c]:[print_framebuffer_mode]: invalid argument. exiting\n");
        return;
    }

    serial_printf("[graphics.c]:[print_framebuffer_mode]: # modes: %u\n", framebuffer->mode_count);
    for (uint64_t i = 0; i < framebuffer->mode_count; i++) {
        struct limine_video_mode *mode = framebuffer->modes[i];
        serial_printf("pitch        = %u\n", mode->pitch);
        serial_printf("width        = %u\n", mode->width);
        serial_printf("height       = %u\n", mode->height);
        serial_printf("bpp          = %u\n", mode->bpp);
        serial_printf("memory_model = %u\n", mode->memory_model);
    }
}
