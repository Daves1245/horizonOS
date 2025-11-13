#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../../interrupts/isr.h"

void setup_keyboard_irq();
void keyboard_interrupt_handler(struct interrupt_context *);
void init_keyboard();

#endif
