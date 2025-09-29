#ifndef KEYBOARD_H
#define KEYBOARD_H

// register keyboard
void setup_keyboard_irq();
void keyboard_interrupt_handler(struct interrupt_context *);
void init_keyboard();
void get_ioapic_base();

#endif
