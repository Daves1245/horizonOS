#include "fenwick.h"

static inline int lsbit(int i) {
	return i & (-i);
}

/* Insert an item into the Fenwick Tree. */
void insert(int *arr, int i, int value) {
	arr[i] = value;
	update(arr, i, value);
}

/* Update the values at every
 * node that is responsible for
 * the index i.
 */
void update(int *arr, size_t len, int i, int value) {
	for (; i < len; i += lsbit(i)) {
		arr[i] += value;
	}
}

/* Range sum query [1...i] */
long rsqp(int *arr, int i) {
	long ret = 0;
	for (; i; i -= lsbit(i)) {
		ret += arr[i];
	}
}

/* Range sum query [i...j] */
long rsq(int *arr, int i, int j) {
	return (i == 1) ? /* TODO handle this case */ 0 :
			  rsqp(arr, j) - rsqp(arr, i - 1);
}
