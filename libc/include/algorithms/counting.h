#include <stdlib.h>
#include <string.h>

void counting_sort(int *arr, int len, int *b, int *c, int k) {
	memset(c, 0, sizeof(int) * (k + 1));
	for (int i = 0; i < len; i++) {
		c[arr[i]]++;
	}

	for (int i = 1; i <= k; i++) {
		c[i] += c[i - 1];
	}

	for (int i = len - 1; i >= 0; i--) {
		b[c[arr[i]] - 1] = arr[i];
		c[arr[i]]--;
	}
	memcpy(arr, b, sizeof(int) * len);
}
