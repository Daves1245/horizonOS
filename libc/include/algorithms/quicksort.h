static int partition(int *arr, int l, int h) {
	int p = arr[h];
	int ei = l - 1;

	for (int i = l; i < h; i++) {
		if (arr[i] <= p) {
			ei++;
			swap(&arr[ei], &arr[i]);
		}
	}
	swap(&arr[ei + 1], &arr[h]);
	return ei + 1;
}

static void _quick_sort(int *arr, int l, int h) {
	if (l < h) {
		int pi = partition(arr, l, h);
		_quick_sort(arr, l, pi - 1);
		_quick_sort(arr, pi + 1, h);
	}
}

void quick_sort(int *arr, size_t len) {
	_quick_sort(arr, 0, len - 1);
}
