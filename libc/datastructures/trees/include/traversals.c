#include <stddef.h>

#include "traversals.h"

/*
 * Make these macros as well, that expand out
 * like for each declarations (perhaps using
 * some to-be-implemented pre/in/post-order successor
 * functions :)
 */

int do_preorder(struct tree *root, void (*func)(struct tree *)) {
	if (!root) {
		return 1;
	}
	func(root);
	if (root && root->left != root) {
		do_preorder(root->left, func);
	}
	if (root && root->right != root) {
		do_preorder(root->right, func);
	}

	return 0;
}

int do_inorder(struct tree *root, void (*func)(struct tree *)) {
	if (!root) {
		return 1;
	}

	if (root->left && root->left != root) {
		do_inorder(root->left, func);
	}
	func(root);
	if (root->right && root->left != root) {
		do_inorder(root->right, func);
	}

	return 0;
}

int do_postorder(struct tree *root, void (*func)(struct tree *)) {
	if (!root) {
		return 1;
	}
	if (root->left && root->left != root) {
		do_postorder(root->left, func);
	}
	if (root->right && root->right != root) {
		do_postorder(root->right, func);
	}
	func(root);

	return 0;
}

struct tree *inorder_successor(struct tree *node) {
	struct tree *i = node;
	if (i->right && i->right != i) {
		i = i->right;
		while (i->left && i->left != i) {
			i = i->left;
		}

		return i;
	} else {
		while (i->parent && i->parent != i) {
			if (i == i->parent->left) {
				return i->parent;
			}
			i = i->parent;
		}
	}

	return NULL;
}
