#ifndef SHELL_H
#define SHELL_H

#include <drivers/graphics.h>
#include <drivers/console.h>

#define BUFFER_SIZE 80

char input_buffer[BUFFER_SIZE];

void shell_init();
void run();

#endif
