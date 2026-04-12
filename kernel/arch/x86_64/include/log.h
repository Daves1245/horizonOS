#ifndef LOG_H

#include <stdarg.h>

// consecutive log levels
enum log_level {
		KERN_DEBUG,
		KERN_INFO,
		KERN_WARN,
		KERN_ERROR,
		KERN_FATAL
} log_level;

void printk(enum log_level, const char *fmt, ...);
void set_log_level(enum log_level);

#endif
