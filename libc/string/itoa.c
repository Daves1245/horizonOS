#include <string.h>

void itoa(char *dest, int num) {
    memset(dest, 0, sizeof dest);
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
    memset(dest, 0, sizeof dest);
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
