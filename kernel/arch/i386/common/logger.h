#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

enum log_type {
	LOG_INFO,
	LOG_SUCCESS,
	LOG_WARNING,
	LOG_ERROR,
	LOG_DEBUG,
};

void log(enum log_type, const char *str);
void logf(enum log_type type, const char *format, ...);
void log_demo();

#define log_info(fmt, ...)		logf(LOG_INFO, (fmt), ##__VA_ARGS__)
#define log_success(fmt, ...)	logf(LOG_SUCCESS, (fmt), ##__VA_ARGS__)
#define log_warning(fmt, ...)	logf(LOG_WARNING, (fmt), ##__VA_ARGS__)
#define log_error(fmt, ...)	logf(LOG_ERROR, (fmt), ##__VA_ARGS__)
#define log_debug(fmt, ...)	logf(LOG_DEBUG, (fmt), ##__VA_ARGS__)

void print_colored(const char* str, uint8_t fg, uint8_t bg);

#endif
