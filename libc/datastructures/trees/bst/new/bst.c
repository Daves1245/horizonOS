#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bst.h"

#define MAX_STRING_SIZE 10

struct bst {
	struct tree node;
	void *data;
};

int comparator(void *a, void *b) {
	return atoi(((struct bst *)a)->data) - atoi(((struct bst *)b)->data);
}

void *container_of(struct tree *t) {
	return __container_of(t, struct bst, node);
}

void print_data(void *t) {
	printf("%s ", (const char *)((struct bst *)t)->data);
}

void delete(void *t) {
	struct bst *root = (struct bst *)container_of(t);
	free(root->data);
	if (root->node.parent !=
	    &root->node) { /* Root isn't allocated in this example, skip its free */
		free(root);
	}
}

int main(int argc, char **argv) {
	struct bst root = { BST_INIT(root.node), NULL };

	if (argc < 2) {
		fprintf(stderr, "usage: %s [number of elements]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	root.data = malloc(MAX_STRING_SIZE);
	scanf("%s", (char *)root.data);

	int n = atoi(argv[1]);
	for (int i = 0; i < n - 1; i++) {
		struct bst *tmp;
		char buff[10];
		int num;

		tmp = malloc(sizeof(struct bst));
		tmp->data = malloc(MAX_STRING_SIZE);
		scanf("%d", &num);
		sprintf(buff, "%d", num);
		strcpy(tmp->data, buff);
		/* Only possible if struct tree is the first member of struct bst */
		bst_ins(&root.node, &tmp->node, container_of, comparator);
	}

	bst_do_inorder(&root.node, container_of, print_data);
	puts("");
	bst_do_postorder(&root.node, container_of, delete);
	return 0;
}
