#include "serial.h"

// Serial stub for x86_64
// TODO: Implement proper serial support

int serial_initialized = 0;

void init_serial(void) {
    serial_initialized = 0;  // Disabled for now
}

void serial_putchar(char c) {
    (void)c;
    // No-op
}
