#ifndef STRING_H
#define STRING_H

#include <stdint.h>

uint32_t strncmp(const char *src, const char *dest, uint32_t len);
uint32_t strlen(const char *str);
void memset(const void *src, const uint32_t bytes, uint32_t len);

#endif
