#include "rotate.h"
#include "tree.h"

/*      n                   m
 *     / \                 / \
 *    a   m       =>      n   c
 *       / \             / \
 *      b   c           a   b 
 */
int left_rotate(struct tree *n) {
	if (!n) {
		return -1;
	}
	struct tree *m = n->right;
	if (!m) {
		return -1;
	}
	n->right = m->left;
	if (n->right) {
		n->right->parent = n;
	}
	m->parent = n->parent;
	if (n->parent) {
		if (n == n->parent->left) {
			n->parent->left = m;
		} else {
			n->parent->right = m;
		}
	}
	m->left = n;
	n->parent = m;
	return 0;
}

/*      n                   m
 *     / \                 / \
 *    m   c       =>      a   n
 *   / \                     / \
 *  a   b                   b   c
 */
// TODO update root of a tree when rotating?
int right_rotate(struct tree *n) {
	if (!n) {
		return -1;
	}
	struct tree *m = n->left;
	if (!m) {
		return -1;
	}
	n->left = m->right;
	if (n->left) {
		n->left->parent = n;
	}
	m->parent = n->parent;
	if (n->parent) {
		if (n == n->parent->left) {
			n->parent->left = m->left;
		} else {
			n->parent->right = m;
		}
	}
	m->right = n;
	n->parent = m;
	return 0;
}
