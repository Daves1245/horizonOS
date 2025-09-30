#include "string.h"

uint32_t strncmp(const char *src, const char *dest, uint32_t len) {
    for (int i = 0; i < len; i++) {
        if (src[i] == '\0') return -1;
        if (dest[i] == '\0') return 1;
        if (src[i] != dest[i]) {
            return src[i] - dest[i];
        }
    }
    return 0;
}

uint32_t strlen(const char *str) {
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}

void memset(const void *src, const uint32_t bytes, uint32_t len) {
    unsigned char *start = (unsigned char *) src;
    unsigned char value = bytes;

    for (unsigned char *it = (unsigned char *) src; len; it++) {
        *it = value;
        len--;
    }
}
