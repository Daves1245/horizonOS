#include "log.h"
#include <drivers/console.h>
#include <drivers/graphics.h>
#include <drivers/serial.h>

enum log_level log_level = KERN_DEBUG;

void set_log_level(enum log_level level) {
    log_level = level;
}

void printk(enum log_level level, const char *fmt, ...) {
    /*
     * dynamically filter out noise
     * from the C11 standard, 6.7.2.2:
     * >Each subsequent enumerator with no = has a value one greater than the previous enumerator.
     */
    if (level < log_level) {
        return;
    }

    const char *tag;
    switch (level) {
        case KERN_DEBUG:
            tag = "[DBUG]:";
            console_set_fg(DEBUG_FG);
            break;
        case KERN_INFO:
            tag = "[INFO]:";
            console_set_fg(INFO_FG);
            break;
        case KERN_OK:
            tag = "[ OK ]:";
            console_set_fg(OK_FG);
            console_set_fg(WARN_FG);
            break;
        case KERN_ERROR:
            tag = "[ERROR]:";
            console_set_fg(ERROR_FG);
            break;
        case KERN_FATAL:
            tag = "[FATAL]:";
            console_set_bg(FATAL_BG);
            console_set_fg(FATAL_FG);
            break;
    }
    console_puts(tag);
    serial_write(tag);

    va_list console_args;
    va_start(console_args, fmt);
    va_list serial_args;
    va_copy(serial_args, console_args);

    console_vprintf(fmt, console_args);
    serial_vprintf(fmt, serial_args);

    va_end(serial_args);
    va_end(console_args);

    console_set_fg(INFO_FG);
    console_set_bg(rgb(0x00, 0x00, 0x00));
}
