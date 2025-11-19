#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/vga.h>
#include <halt.h>
#include <drivers/io.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
#define VGA_CURSOR_LOC_HIGH 0x0E
#define VGA_CURSOR_LOC_LOW 0x0F

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
	for (size_t x = 0; x < VGA_WIDTH; x++) {
	    const size_t index = y * VGA_WIDTH + x;
	    terminal_buffer[index] = vga_entry(' ', terminal_color);
	}
    }
    update_hardware_cursor();
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void update_hardware_cursor(void) {
    uint16_t cursor_location = terminal_row * VGA_WIDTH + terminal_column;

    // high byte of cursor location to the VGA cursor address register
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOC_HIGH);
    outb(VGA_DATA_REGISTER, cursor_location >> 8);

    // low byte of cursor location to the VGA cursor address register
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOC_LOW);
    outb(VGA_DATA_REGISTER, cursor_location & 0xFF);
}

void terminal_set_cursor(size_t x, size_t y) {
    terminal_row = y;
    terminal_column = x;
    update_hardware_cursor();
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
        __asm__ volatile ("int $0x3");
        return;
    }
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        terminal_buffer[0] = vga_entry('B', terminal_color);
        terminal_buffer[1] = vga_entry('U', terminal_color);
        terminal_buffer[2] = vga_entry('G', terminal_color);
        terminal_buffer[3] = vga_entry('B', terminal_color);
        terminal_buffer[4] = vga_entry('U', terminal_color);
        terminal_buffer[5] = vga_entry('G', terminal_color);
        terminal_buffer[6] = vga_entry('B', terminal_color);
        terminal_buffer[7] = vga_entry('U', terminal_color);
        terminal_buffer[8] = vga_entry('G', terminal_color);
        __asm__ volatile ("hlt");
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
        for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            terminal_buffer[i] = vga_entry(' ', terminal_color);
        }
        return;
    }

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

    // handle backspace (keyboard driver)
    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
            update_hardware_cursor();
        }
        return;
    }

    if (c == '\n') {
        // If we're on the last row, we can't print on the next - scroll the
        // screen up and keep ourselves on the last row (newline)
	if (terminal_row >= VGA_HEIGHT - 1) {
	    terminal_scroll();
	    terminal_row = VGA_HEIGHT - 1;
	} else {
	    terminal_row++;
	}
	terminal_column = 0;
	update_hardware_cursor();
	return;
    }
    terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
    // only move up if we printed a value
    if (c != '\n') {
        terminal_column++;
        if (terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;

            // logic here is different, since we're not moving down a row
            if (terminal_row >= VGA_HEIGHT) {
                terminal_scroll();
                terminal_row = VGA_HEIGHT - 1;
            }
        }
        update_hardware_cursor();
    }
}

void terminal_putchar_at(char c, int x, int y) {
    if (x < 0 || y < 0 || x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        printf("PANIC: terminal_putchar_at bounds violation: (%d, %d)\n", x, y);
        halt();
    }
    terminal_putentryat(c, terminal_color, x, y);
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

void print_colored(const char* str, uint8_t fg, uint8_t bg) {
    uint8_t old_color = terminal_color;
    terminal_color = vga_entry_color(fg, bg);
    terminal_writestring(str);
    terminal_color = old_color;
}

