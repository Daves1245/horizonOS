#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

#define VECTOR_TYPE void *

// Return a pointer to a newly allocated vector
vector *vector_new() {
	vector *v = malloc(sizeof(vector));
	v->capacity = VECTOR_INIT_CAPACITY;
	v->size = 0;
	v->items = malloc(sizeof(void *) * v->capacity);
	return v;
}

// Deallocate memory used for vector
void vector_delete(vector *v) {
	free(v->items);
	free(v);
}

// Size of a vector
int vector_size(vector *v) {
	return v->size;
}
int vector_empty(vector *v) {
	return v->size == 0;
}

// Resize a vector
void vector_resize(vector *v, int capacity) {
	void **items = realloc(v->items, sizeof(void *) * capacity);
	if (items) {
		v->items = items;
		v->capacity = capacity;
	}
}

// Add an element to a vector
void vector_add(vector *v, void *elem) {
	if (v->capacity == v->size)
		vector_resize(v, v->capacity * 2);
	v->items[v->size++] = elem;
}

// Set an element in the vector
void vector_set(vector *v, int index, void *item) {
	if (index >= 0 && index < v->size)
		v->items[index] = item;
}

// Get the element from a vector at a certain index
void *vector_get(vector *v, int index) {
	if (index >= 0 && index < v->size)
		return v->items[index];
	return NULL;
}

// Remove an element from a vector
void vector_rm(vector *v, int index) {
	if (index < 0 || index >= v->size) {
		return; // we're not in a valid range
	}

	/* 'Remove' the element at index, and shift every
   *  element onwards back */
	v->items[index] = NULL;
	for (int i = index; i < v->size - 1; i++) {
		v->items[i] = v->items[i + 1];
		v->items[i + 1] = NULL;
	}

	v->size--;

	if (v->size > 0 && v->size == v->capacity / 4) {
		vector_resize(v, v->capacity / 2);
	}
}
