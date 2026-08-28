#include <stdlib.h>
#include <stdio.h>

#include "avl.h"

struct node {
	int data;
	int height;
	int balance;
	struct node *parent, *left, *right;
};

static inline int max(int a, int b) {
	return a > b ? a : b;
}

static int height(struct node *n) {
	if (!n) {
		return -1;
	}
	return n->height = max(height(n->left), height(n->right)) + 1;
}

static inline void rightrotate(struct node *n) {
	n->left->parent = n->parent;
	n->parent = n->left;
	n->left = n->left->right;
	n->parent->right = n;
}

static inline void leftrotate(struct node *n) {
	n->right->parent = n->parent;
	n->parent = n->right;
	n->right = n->right->left;
	n->parent->left = n;
}

void avl_insert(struct node *root, int data) {
	struct node *elem = malloc(sizeof(*elem));
	elem->data = data;
	elem->height = elem->weight = 0;
	elem->left = elem->right = elem->parent = NULL;

	/* BST insert */
	struct node *tmp = root;
	while (tmp->left || tmp->right) {
		if (data < tmp->data) {
			tmp->weight--;
			if (tmp->left) {
				tmp = tmp->left;
			} else {
				break;
			}
		} else {
			tmp->weight++;
			if (tmp->right) {
				tmp = tmp->right;
			} else {
				break;
			}
		}
	}
	elem->parent = tmp;
	if (elem->data <= tmp->data) {
		tmp->left = elem;
	} else {
		tmp->right = elem;
	}

	/* Fix AVL property */
	while (n->parent) {
		if (n->weight < -2) {
			// FIX ON LEFT HAND SIDE
		} else if (n->weight > 2) {
			// FIX ON RIGHT HAND SIDE
		}
	}
}

struct node *avl_remove(struct node *elem);

static inline int avl_balanced(struct node *root) {
	return root->height > -2 && root->height < 2;
}

static void balance(struct node *root) {
}

void inorder_traverse(struct node *root, void (*func)(struct node *)) {
	if (root->left) {
		inorder_traverse(root->left, func);
	}
	func(root);
	if (root->right) {
		inorder_traverse(root->right, func);
	}
}

void postorder_traverse(struct node *root, void (*func)(struct node *)) {
	if (root->left) {
		postorder_traverse(root->left, func);
	}
	if (root->right) {
		postorder_traverse(root->right, func);
	}
	func(root);
}

void printdata(struct node *n) {
	//printf("d: %d h: %d\n", n->data, n->height);
	printf("%d ", n->data);
}

int main(int argc, char **argv) {
	struct node *root = malloc(sizeof(*root));
	root->data = 41;
	avl_insert(root, 20);
	avl_insert(root, 65);
	avl_insert(root, 11);
	avl_insert(root, 29);
	avl_insert(root, 26);
	avl_insert(root, 50);
	height(root);

	inorder_traverse(root, printdata);
	return 0;
}
