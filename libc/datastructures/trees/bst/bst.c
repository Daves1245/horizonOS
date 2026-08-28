#include <stdio.h>
#include <stdlib.h>

#include "bst.h"

/************************************************************
*
* author: David Santamaria (davidsantamaria1245@gmail.com)
* purpose: C implementation of a BST (binary search node) 
* operations (where N is the number of nodes in the node:
        * insert O(logN)
        * delete O(logN)
        * find O(logN)
        *
        * version: 1.0.2
        * changes:
                * added documentation
                * added TODOs 
                * added duplicate data by implementing linked list
                * status: broken (fix insert) 
                * XXX add remove function
************************************************************/

// create a new tree
node *bst_new(int data) {
	node *ret = malloc(sizeof(*ret));
	if (!ret) {
		return NULL;
	}
	ret->left = ret->right = NULL;
	ret->data = data;
	return ret;
}

void bst_delete(node *root) {
	if (root->left) {
		bst_delete(root->left);
	}
	if (root->right) {
		bst_delete(root->right);
	}
	free(root);
}

static inline node *bst_minimum(node *root) {
	node *c = root;
	while (c && c->left) {
		c = c->left;
	}
	return c;
}

static inline node *bst_maximum(node *root) {
	node *c = root;
	while (c && c->right) {
		c = c->right;
	}
	return c;
}
/***************************************************
 *
 *               Tree modification
 *
 ****************************************************/
// insert a new node into the tree
void bst_insert(node *root, int data) {
	node *elem = bst_new(data);
	node *iterator = root;
	while (1) {
		if (data < iterator->data) {
			if (iterator->left) {
				iterator = iterator->left;
			} else {
				iterator->left = elem;
				break;
			}
		} else if (data > iterator->data) {
			if (iterator->right) {
				iterator = iterator->right;
			} else {
				iterator->right = elem;
				break;
			}
		} else {
			if (iterator->next) {
				while (iterator->next) {
					iterator = iterator->next;
				}
			}
			iterator->next = elem;
			break;
		}
	}
}

// Returns the element in the tree with the specified data
// in the case that there are multiple elements with the same data,
// this returns the head of a linked list of all such elements.
node *bst_find(node *root, int data) {
	if (root->data == data) {
		return root;
	}

	node *iterator = root;
	while (1) {
		if (data < iterator->data) {
			if (iterator->left) {
				iterator = iterator->left;
			} else {
				return NULL;
			}
		} else if (data > iterator->data) {
			if (iterator->right) {
				iterator = iterator->right;
			} else {
				return NULL;
			}
		} else {
			break;
		}
	}

	if (iterator->data == data) {
		return iterator;
	} else {
		return NULL;
	}
}

/****************************************************
 *
 *                    Tree traversal
 *
 *****************************************************/
/* Perform the function fun on all nodes in a node in specified traversal method */
void bst_preorder(void (*fun)(node *), node *n) {
	if (n) {
		fun(n);
	}
	if (n->left) {
		bst_preorder(fun, n->left);
	}
	if (n->right) {
		bst_preorder(fun, n->right);
	}
}

void bst_inorder(void (*fun)(node *), node *n) {
	if (n->left) {
		bst_inorder(fun, n->left);
	}
	if (n) {
		fun(n);
	}
	if (n->right) {
		bst_inorder(fun, n->right);
	}
}

void bst_postorder(void (*fun)(node *), node *n) {
	if (n->left) {
		bst_postorder(fun, n->left);
	}
	if (n->right) {
		bst_postorder(fun, n->right);
	}
	if (n) {
		fun(n);
	}
}
