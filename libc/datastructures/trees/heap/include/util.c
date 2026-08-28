#include <stdlib.h>
#include <stdio.h>

#include "util.h"

int left(int);
int right(int);

int rand_in_range(int min, int max) {
	return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

void fill_with_random(int *arr, struct heap *ctx) {
	arr[0] = -1;
	for (int i = ROOT; i <= ctx->arr_len; i++) {
		arr[i] =
			rand_in_range(HEAP_MIN_ELEM_VALUE, HEAP_MAX_ELEM_VALUE);
	}
}

void *assurealloc(size_t len) {
	void *p = malloc(len);
	if (!p) {
		fprintf(stderr,
			"[assurealloc]: failed to allocate memory.\nAbort\n");
		exit(EXIT_FAILURE);
	}
}

int sort_check(void *arr, int arrlen, size_t elem_size,
	       int (*comp)(void *, void *)) {
	char *a = (char *)arr;
	for (int i = 1; i <= arrlen - 1; i++) {
		if (!comp(&a[i * elem_size], &a[(i + 1) * elem_size])) {
			printf("Failed at index i = %d, a[i] > a[i + 1] = %d, %d\n",
			       i, ((int *)arr)[i], ((int *)arr)[i + 1]);
			for (int i = 1; i <= arrlen; i++) {
				printf("%d ", ((int *)arr)[i]);
			}
			puts("");
			return 0;
		}
	}
	return 1;
}

int is_heap(void *arr, int heap_size, size_t elem_size,
	    int (*comp)(void *, void *)) {
	char *a = (char *)arr;
	for (int i = 1; left(i) <= heap_size || right(i) <= heap_size; i++) {
		if (left(i) > heap_size || right(i) > heap_size) {
			return 1;
		}

		if (!comp(&a[i * elem_size], &a[left(i) * elem_size])) {
			printf("FAILED AT INDEX i = %d, l(i) = %d\n", i,
			       left(i));
			printf("VALUES AT i, left(i) = %d, %d\n",
			       ((int *)arr)[i], ((int *)arr)[left(i)]);
			printf("i vs left(i): %d\n",
			       comp(&a[i * elem_size],
				    &a[(left(i) * elem_size)]));
			return 0;
		}

		if (!comp(&a[i * elem_size], &a[right(i) * elem_size])) {
			printf("FAILED AT INDEX i = %d, r(i) = %d\n", i,
			       right(i));
			printf("VALUES AT i, right(i) = %d, %d\n",
			       ((int *)arr)[i], ((int *)arr)[right(i)]);
			printf("i vs right(i): %d\n",
			       comp(&a[i * elem_size],
				    &a[right(i) * elem_size]));
			return 0;
		}
	}
	return 1;
}
