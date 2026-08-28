#include <stdio.h>
#include <stdlib.h>

#include "list.h"

struct mylinkedints {
	int data;
	struct list ll;
};

struct dsops {
	void (*ins)(struct list *, struct list *);
	void (*rm)(struct list *);
};

struct dsctx {
	void *ds;
	struct dsops ops;
};

void mystack_push(struct list *stackbase, struct list *elem) {
	list_add(stackbase, elem);
}

struct list *mystack_pop(struct list *base) {
	if (base->prev == base) {
		return NULL; // Cannot pop from an empty stack
	}
	struct list *ret = base->prev;
	list_rm(ret);
	return ret;
}

int main(int argc, char **argv) {
	struct mylinkedints llist, lstack;

	struct dsctx contexes[] = { { &llist, list_add, list_rm },
				    { &lstack, list_add, list_rm } };

	return 0;
}
