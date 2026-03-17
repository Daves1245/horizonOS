#include "graphics.h"
#include <drivers/serial.h>

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}

void put_pixel(struct limine_framebuffer *framebuffer, uint64_t x, uint64_t y, uint32_t val) {
    if (x >= framebuffer->width || y >= framebuffer->height) {
        return;
    }
    uint64_t bpp = framebuffer->bpp / 8;
    volatile uint32_t *pixel = (volatile uint32_t *)((uint8_t *)framebuffer->address + framebuffer->pitch * y + x * bpp);
    *pixel = val;
}

void fill_rect(struct limine_framebuffer *framebuffer, uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint32_t val) {
    uint32_t *fb = framebuffer->address;
    for (uint64_t i = a; i <= c; i++) {
        if (i >= framebuffer->width) continue;
        for (uint64_t j = b; j <= d; j++) {
            if (j >= framebuffer->height) continue;
            fb[j * (framebuffer->pitch / 4) + i] = val;
        }
    }
}

void print_framebuffer(struct limine_framebuffer *framebuffer) {
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

void print_framebuffer_mode(struct limine_framebuffer *framebuffer) {
    if (!framebuffer) {
        serial_printf("[graphics.c]:[print_framebuffer_mode]: invalid argument. exiting\n");
        return;
    }

    serial_printf("[graphics.c]:[print_framebuffer_mode]: # modes: %u\n", framebuffer->mode_count);
    for (int i = 0; i < framebuffer->mode_count; i++) {
        struct limine_video_mode *mode = framebuffer->modes[i];
        serial_printf("pitch        = %u\n", mode->pitch);
        serial_printf("width        = %u\n", mode->width);
        serial_printf("height       = %u\n", mode->height);
        serial_printf("bpp          = %u\n", mode->bpp);
        serial_printf("memory_model = %u\n", mode->memory_model);
    }
}
