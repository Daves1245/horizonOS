#ifndef BENCHMARKING_H
#define BENCHMARKING_H

struct sort_context {
	int *arr; /* Array to be checked */
	size_t len; /* Length of array */
	int failed[3]; /* Indices of failure */
	int (*comp)(int, int); /* Comparator function to use */
};

int is_sorted(int *arr, struct sort_context *ctx);
/* get a random number within a range */
int rand_in_range(int min, int max);
/* fill a an array with random values (within the heap's min and max element values) */
void fill_randomly(int *arr, struct heap *ctx);
/* exit if memory is not properly allocated. Avoids SEGFAULTs*/
void *assurealloc(size_t len);
/* Check if an array is sorted */
int is_heap(struct heap *hp, struct sort_context *sctx);
/* Print error diagnostics for heap related checks */
void print_heap_err_diagnostics(struct heap *hp, struct sort_context *sctx);

#endif
