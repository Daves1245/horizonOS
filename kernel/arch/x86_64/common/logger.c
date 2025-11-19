#include <kernel/logger.h>
#include <drivers/serial.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

// x86_64 logger implementation using serial output
// VGA text mode is not available with Limine framebuffer

// Helper function to write to serial instead of putchar
static bool serial_print(const char* data, size_t length) {
    for (size_t i = 0; i < length; i++)
        serial_putchar(data[i]);
    return true;
}

static void log_prefix(enum log_type type) {
    const char *prefix;

    switch (type) {
        case LOG_INFO:
            prefix = "[INFO] ";
            break;
        case LOG_SUCCESS:
            prefix = "[ OK ] ";
            break;
        case LOG_WARNING:
            prefix = "[WARN] ";
            break;
        case LOG_ERROR:
            prefix = "[FAIL] ";
            break;
        case LOG_DEBUG:
            prefix = "[DBUG] ";
            break;
        default:
            prefix = "[????] ";
            break;
    }

    serial_write(prefix);
}

void log(enum log_type type, const char *str) {
    log_prefix(type);
    serial_write(str);
}

// Simplified printf-like formatter for serial output
void logf(enum log_type type, const char* format, ...) {
    log_prefix(type);

    va_list parameters;
    va_start(parameters, format);

    int written = 0;

    while (*format != '\0') {
        size_t maxrem = INT_MAX - written;

        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%')
                format++;
            size_t amount = 1;
            while (format[amount] && format[amount] != '%')
                amount++;
            if (maxrem < amount) {
                va_end(parameters);
                return;
            }
            if (!serial_print(format, amount)) {
                va_end(parameters);
                return;
            }
            format += amount;
            written += amount;
            continue;
        }

        const char* format_begun_at = format++;

        if (*format == 'c') {
            format++;
            char c = (char) va_arg(parameters, int);
            if (!maxrem) {
                va_end(parameters);
                return;
            }
            if (!serial_print(&c, sizeof(c))) {
                va_end(parameters);
                return;
            }
            written++;
        } else if (*format == 's') {
            format++;
            const char* str = va_arg(parameters, const char*);
            size_t len = strlen(str);
            if (maxrem < len) {
                va_end(parameters);
                return;
            }
            if (!serial_print(str, len)) {
                va_end(parameters);
                return;
            }
            written += len;
        } else if (*format == 'd') {
            format++;
            if (!maxrem) {
                va_end(parameters);
                return;
            }
            char num_buff[12] = {0};
            int num = va_arg(parameters, int);
            itoa(num_buff, num);
            size_t len = strlen(num_buff);
            if (!serial_print(num_buff, len)) {
                va_end(parameters);
                return;
            }
            written += len;
        } else if (*format == 'x') {
            format++;
            if (!maxrem) {
                va_end(parameters);
                return;
            }
            char num_buff[9] = {0};
            unsigned int num = va_arg(parameters, unsigned int);
            itoa_hex(num_buff, num);
            size_t len = strlen(num_buff);
            if (!serial_print(num_buff, len)) {
                va_end(parameters);
                return;
            }
            written += len;
        } else {
            format = format_begun_at;
            size_t len = strlen(format);
            if (maxrem < len) {
                va_end(parameters);
                return;
            }
            if (!serial_print(format, len)) {
                va_end(parameters);
                return;
            }
            written += len;
            format += len;
        }
    }

    va_end(parameters);
}

void log_demo() {
    serial_write("\n=== Color Log Demo ===\n");
    log_info("status\n");
    log_success("tests passed\n");
    log_warning("unexpected value\n");
    log_error("test failed\n");
    log_debug("debug information\n");
    serial_write("\n");
}
