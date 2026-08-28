#include <stdio.h>
#include <stddef.h>

#include "bst.h"

static void print_tree(node *n) {
	printf("%d ", n->data);
	if (n->next) {
		node *iterator = n->next;
		while (iterator) {
			printf("%d ", iterator->data);
			iterator = iterator->next;
		}
	}
}

#define LIMIT 10

int main(void) {
	node *root = bst_new(LIMIT / 2);

	for (int i = 0; i < LIMIT; i++) {
		bst_insert(root, i);
	}

	bst_inorder(print_tree, root);

	int n;
	while (scanf("%d", &n)) {
		node *elem = bst_find(root, n);
		if (elem) {
			printf("%d ", elem->data);
		} else {
			printf("not in tree!");
		}
	}

	return 0;
}
