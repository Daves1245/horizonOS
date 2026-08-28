#include <stdio.h>
#include <stdlib.h>

#include "priority_queue.h"

int main(void) {
	priority_queue *q = pq_new();
	pq_push(q, 1, 3);
	pq_push(q, 2, 2);
	pq_push(q, 3, 1);

	while (!pq_empty(q)) {
		printf("%d\n", pq_pop(q));
	}

	pq_delete(q);
	return 0;
}
