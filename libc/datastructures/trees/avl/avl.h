#include "../include/tree.h"

struct avl {
	struct tree node;
	int height;
};

int avl_balance(struct avl *root, struct avl *leaf,
		int (*comp)(void *, void *)) {
}

int avl_ins(struct avl *root, struct avl *leaf, int (*comp)(void *, void *)) {
}
