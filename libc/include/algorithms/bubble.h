void bubble_sort(int arr[], size_t size) {
	for (int i = 0; i < size - 1; i++) {
		for (int j = 0; j < size - (i + 1); j++) {
			if (arr[j] > arr[j + 1]) {
				swap(&arr[j], &arr[j + 1]);
			}
		}
	}
}
