#include "stack.h"

void stack_new(stack *s) {
}

int stack_push(stack *s, void *data) {
}

void *stack_pop(stack *s) {
	if (stack_empty(s)) {
		return NULL;
	}
}

int stack_empty(stack *s) {
	return list_empty(s->base);
}
