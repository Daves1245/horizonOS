int linear_search(int arr[], int n, int elem) {
	for (register int i = 0; i < n; i++) {
		if (arr[i] == elem) {
			return i;
		}
	}

	return -1;
}
