#ifndef SHELL_H
#define SHELL_H

#include <kernel/graphics.h>
#include <kernel/console.h>

#define SHELL_BUFFER_SIZE 256

void shell_init(void);
void shell_run(void);

#endif
