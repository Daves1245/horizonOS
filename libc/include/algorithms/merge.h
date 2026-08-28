static void merge(int *arr, int l, int m, int r, int *left, int *right) {
	int a, b, c;

	int n1 = m - l + 1;
	int n2 = r - m;

	for (int i = 0; i < n1; i++) {
		left[i] = arr[l + i];
	}
	for (int j = 0; j < n2; j++) {
		right[j] = arr[m + 1 + j];
	}

	a = 0;
	b = 0;
	c = l;
	while (a < n1 && b < n2) {
		if (left[a] <= right[b]) {
			arr[c] = left[a];
			a++;
		} else {
			arr[c] = right[b];
			b++;
		}
		c++;
	}

	while (a < n1) {
		arr[c] = left[a];
		a++;
		c++;
	}

	while (b < n2) {
		arr[c] = right[b];
		b++;
		c++;
	}
}

static void _merge_sort(int *arr, int l, int r, int *left, int *right) {
	if (l < r) {
		int m = l + (r - l) / 2;

		_merge_sort(arr, l, m, left, right);
		_merge_sort(arr, m + 1, r, left, right);

		merge(arr, l, m, r, left, right);
	}
}

void merge_sort(int *arr, size_t len, int *left, int *right) {
	_merge_sort(arr, 0, len, left, right);
}
