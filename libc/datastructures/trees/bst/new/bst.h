#include "../../include/tree.h"

// XXX
#define BST_INIT(root) { &(root), &(root), &(root) }
#define BST_INIT_ROOT(root) struct tree(root) = { &(root), &(root), &(root) }

/*
 * XXX macro 
 * #define BST_INORDER() 
 * expands to for each-like loop
 * same for BST_POST_ORDER(), BST_PREORDER
 */

#define bst_rm(ptr, type, member, comp)                \
	do {                                           \
		bst_del(ptr, comp);                    \
	free(container_of( (ptr) , (type) , (member)); \
	} while (0)

int bst_ins(struct tree *root, struct tree *leaf,
	    void *(*container_of)(struct tree *), int (*comp)(void *, void *)) {
	struct tree *cur, *child;

	leaf->left = leaf->right = leaf;
	cur = child = root;

	while (1) {
		if (comp(container_of(leaf), container_of(child)) <= 0) {
			cur = child;
			if (child->left != child) {
				child = child->left;
			} else {
				break;
			}
		} else if (comp(container_of(leaf), container_of(child)) > 0) {
			cur = child;
			if (child->right != child) {
				child = child->right;
			} else {
				break;
			}
		}
	}

	leaf->parent = cur;
	if (comp(container_of(leaf), container_of(cur)) <= 0) {
		cur->left = leaf;
	} else if (comp(container_of(leaf), container_of(cur)) > 0) {
		cur->right = leaf;
	}

	return 0;
}

int bst_del(struct tree *n) {
	if (n->parent != n) {
		if (n == n->parent->left) {
			n->parent->left = n->right;
			n->right->parent = n;
		} else if (n == n->parent->right) {
			n->parent->right = n->left;
			n->left->parent = n->right;
		}
	}
	n->right->parent = n->right;
	n->left->parent = n->right;

	return 0;
}

int bst_do_preorder(struct tree *root, void *(container_of)(struct tree *),
		    void (*func)(void *)) {
	func(container_of(root));
	if (root->left != root) {
		bst_do_preorder(root->left, container_of, func);
	}
	if (root->right != root) {
		bst_do_preorder(root->right, container_of, func);
	}

	return 0;
}

int bst_do_inorder(struct tree *root, void *(container_of)(struct tree *),
		   void (*func)(void *)) {
	if (root->left != root) {
		bst_do_inorder(root->left, container_of, func);
	}
	func(container_of(root));
	if (root->right != root) {
		bst_do_inorder(root->right, container_of, func);
	}

	return 0;
}

int bst_do_postorder(struct tree *root, void *(container_of)(struct tree *),
		     void (*func)(void *)) {
	if (root->left != root) {
		bst_do_postorder(root->left, container_of, func);
	}
	if (root->right != root) {
		bst_do_postorder(root->right, container_of, func);
	}
	func(container_of(root));

	return 0;
}
