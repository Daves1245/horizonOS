#include <drivers/graphics.h>
#include <drivers/serial.h>
#include "multiboot_info.h"
#include "memory/paging.h"

void graphics_init(struct multiboot_info *mbi) {
    if (!(mbi->flags & MB_FLAG_FRAMEBUFFER)) {
        serial_write("[graphics]: no framebuffer info — GRUB ignored VBE request\n");
        return;
    }
    if (mbi->framebuffer_type != MB_FB_TYPE_RGB) {
        serial_write("[graphics]: framebuffer is not RGB linear (fell back to text mode)\n");
        return;
    }
    if (mbi->framebuffer_bpp != 32) {
        serial_printf("[graphics]: unsupported bpp=%u (need 32)\n", mbi->framebuffer_bpp);
        return;
    }

    uint32_t phys  = (uint32_t) mbi->framebuffer_addr;
    uint32_t size  = mbi->framebuffer_pitch * mbi->framebuffer_height;

    /* identity-map the framebuffer physical pages before touching them */
    map_physical_range(phys, size, 1, 1);

    gfx_framebuffer_t fb = {
        .address = (uint32_t *) phys,
        .pitch   = mbi->framebuffer_pitch,
        .width   = mbi->framebuffer_width,
        .height  = mbi->framebuffer_height,
        .bpp     = mbi->framebuffer_bpp,
    };

    gfx_init(&fb);
}
