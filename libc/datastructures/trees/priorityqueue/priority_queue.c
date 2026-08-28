#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "priority_queue.h"

#define DEFAULT_DATA 0
#define DEFAULT_PRIORITY -1

pq_node *new_node(int data, int priority) {
	pq_node *ret = malloc(sizeof(pq_node));
	ret->data = data;
	ret->priority = priority;
	return ret;
}
priority_queue *pq_new() {
	priority_queue *ret = malloc(sizeof(priority_queue));
	ret->head = NULL;
	ret->size = 1;
	return ret;
}
void pq_delete(priority_queue *queue) {
	register pq_node *iterator = *(queue->head);
	while (iterator->next) {
		iterator = iterator->next;
		free(iterator->prev);
	}
	free(iterator);
}
/* 
* push an element into the priority queue. This will
* put the data into its appropriate place determined
* by its priority.
*/
void pq_push(priority_queue *queue, int data, int priority) {
	queue->size++;
	register pq_node *iterator = *(queue->head);
	pq_node *elem = new_node(data, priority);
	if (!iterator) {
		*(queue->head) = elem;
	}
	if (iterator->priority > priority) {
		iterator->next = *(queue->head);
		*(queue->head) = iterator;
	} else {
		while (iterator->next && iterator->next->priority < priority) {
			iterator = iterator->next;
		}
		elem->next = iterator->next;
		iterator->next = elem;
	}
}

// return the data from the head of the list
// and remove it from the list
int pq_pop(priority_queue *queue) {
	if (*(queue->head) && *(queue->head)->next) {
		int ret = *(queue->head)->data;
		*head = *(queue->head)->next;
		free(*(queue->head)->prev);
		queue->size--;
		return ret;
	}
}

int pq_peek(priority_queue *queue) {
	return *(queue->head)->data;
}

// at the moment we define an empty priority queue
// as a linked list with one element that has data == 0
int pq_empty(priority_queue *queue) {
	return queue->size == 0;
}
int pq_size(priority_queue *queue) {
	return queue->size;
}
