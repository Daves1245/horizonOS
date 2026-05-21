/*
 * https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Boot-information-format
 */
#ifndef MULTIBOOT_INFO_H
#define MULTIBOOT_INFO_H

#include <stdint.h>

/* flags */
#define MB_FLAG_MEM         (1 << 0)   // mem_lower/mem_upper valid
#define MB_FLAG_BOOT_DEV    (1 << 1)   // boot_device valid
#define MB_FLAG_CMDLINE     (1 << 2)   // cmdline valid
#define MB_FLAG_MODS        (1 << 3)   // mods_count/mods_addr valid
#define MB_FLAG_AOUT_SYM    (1 << 4)   // syms.aout valid
#define MB_FLAG_ELF_SHD     (1 << 5)   // syms.elf valid
#define MB_FLAG_MMAP        (1 << 6)   // mmap_length/mmap_addr valid
#define MB_FLAG_DRIVES      (1 << 7)   // drives_length/drives_addr valid
#define MB_FLAG_CONFIG      (1 << 8)   // config_table valid
#define MB_FLAG_LOADER_NAME (1 << 9)   // boot_loader_name valid
#define MB_FLAG_APM         (1 << 10)  // apm_table valid
#define MB_FLAG_VBE         (1 << 11)  // vbe_* fields valid
#define MB_FLAG_FRAMEBUFFER (1 << 12)  // framebuffer_* fields valid

/* framebuffer_type values */
#define MB_FB_TYPE_INDEXED  0
#define MB_FB_TYPE_RGB      1
#define MB_FB_TYPE_EGA_TEXT 2

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;            // KB below 1MB
    uint32_t mem_upper;            // KB above 1MB
    uint32_t boot_device;
    uint32_t cmdline;              // physical addr of string
    uint32_t mods_count;
    uint32_t mods_addr;
    union {
        struct { uint32_t tabsize, strsize, addr, reserved; } aout;
        struct { uint32_t num, size, addr, shndx; } elf;
    } syms;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;     //physical addr of string
    uint32_t apm_table;
    uint32_t vbe_control_info;     // physical addr of VbeInfoBlock
    uint32_t vbe_mode_info;        // physical addr of ModeInfoBlock
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;    // bytes per row
    uint32_t framebuffer_width;    // pixels
    uint32_t framebuffer_height;   // pixels
    uint8_t  framebuffer_bpp;      // bits per pixel
    uint8_t  framebuffer_type;     // MB_FB_TYPE_*

    /* color info (offset 110, layout depends on framebuffer_type) */
    union {
        struct {
            uint32_t palette_addr;
            uint16_t palette_num_colors;
        } indexed;
        struct {
            uint8_t red_field_position;
            uint8_t red_mask_size;
            uint8_t green_field_position;
            uint8_t green_mask_size;
            uint8_t blue_field_position;
            uint8_t blue_mask_size;
        } rgb;
    } color_info;
} __attribute__((packed));

#endif
