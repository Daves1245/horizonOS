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

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
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
	terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + c] = ' ';
    }
}

void terminal_scrolln(int rows) {
    for (int i = 0; i < rows; i++) {
	terminal_scroll();
    }
}

void terminal_putchar(char c) {
    unsigned char uc = c;
    if (c == '\n') {
	if (terminal_row >= VGA_HEIGHT - 1) {
	    terminal_scroll();
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
    for (size_t i = 0; i < size; i++)
	terminal_putchar(data[i]);
}

void terminal_write_dec(u32int n) {
    // 10 digits to cover unsigned 2^32 bits
    int tmp[10] = {0};
    int i = 0;

    do {
	tmp[i++] = n % 10;
	n /= 10;
    } while (n);
    do {
	terminal_putchar(tmp[--i]);
    } while (i);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}
