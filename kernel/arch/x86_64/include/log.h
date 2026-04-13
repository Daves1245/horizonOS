#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

#define DEBUG_FG rgb(0x87, 0xCE, 0xFA) // light blue
#define INFO_FG rgb(0xCC, 0xCC, 0xCC) // default foreground
#define WARN_FG rgb(0xFF, 0xFF, 0x00) // yellow
#define ERROR_FG rgb(0xFF, 0x00, 0x00) // red
#define FATAL_FG rgb(0xFF, 0xFF, 0xFF) // white on red
#define OK_FG    rgb(0x00, 0xCC, 0x00)  // green
#define FATAL_BG rgb(0xCC, 0x00, 0x00)

// consecutive log levels
enum log_level {
		KERN_DEBUG,
		KERN_INFO,
		KERN_OK,
		KERN_WARN,
		KERN_ERROR,
		KERN_FATAL
};

extern enum log_level log_level;

void printk(enum log_level, const char *fmt, ...);
void set_log_level(enum log_level);

#endif
