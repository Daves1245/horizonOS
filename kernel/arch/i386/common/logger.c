#include "logger.h"
#include <drivers/console.h>
#include <drivers/graphics.h>
#include <drivers/serial.h>

/* color palette matching log.h conventions */
#define LOG_FG_INFO    rgb(0xCC, 0xCC, 0xCC)
#define LOG_FG_SUCCESS rgb(0x00, 0xCC, 0x00)
#define LOG_FG_WARN    rgb(0xFF, 0xFF, 0x00)
#define LOG_FG_ERROR   rgb(0xFF, 0x00, 0x00)
#define LOG_FG_DEBUG   rgb(0x87, 0xCE, 0xFA)
#define LOG_FG_DEFAULT rgb(0xCC, 0xCC, 0xCC)
#define LOG_BG_DEFAULT rgb(0x00, 0x00, 0x00)

/*
 * i386 cannot map the framebuffer until after paging is up, so the console
 * cannot be initialized at early boot.  Call logger_init_console() right
 * after console_init() to switch from serial-only to console+serial output.
 */
static int g_console_ready = 0;

void logger_init_console(void) {
    g_console_ready = 1;
}

static void log_prefix(enum log_type type) {
    const char *prefix;
    uint32_t fg;

    switch (type) {
        case LOG_INFO:    prefix = "[INFO] "; fg = LOG_FG_INFO;    break;
        case LOG_SUCCESS: prefix = "[ OK ] "; fg = LOG_FG_SUCCESS; break;
        case LOG_WARNING: prefix = "[WARN] "; fg = LOG_FG_WARN;    break;
        case LOG_ERROR:   prefix = "[FAIL] "; fg = LOG_FG_ERROR;   break;
        case LOG_DEBUG:   prefix = "[DBUG] "; fg = LOG_FG_DEBUG;   break;
        default:          prefix = "[????] "; fg = LOG_FG_DEFAULT; break;
    }

    if (g_console_ready) {
        console_set_fg(fg);
        console_puts(prefix);
        console_set_fg(LOG_FG_DEFAULT);
        console_set_bg(LOG_BG_DEFAULT);
    }
    serial_write(prefix);
}

void log(enum log_type type, const char *str) {
    log_prefix(type);
    if (g_console_ready)
        console_puts(str);
    serial_write(str);
}

void logf(enum log_type type, const char *format, ...) {
    log_prefix(type);

    va_list console_args;
    va_start(console_args, format);
    va_list serial_args;
    va_copy(serial_args, console_args);

    if (g_console_ready)
        console_vprintf(format, console_args);
    serial_vprintf(format, serial_args);

    va_end(serial_args);
    va_end(console_args);
}

void log_demo() {
    if (g_console_ready)
        console_printf("\n=== Color Log Demo ===\n");
    serial_write("\n=== Color Log Demo ===\n");
    log_info("This is informational text\n");
    log_success("Operation completed successfully\n");
    log_warn("This is a warning message\n");
    log_error("This is an error message\n");
    log_debug("Debug information\n");
    if (g_console_ready)
        console_printf("\n");
    serial_write("\n");
}
