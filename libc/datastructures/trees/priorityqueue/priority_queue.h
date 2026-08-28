#ifndef CUP_PRIORITY_QUEUE_H
#define CUP_PRIORITY_QUEUE_H

/* we represent the queue as a linked
* list with a priority attribute */
typedef struct pq_node {
	struct pq_node *prev;
	struct pq_node *next;
	int data;
	int priority;
} pq_node;

typedef struct priority_queue {
	pq_node **head;
	int size;
} priority_queue;

// creation and deletion
priority_queue *pq_new();
pq_node *new_node(int data, int priority);
void pq_delete(priority_queue *queue);
// modification
void pq_push(priority_queue *queue, int data, int priority);
int pq_pop(priority_queue *queue);
// non-modifiers
int pq_peek(priority_queue *queue);
int pq_empty(priority_queue *queue);
int pq_size(priority_queue *queue);

#endif
