#include "graphics.h"
#include <drivers/serial.h>

void put_pixel(struct limine_framebuffer *framebuffer, int x, int y, uint32_t val) {
    volatile uint32_t *pixel = &framebuffer->address[framebuffer->width * y + x];
    *pixel = val;
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
