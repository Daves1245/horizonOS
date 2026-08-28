#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "heap.h"

void swap(int *arr, int a, int b) {
	int tmp = arr[a];
	arr[a] = arr[b];
	arr[b] = tmp;
}

int parent(int i) {
	return i / 2;
}

int left(int i) {
	return 2 * i;
}

int right(int i) {
	return 2 * i + 1;
}

/* Percolate an element up the heap */
void percolateUp(int *arr, struct heap *hctx, int i) {
	while (i != ROOT && !hctx->comp(arr[i], arr[parent(i)])) {
		swap(arr, i, parent(i));
		i = parent(i);
	}
}

/* Percolate an element down the heap */
void percolateDown(int *arr, struct heap *hctx, int i) {
	int swapi = i;
	if (left(i) <= hctx->heap_size && !hctx->comp(arr[i], arr[left(i)])) {
		swapi = left(i);
	}
	if (right(i) <= hctx->heap_size &&
	    !hctx->comp(arr[swapi], arr[right(i)])) {
		swapi = right(i);
	}

	if (i != swapi) {
		swap(arr, i, swapi);
		percolateDown(arr, hctx, swapi);
	}
}

void _percolateDown(int *arr, struct heap *hp, int i) {
	int swapi = i;
	if (left(i) <= hp->heap_size && hp->comp(arr[i], arr[left(i)])) {
		swapi = left(i);
	}
	if (right(i) <= hp->heap_size && hp->comp(arr[swapi], arr[right(i)])) {
		swapi = right(i);
	}
	if (swapi != i) {
		swap(arr, i, swapi);
		_percolateDown(arr, hp, swapi);
	}
}

/* Insert an element into the heap */
int heap_insert(int *arr, struct heap *hctx, int val) {
	if (hctx->heap_size == hctx->arr_len) {
		// TODO set an error state
		printf("HEAP REACHED CAPACITY, CANNOT ADD TO HEAP\n");
		return -1;
	}
	hctx->heap_size++;
	arr[hctx->heap_size] = val;
	percolateUp(arr, hctx, hctx->heap_size);
}

/* Remove an element from the heap */
int heap_pop(int *arr, struct heap *hctx) {
	int ret = -1;
	if (hctx->heap_size == 0) {
		printf("heap empty, cannot extract\n");
		// TODO set an error state
		return ret;
	}

	ret = arr[ROOT];
	swap(arr, ROOT, hctx->heap_size);
	// ret = arr[ROOT] = arr[hctx->heap_size];
	hctx->heap_size--;
	_percolateDown(arr, hctx, ROOT);
	return ret;
}

/* Build a heap from an array */
void build_heap(int *arr, struct heap *hctx) {
	hctx->heap_size = hctx->arr_len;
	for (int i = hctx->heap_size; i >= ROOT; i--) {
		percolateDown(arr, hctx, i);
	}
}

/* Apply heap sort to an array */
void heapsort(int *arr, struct heap *hctx) {
	build_heap(arr, hctx);
	for (int i = hctx->arr_len; i >= ROOT; i--) {
		swap(arr, i, ROOT);
		hctx->heap_size--;
		percolateDown(arr, hctx, ROOT);
	}
}

/* heap_pop for any suitable array  */
void *abstract_heap_pop(void *arr, struct abstract_heap *ahctx) {
}

int abstract_heapify(struct abstract_heap *ahctx) {
}

void abstract_build_heap(struct abstract_heap *ahctx) {
}

void __abstract_percolate_down(struct abstract_heap *ahctx, int i) {
}

int abstract_heap_insert(struct abstract_heap *ahctx, void *elem) {
}

void *abstract_heap_extract(struct abstract_heap *ahctx) {
}
