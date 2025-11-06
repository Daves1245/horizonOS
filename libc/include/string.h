#ifndef _STRING_H
#define _STRING_H

#include <sys/cdefs.h>

#include <stddef.h>

int memcmp(const void *, const void *, size_t);
void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
size_t strlen(const char *);
size_t strnlen(const char *str, size_t maxlen);
int strncmp(const char *s1, const char *s2, size_t n);
void itoa(char *dest, int num);
void itoa_hex(char *dest, unsigned int num);

#endif
