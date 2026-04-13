#ifndef SHELL_H
#define SHELL_H

#include <drivers/graphics.h>
#include <drivers/console.h>

#define SHELL_BUFFER_SIZE 256

void shell_init();
void shell_run();

#endif
