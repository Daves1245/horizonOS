static void __radix_count_sort(int *arr, int *b, int len, int index, int base) {
	int *c = malloc(sizeof(int) * base);
	for (int i = 0; i < len; i++) {
		c[(arr[i] / index) % base]++;
	}
	for (int i = 1; i < base; i++) {
		c[i] += c[i - 1];
	}
	for (int i = len - 1; i >= 0; i--) {
		b[c[(arr[i] / index) % base] - 1] = arr[i];
		c[(arr[i] / index) % base]--;
	}

	memcpy(arr, b, sizeof(int) * len);
}

void radix_sort(int *arr, int *b, int len, int base, int max_element) {
	for (int i = 1; max_element / i > 0; i *= 10) {
		__radix_count_sort(arr, b, len, i, 10);
	}
}
