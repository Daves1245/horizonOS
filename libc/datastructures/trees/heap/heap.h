#ifndef CUP_HEAP_H
#define CUP_HEAP_H

#include <stdlib.h>

#define HEAP_SIZE 100
#define HEAP_MAX_ELEM_VALUE 1000
#define HEAP_MIN_ELEM_VALUE 1
#define ROOT 1

struct heap {
	int *arr; /* Array of integers */
	size_t arr_len; /* Length of integer array */
	int heap_size; /* Size of heap */
	int (*comp)(int, int); /* Comparator function */
};

struct abstract_heap {
	void *arr; /* Array of elements */
	size_t elen; /* Length in bytes of an element */
	int arr_len; /* Length of array */
	int heap_size; /* Size of heap */
	int (*comp)(void *, void *); /* comparator function */
};

/* Pop max/min element from heap */
int heap_pop(int *arr, struct heap *hctx);
/* Heapify an array */
void heapify(int *arr, struct heap *hctx, int i);
/* Build a heap with comparator func */
void build_heap(int *arr, struct heap *hctx);
/* Perculate an item down the heap */
void percolateDown(int *arr, struct heap *hctx, int i);
/* Perculate an item up the heap */
void percolateUp(int *arr, struct heap *hctx, int i);
/* Insert an element into the heap */
int heap_insert(int *arr, struct heap *hctx, int val);
/* Delete an element from the heap */
int heap_extract(int *arr, struct heap *hctx);
/* Swap two elements in an array */
void swap(int *arr, int a, int b);

/* - - -  Abstract variations - - - */
void *abstract_heap_pop(void *arr, struct abstract_heap *ahctx);
int abstract_heapify(struct abstract_heap *ahctx);
void abstract_build_heap(struct abstract_heap *ahctx);
void __abstract_percolate_down(struct abstract_heap *ahctx, int i);
int abstract_heap_insert(struct abstract_heap *ahctx, void *elem);
void *abstract_heap_extract(struct abstract_heap *ahctx);

/* Sort array using a heap */
void heapsort(int *arr, struct heap *hctx);

#endif
