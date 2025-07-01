#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
	for (size_t x = 0; x < VGA_WIDTH; x++) {
	    const size_t index = y * VGA_WIDTH + x;
	    terminal_buffer[index] = vga_entry(' ', terminal_color);
	}
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat_visual_debug(unsigned char c, uint8_t color, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        // Flash corner of screen bright red to indicate bounds error
        terminal_buffer[0] = vga_entry('!', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
        return;
    }
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putentryat_interrupt_debug(unsigned char c, uint8_t color, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        __asm__ volatile ("int $0x3"); // Debug interrupt
        return;
    }
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        // Write error message and halt
        terminal_buffer[0] = vga_entry('B', vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        terminal_buffer[1] = vga_entry('U', vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        terminal_buffer[2] = vga_entry('G', vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        __asm__ volatile ("hlt");
        return;
    }
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll() {
    for (size_t r = 0; r < VGA_HEIGHT - 1; r++) {
	for (size_t c = 0; c < VGA_WIDTH; c++) {
	    terminal_buffer[r * VGA_WIDTH + c] =
		terminal_buffer[(r + 1) * VGA_WIDTH + c];
	}
    }
    for (size_t c = 0; c < VGA_WIDTH; c++) {
	terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + c] =
            vga_entry(' ', terminal_color);
    }
}

void terminal_scrolln(size_t rows) {
    if (!rows) {
        return;
    }

    if (rows >= VGA_HEIGHT) {
        // Clear entire screen if scrolling more than screen height
        for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            terminal_buffer[i] = vga_entry(' ', terminal_color);
        }
        return;
    }

    // Move rows up by 'rows' lines in one operation
    for (size_t r = 0; r < VGA_HEIGHT - rows; r++) {
        for (size_t c = 0; c < VGA_WIDTH; c++) {
            terminal_buffer[r * VGA_WIDTH + c] =
                terminal_buffer[(r + rows) * VGA_WIDTH + c];
        }
    }

    // Clear the bottom 'rows' lines
    for (size_t r = VGA_HEIGHT - rows; r < VGA_HEIGHT; r++) {
        for (size_t c = 0; c < VGA_WIDTH; c++) {
            terminal_buffer[r * VGA_WIDTH + c] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_putchar(char c) {
    unsigned char uc = c;
    if (c == '\n') {
	if (terminal_row >= VGA_HEIGHT - 1) {
	    terminal_scroll();
	    terminal_row = VGA_HEIGHT - 1;
	} else {
	    terminal_row++;
	}
	terminal_column = 0;
	return;
    }
    terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
    terminal_column++;
    if (terminal_column == VGA_WIDTH) {
	terminal_column = 0;
	terminal_row++;
	if (terminal_row == VGA_HEIGHT) {
	    terminal_row--;
	    terminal_scroll();
	}
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
	terminal_putchar(data[i]);
    }
}

void terminal_write_dec(uint32_t n) {
    // 10 digits to cover unsigned 2^32 bits
    int tmp[11] = {0};
    int i = 0;

    do {
	tmp[i++] = n % 10;
	n /= 10;
    } while (n);
    do {
	terminal_putchar('0' + tmp[--i]);
    } while (i);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}
