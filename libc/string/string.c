#include <string.h>
#include <kernel/panic.h>

#define MAX_STRCMP_COMPARISONS 1000

int memcmp(const void* aptr, const void* bptr, size_t size) {
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (size_t i = 0; i < size; i++) {
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	}
	return 0;
}

void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

void* memmove(void* dstptr, const void* srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}

void* memset(void* bufptr, int value, size_t size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

size_t strnlen(const char *str, size_t maxlen) {
	size_t len = 0;
	for (size_t i = 0; i < maxlen; i++) {
		if (!str[i])
			break;
		len++;
	}
	return len;
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

int strcmp(const char *s1, const char *s2) {
	// sanity check - let's assume relatively large strings are indicative of bugs
	for (size_t i = 0; i < MAX_STRCMP_COMPARISONS; i++) {
		if (s1[i] == '\0' || s2[i] == '\0') {
			return s1[i] - s2[i];
		}

		if (s1[i] != s2[i]) {
			return s1[i] - s2[i];
		}
	}

	panic("strcmp exceeded MAX_STRCMP_COMPARISONS");
}

void itoa(char *dest, int num) {
	char stack[12] = {0};
	int i;
	for (i = 0; i < 11; i++) {
		stack[i] = num % 10;
		num /= 10;
		if (num == 0) break;
	}
	for (int k = 0; k <= i; k++) {
		dest[k] = stack[i - k] + '0';
	}
	dest[i + 1] = '\0';
}

void itoa_hex(char *dest, unsigned int num) {
	char stack[9] = {0};  // 8 hex digits + null terminator
	int i;

	if (num == 0) {
		dest[0] = '0';
		dest[1] = '\0';
		return;
	}

	for (i = 0; i < 8 && num > 0; i++) {
		int digit = num % 16;
		if (digit < 10) {
			stack[i] = digit + '0';
		} else {
			stack[i] = digit - 10 + 'a';
		}
		num /= 16;
	}

	for (int k = 0; k < i; k++) {
		dest[k] = stack[i - 1 - k];
	}
	dest[i] = '\0';
}
