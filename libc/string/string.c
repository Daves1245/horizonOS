#include <string.h>

int memcmp(const void *src, const void *dest, size_t n) {
  const char *_src = src, *_dest = dest;
  int ret;
  for (ret = 0; ret < n && (*_src++ == *_dest++); ret++);
  return ret;
}

// areas must not overlap
void *memcpy(void *dest, const void *src, size_t n) {
  const char *_src = src;
  char *_dest = dest;
  for (int i = 0; i < n; i++) {
    *_dest++ = *_src++;
  }
  return dest;
}

// areas may overlap. acts as src -> buffer -> dest
void *memmove(void *dest, const void *src, size_t n) {
  const char *_src = src;
  char *_dest = dest;

  if (_dest < _src) {
    for (size_t i = 0; i < n; i++) {
      _dest[i] = _src[i];
    }
  } else {
    for (size_t i = n; i > 0; i--) {
      _dest[i-1] = _src[i-1];
    }
  }
  return dest;
}

void *memset(void *dest, int val, size_t n) {
  char *_dest = dest;
  for (int i = 0; i < n; i++) {
    *_dest++ = val;
  }
  return dest;
}

size_t strlen(const char *str) {
  int ret = 0;
  while (*str++) ret++;
  return ret;
}

size_t strnlen(const char *str, size_t n) {
  int ret = 0;
  for (int i = 0; i < n; i++) {
    if (!*str++) break;
    ret++;
  }
  return ret;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] == '\0' || s2[i] == '\0') {
      return s1[i] - s2[i];
    }
    if (s1[i] != s2[i]) {
      return s1[i] - s2[i];
    }
  }
  return 0;
}

void itoa(char *dest, int num) {
  // TODO
}

void itoa_hex(char *dest, unsigned int num) {
  // TODO
}
