#ifndef PANIC_H
#define PANIC_H

#include <kernel/compiler.h>

#define panic(msg) _panic((msg), __FILE__, __LINE__, __func__)

__noreturn void _panic(const char *msg, const char *file,
				      int line, const char *func);

#endif
