#include "log.h"

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

    switch (log_level) {
        case KERN_INFO:
            break;
        case KERN_WARN:
            break;
        case KERN_ERROR:
            break;
        case KERN_DEBUG:
            break;
        case KERN_FATAL:
            break;
    }

    va_list parameters;
    va_start(parameters, fmt);


}
