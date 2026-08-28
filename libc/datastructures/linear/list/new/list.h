#ifndef LIST_H
#define LIST_H

#include <assert.h>
#include <stddef.h>

#define container_of(ptr, type, member)                            \
	({                                                         \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); \
	})

#define INIT_HEAD(list)                \
	do {                           \
		(list).next = &(list); \
		(list).prev = &(list); \
	} while (0);

struct list {
	struct list *prev, *next;
};

void list_add(struct list *head, struct list *elem);
void list_rm(struct list *elem);

#endif
