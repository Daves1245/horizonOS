#ifndef VECTOR_H
#define VECTOR_H

/* Initial vector capacity */
#define VECTOR_INIT_CAPACITY 10

struct vector {
	void **items; /* The internal array of the vector */
	int capacity; /* The amount of elements the vector can currently keep track of */
	int size; /* The current number of elements in the vector */
};

/* TODO move to separate file */
typedef struct vector vector;

/* Grab the pointer to an allocated vector */
vector *vector_new(); // Dynamic allocation is needed for this data structure

/* Dispose of a vector */
void vector_delete(vector *);

/* Get the number of components in a vector */
int vector_size(vector *);

/* Grab a pointer to the element in the vector with index i */
void *vector_get(vector *, int);

/* Boolean if the vector contains no element */
int vector_empty(vector *);

/* Resize the vector to a certain capacity */
void vector_resize(vector *, int);

/* Push an element into the end of a vector */
void vector_add(vector *, void *);

/* Set the element at index i */
void vector_set(vector *, int, void *);

/* Remove the element at index i from the vector */
void vector_rm(vector *, int);

#endif
