#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.h>

#ifdef __x86_64__
#include "../../kernel/arch/x86_64/drivers/serial.h"
#else
#include "../../kernel/arch/i386/drivers/serial.h"
#endif

extern int serial_initialized;
#endif

int putchar(int ic) {
#if defined(__is_libk)
	char c = (char) ic;
	terminal_write(&c, sizeof(c));
	// Also output to serial port for debugging
	if (serial_initialized) {
		serial_putchar(c);
	}
#else
	// TODO: Implement stdio and the write system call.
#endif
	return ic;
}
