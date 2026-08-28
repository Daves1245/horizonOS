void insertion_sort(int *arr, size_t size) {
	for (int i = 1; i < size; i++) {
		int j = i - 1, key = arr[i];

		while (j >= 0 && arr[j] > key) {
			arr[j + 1] = arr[j];
			j--;
		}
		arr[j + 1] = key;
	}
}
