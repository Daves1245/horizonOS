#include <drivers/graphics.h>
#include <drivers/serial.h>
#include <limine.h>

void graphics_init(struct limine_framebuffer *framebuffer) {
    gfx_framebuffer_t fb = {
        .address = framebuffer->address,
        .pitch   = (uint32_t) framebuffer->pitch,
        .width   = (uint32_t) framebuffer->width,
        .height  = (uint32_t) framebuffer->height,
        .bpp     = (uint8_t)  framebuffer->bpp,
    };
    gfx_init(&fb);
}

void gfx_print_framebuffer(struct limine_framebuffer *fb) {
    serial_printf("framebuffer:\n");
    serial_printf("  address        = %p\n",  fb->address);
    serial_printf("  width          = %u\n",  (uint32_t) fb->width);
    serial_printf("  height         = %u\n",  (uint32_t) fb->height);
    serial_printf("  pitch          = %u\n",  (uint32_t) fb->pitch);
    serial_printf("  bpp            = %u\n",  (uint32_t) fb->bpp);
    serial_printf("  memory_model   = %u\n",  (uint32_t) fb->memory_model);
    serial_printf("  red_mask_size  = %u\n",  (uint32_t) fb->red_mask_size);
    serial_printf("  red_mask_shift = %u\n",  (uint32_t) fb->red_mask_shift);
    serial_printf("  green_mask_size  = %u\n",(uint32_t) fb->green_mask_size);
    serial_printf("  green_mask_shift = %u\n",(uint32_t) fb->green_mask_shift);
    serial_printf("  blue_mask_size   = %u\n",(uint32_t) fb->blue_mask_size);
    serial_printf("  blue_mask_shift  = %u\n",(uint32_t) fb->blue_mask_shift);
}

void gfx_print_framebuffer_mode(struct limine_framebuffer *fb) {
    if (!fb) {
        serial_printf("[graphics]: gfx_print_framebuffer_mode: null pointer\n");
        return;
    }
    serial_printf("[graphics]: %u modes\n", (uint32_t) fb->mode_count);
    for (uint64_t i = 0; i < fb->mode_count; i++) {
        struct limine_video_mode *mode = fb->modes[i];
        serial_printf("  [%u] %ux%u bpp=%u pitch=%u model=%u\n",
            (uint32_t) i,
            (uint32_t) mode->width, (uint32_t) mode->height,
            (uint32_t) mode->bpp,   (uint32_t) mode->pitch,
            (uint32_t) mode->memory_model);
    }
}
