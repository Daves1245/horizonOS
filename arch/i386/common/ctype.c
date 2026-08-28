#include "ctype.h"

// Character classification functions
int isprintable(char c) {
    return c >= 32 && c <= 126;  // ASCII printable range
}

int iswhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int isalpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isnum(char c) {
    return c >= '0' && c <= '9';
}

int isalphanum(char c) {
    return isalpha(c) || isnum(c);
}