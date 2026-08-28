#ifndef VBE_H
#define VBE_H

#include <stdint.h>

/* https://pdos.csail.mit.edu/6.828/2018/readings/hardware/vbe3.pdf */

// page 25 - return structure of querying for controller information
struct __attribute__((packed)) vbe_controller_info {
    char signature[4]; // db 'VESA' VBE signature
    uint16_t vbe_version; // dw 0300h version
    uint32_t oem_string; // dd ? VbeFarPtr to OEM string
    // we use an array of uint8 to make it clear which byte (some are reserved) we're using and why
    uint8_t capabilities[4]; // db 4 dup (?) capabilities of graphics controller
    uint32_t video_mode_ptr; // dd ? vbefarptr to videomodelist
    uint16_t total_mem; // number of 64kb memory blocks

    /* VBE 2.0+ */
    uint16_t oem_software_rev; // vbe implementation software revision
    uint32_t oem_vendor_name_ptr; // vbefarptr to vendor name string
    uint32_t oem_product_name_ptr; // vbefarptr to product name string
    uint32_t oem_product_rev_ptr; // vbefarptr to product revision string
    char reserved[222]; // reserved for vbe implementation scratch area
    char oem_data[256]; // data area for oem strings
};

struct __attribute__((packed)) vbe_model_info_block {
    // Mandatory information for all VBE revisions
    uint16_t mode_attributes;           // dw ? mode attributes
    uint8_t win_a_attributes;           // db ? window A attributes
    uint8_t win_b_attributes;           // db ? window B attributes
    uint16_t win_granularity;           // dw ? window granularity
    uint16_t win_size;                  // dw ? window size
    uint16_t win_a_segment;             // dw ? window A start segment
    uint16_t win_b_segment;             // dw ? window B start segment
    uint32_t win_func_ptr;              // dd ? real mode pointer to window function
    uint16_t bytes_per_scanline;        // dw ? bytes per scan line

    // Mandatory information for VBE 1.2 and above
    uint16_t x_resolution;              // dw ? horizontal resolution in pixels or characters
    uint16_t y_resolution;              // dw ? vertical resolution in pixels or characters
    uint8_t x_char_size;                // db ? character cell width in pixels
    uint8_t y_char_size;                // db ? character cell height in pixels
    uint8_t number_of_planes;           // db ? number of memory planes
    uint8_t bits_per_pixel;             // db ? bits per pixel
    uint8_t number_of_banks;            // db ? number of banks
    uint8_t memory_model;               // db ? memory model type
    uint8_t bank_size;                  // db ? bank size in KB
    uint8_t number_of_image_pages;      // db ? number of images
    uint8_t reserved1;                  // db 1 reserved for page function

    // Direct Color fields (required for direct/6 and YUV/7 memory models)
    uint8_t red_mask_size;              // db ? size of direct color red mask in bits
    uint8_t red_field_position;         // db ? bit position of lsb of red mask
    uint8_t green_mask_size;            // db ? size of direct color green mask in bits
    uint8_t green_field_position;       // db ? bit position of lsb of green mask
    uint8_t blue_mask_size;             // db ? size of direct color blue mask in bits
    uint8_t blue_field_position;        // db ? bit position of lsb of blue mask
    uint8_t rsvd_mask_size;             // db ? size of direct color reserved mask in bits
    uint8_t rsvd_field_position;        // db ? bit position of lsb of reserved mask
    uint8_t direct_color_mode_info;     // db ? direct color mode attributes

    // Mandatory information for VBE 2.0 and above
    uint32_t phys_base_ptr;             // dd ? physical address for flat memory frame buffer
    uint32_t reserved2;                 // dd 0 Reserved - always set to 0
    uint16_t reserved3;                 // dw 0 Reserved - always set to 0

    // Mandatory information for VBE 3.0 and above
    uint16_t lin_bytes_per_scanline;    // dw ? bytes per scan line for linear modes
    uint8_t bnk_number_of_image_pages;  // db ? number of images for banked modes
    uint8_t lin_number_of_image_pages;  // db ? number of images for linear modes
    uint8_t lin_red_mask_size;          // db ? size of direct color red mask (linear modes)
    uint8_t lin_red_field_position;     // db ? bit position of lsb of red mask (linear modes)
    uint8_t lin_green_mask_size;        // db ? size of direct color green mask (linear modes)
    uint8_t lin_green_field_position;   // db ? bit position of lsb of green mask (linear modes)
    uint8_t lin_blue_mask_size;         // db ? size of direct color blue mask (linear modes)
    uint8_t lin_blue_field_position;    // db ? bit position of lsb of blue mask (linear modes)
    uint8_t lin_rsvd_mask_size;         // db ? size of direct color reserved mask (linear modes)
    uint8_t lin_rsvd_field_position;    // db ? bit position of lsb of reserved mask (linear modes)
    uint32_t max_pixel_clock;           // dd ? maximum pixel clock (in Hz) for graphics mode
    uint8_t reserved4[189];             // db 189 dup (?) remainder of ModeInfoBlock
};

int vbe_get_controller_info(struct vbe_controller_info *info);

#endif
