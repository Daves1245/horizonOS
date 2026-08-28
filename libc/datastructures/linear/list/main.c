#include <stdio.h>
#include <stdlib.h>

#include <dsa/list.h>
#include "colors.h" // fun colors!

/*
 * Here is the beauty of linked lists: they are so incredibly versatile.
 * I didn't know exactly by how much until I started reading about the linux kernel.
 * Below is my implementation of stacks and queues using a modified version of the
 * kernel's linked list implementation.
 */

/*
 * Now this is truly interesting. Instead of making linked lists that contain pointers
 * to the structure, the linux kernel generalizes the linked list by making structures
 * contain lists, and then by the magic of the container_of() macro (used in the definition
 * of list_entry() used below), we are able to grab the struct that contains the linked
 * list entry given. By this method, we can completely separate the idea of structures
 * and lists.
 */
struct element {
	char *data;
	struct list_head list;
};

void welcome() {
	printf(ANSI_GREEN(
		"Welcome to the data structures tester!\n" ANSI_RESET));
}

void _print(struct list_head *list, const char *ds_name) {
	if (list_empty(list)) {
		printf(ANSI_YELLOW("Empty %s, cannot print!\n" ANSI_RESET),
		       ds_name);
		return;
	}
	struct list_head *iterator;
	list_for_each(iterator, list) {
		printf(ANSI_MAGENTA("%s: %d " ANSI_RESET), ds_name,
		       list_entry(iterator, struct element, list)->data);
	}
	printf("\n");
}

void print(struct list_head *list, struct list_head *stack,
	   struct list_head *queue) {
	_print(list, "list");
	_print(stack, "stack");
	_print(queue, "queue");
}

void linked_list_add(struct list_head *list) {
	struct element *elem = malloc(sizeof(struct element));
	printf("Enter number to add to list: ");
	scanf("%d", &elem->data);
	list_add(&elem->list, list);
	printf(ANSI_GREEN("Number %d added to list\n" ANSI_RESET), elem->data);
}

void linked_list_delete(struct list_head *list) {
	if (!list_empty(list)) {
		struct element *sp =
			list_entry(list->next, struct element, list);
		int tmp = sp->data;
		list_del(list->next);
		free(sp);
		printf(ANSI_GREEN("Deleted %d from list\n"), tmp);
	} else {
		printf(ANSI_YELLOW("List is empty, cannot delete!\n"));
	}
	printf(ANSI_RESET);
}

void stack_push(struct list_head *stack) {
	printf("Enter number to add to stack: ");
	struct element *elem = malloc(sizeof(struct element));
	scanf("%d", &elem->data);
	list_add(&elem->list, stack);
	printf(ANSI_GREEN("Added %d to stack\n"), elem->data);
}

void stack_pop(struct list_head *stack) {
	if (list_empty(stack)) {
		printf(ANSI_YELLOW(
			"Stack empty, cannot pop from it!\n" ANSI_RESET));
		return;
	}

	int ret = list_entry(stack->next, struct element, list)->data;
	list_del(stack->next);
	printf("Popped %d from the stack\n", ret);
}

int main(void) {
	welcome();

	static LIST_HEAD(list);
	static LIST_HEAD(stack);
	static LIST_HEAD(queue);

	while (1) {
		printf(ANSI_CYAN("Available options:\n" ANSI_RESET));
		printf("0. Print datastructures\n"
		       "1. Add to list\n"
		       "2. Delete from list\n"
		       "3. Push to stack\n"
		       "4. Pop from stack\n"
		       "5. Exit\n");
		int c;
		scanf("%d", &c);
		switch (c) {
		case 0:
			print(&list, &stack, &queue);
			break;
		case 1:
			linked_list_add(&list);
			break;
		case 2:
			linked_list_delete(&list);
			break;
		case 3:
			stack_push(&stack);
			break;
		case 4:
			stack_pop(&stack);
			break;
		case 5:
			goto end;
		default:
			printf(ANSI_RED(
				"Input not recognized. Try again\n" ANSI_RESET));
			break;
		}
	}

end:;
	printf(ANSI_GREEN("Have a nice day!\n" ANSI_RESET));
	return 0;
}
