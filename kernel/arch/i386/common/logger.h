#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdio.h>

enum log_type {
	LOG_INFO,
	LOG_SUCCESS,
	LOG_WARNING,
	LOG_ERROR,
	LOG_DEBUG,
};

void log(enum log_type, const char *str);
void log_demo();

#define log_info(str)		 log(LOG_INFO, (str))
#define log_success(str) log(LOG_SUCCESS, (str))
#define log_warning(str) log(LOG_WARNING, (str))
#define log_error(str)	 log(LOG_ERROR, (str))
#define log_debug(str)	 log(LOG_DEBUG, (str))

void print_colored(const char* str, uint8_t fg, uint8_t bg);

#endif
