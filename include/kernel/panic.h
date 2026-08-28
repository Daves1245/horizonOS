#ifndef PANIC_H
#define PANIC_H

#define panic(msg) _panic((msg), __FILE__, __LINE__, __func__)

__attribute__((noreturn))
void _panic(const char *msg, const char *file, int line, const char *func);

#endif
