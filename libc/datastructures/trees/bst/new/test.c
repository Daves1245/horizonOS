#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s [number of elements]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < atoi(argv[1]); i++) {
		char a[10], b[10];
		scanf("%s %s", a, b);

		printf("%d\n", strcmp(a, b));
	}
	return 0;
}
