#ifndef CUP_STACK_H
#define CUP_STACK_H

struct list {
	struct list *next, *prev;
};

struct stack {
	struct list base;
};

typedef struct stack stack;

void stack_new(stack *s);

stack *stack_alloc(stack *s);
void stack_dealloc(stack *s);

int stack_push(stack *s, void *data);
void *stack_pop(stack *s);

int stack_empty(stack *s);

#endif
