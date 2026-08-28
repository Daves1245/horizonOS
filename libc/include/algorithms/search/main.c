#include <stdio.h>

#include "binary.c"
#include "linear.c"

#define LIMIT 100

int main(void) {
	for (int i = 0; i < LIMIT; i++) {
		int arr[i];
		for (int j = 0; j < i; j++) {
			arr[j] = j + 1;
		}
		for (int j = 0; j < i; j++) {
			printf("%d ", linear_search(arr, i, j + 1) + 1);
		}
		printf("\n");
	}
	return 0;
}
