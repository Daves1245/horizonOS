#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Inspiration from www.codeforces.com/blog/entry/18051
 */

static inline int parent(int i) {
	return i / 2;
}

static inline int lchild(int i) {
	return 2 * i + 1;
}

static inline int rchild(int i) {
	return 2 * i + 2;
}

// this should be check if i is within [(n + 1) / 2, n)
// but due to indexing being 0 based, it is [n / 2, n - 1)
static inline int isleaf(int i, size_t n) {
	return (i >= n / 2) && (i < n);
}

static int build(int tree[], size_t n, int i) {
	if (!isleaf(i, n)) {
		tree[i] = build(tree, n, lchild(i)) + build(tree, n, rchild(i));
	}
	return tree[i];
}

// XXX - figure out if seg tree should be allocated to the heap or not
// What is the best way of storing the tree?

/* Build the segtree */
int *segtree_build(int tree[], size_t n) {
	int *ret = malloc(sizeof(*ret) * 2 * n);
	/* A segment tree is composed of 2n elements */
	for (int i = 0; i < n; i++) {
		ret[i + n - 1] =
			tree[i]; // The first n elements correspond to the tree itself
	}
	ret[0] = build(ret, 2 * n - 1,
		       0); // The next n to the array the tree represents
	return ret;
}

/* Update a value in the tree */
void segtree_update(int *tree, int i, int value) {
	int diff = value - tree[i]; // First we calculate the difference
	while (i > 0) {
		tree[i] +=
			diff; /* Now we add this difference up along the tree so that */
		i = parent(
			i); /* every necessary node is updated with the difference  */
	}
	tree[0] += diff;
}

/* Query the sum on the interval [l, r) */
int segtree_query(int *tree, size_t n, int l, int r) {
	int res = 0;
	for (l += n, r += n; l < r; l /= 2, r /= 2) {
		if (l & 1)
			res += tree[l++];
		if (r & 1)
			res += tree[--r];
	}
	return res;
}

#define LIMIT 5

void print(int arr[], size_t n) {
	for (int i = 0; i < n; i++) {
		printf("%d ", arr[i]);
	}
	printf("\n");
}

int main(int argc, char **argv) {
	printf("Size of arr: ");
	int n;
	scanf("%d", &n);
	int arr[n];
	printf("arr: ");
	for (int i = 0; i < n; i++) {
		scanf("%d", &arr[i]);
	}
	int *tree = segtree_build(arr, n);
	print(tree, 2 * n);
	free(tree);
	return 0;
}
