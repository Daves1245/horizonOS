#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "heap.h"
#include "benchmarking.h"

int left(int);
int right(int);

void print_heap_err_diagnostics(struct heap *hp, struct sort_context *sctx) {
	printf("Heap property violated!\n"
	       "At nodes (p, l, r) = (%d, %d, %d)\n"
	       "With indices (pi, li, ri) = (%d, %d, %d)\n",
	       hp->arr[sctx->failed[0]], hp->arr[sctx->failed[1]],
	       hp->arr[sctx->failed[2]], sctx->failed[0], sctx->failed[1],
	       sctx->failed[2]);
}

void *assurealloc(size_t len) {
	void *ret = malloc(len);
	if (!ret) {
		fprintf(stderr,
			"[assurealloc]: failed to allocate memory.\nAbort.\n");
		exit(EXIT_FAILURE);
	}
	return ret;
}

int rand_in_range(int min, int max) {
	return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

void fill_randomly(int *arr, struct heap *ctx) {
	arr[0] = -1;
	for (int i = ROOT; i <= ctx->arr_len; i++) {
		arr[i] =
			rand_in_range(HEAP_MIN_ELEM_VALUE, HEAP_MAX_ELEM_VALUE);
	}
}

int is_sorted(int *arr, struct sort_context *ctx) {
	if (arr) {
		ctx->arr = arr;
	}
	for (int i = 1; i < ctx->len - 1; i++) {
		if (!ctx->comp(ctx->arr[i], ctx->arr[i + 1])) {
			ctx->failed[0] = i;
			ctx->failed[1] = i + 1;
			return 0;
		}
	}

	memset(ctx->failed, -1, sizeof(int) * 3);
	return 1;
}

int is_heap(struct heap *hp, struct sort_context *sctx) {
	for (int i = ROOT; i < hp->heap_size; i++) {
		if (left(i) <= hp->heap_size) {
			if (!sctx->comp(hp->arr[i], hp->arr[left(i)])) {
				sctx->failed[0] = i;
				sctx->failed[1] = left(i);
				sctx->failed[2] = right(i);
				return 0;
			}
		}
		if (right(i) <= hp->heap_size) {
			if (!sctx->comp(hp->arr[i], hp->arr[right(i)])) {
				sctx->failed[0] = i;
				sctx->failed[1] = left(i);
				sctx->failed[2] = right(i);
			}
		}
	}
	memset(sctx->failed, -1, sizeof(int) * 3);
	return 1;
}
