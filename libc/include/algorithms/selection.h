int min_index(int arr[], int low, int high) {
	int ret = low, val = arr[ret], i;
	for (i = low; i < high; i++) {
		if (arr[i] < val) {
			ret = i;
			val = arr[ret];
		}
	}

	return ret;
}

void selection_sort(int arr[], size_t size) {
	for (int i = 0; i < size; i++) {
		swap(&arr[i], &arr[min_index(arr, i, size)]);
	}
}
