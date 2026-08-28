#include "list.h"

void list_add(struct list *head, struct list *elem) {
	assert(head && elem);
	elem->next = head;
	elem->prev = head->prev;
	head->prev->next = elem;
	head->prev = elem;
}

void list_rm(struct list *elem) {
	assert(elem); // element must be non-null
	if (elem->prev) {
		elem->prev->next = elem->next;
	}
	if (elem->next) {
		elem->next->prev = elem->prev;
	}
}
