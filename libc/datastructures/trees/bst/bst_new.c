#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

/*
 * Benefits of the new bst data structure:
 *  - separated tree structure from data, allowing
 *    a BST to point to anything, as long as the tree
 *    is given a proper comparator function to compare
 *    the elements in the tree.
 * XXX (will be added)
 *  - non-invasive structure will allow for static
 *    construction of BST. aka, you will be able to
 *    make a tree out of any struct by adding a tree_node
 *    to it and using the bst function family correctly
 *    (currently a work in progress)
 *
 */

#define container_of(ptr, type, member)                            \
	({                                                         \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); \
	})

#define TREE_NODE_INIT(name) { &(name), &(name), &(name), &(name) }

struct tree_node {
	struct tree_node *parent, *lchild, *rchild;
	void *data;
};

typedef struct bst {
	struct tree_node tree;
	int (*compare)(void *, void *);
} bst;

struct tree_node *__get_parent(struct bst *tree, void *data) {
	struct tree_node *ret = &tree->tree;
	while (1) {
		if (tree->compare(ret, data) < 0) {
			if (ret != ret->lchild) {
				ret = ret->lchild;
			} else {
				return ret;
			}
		} else {
			if (ret != ret->rchild) {
				ret = ret->rchild;
			} else {
				return ret;
			}
		}
	}
	return ret;
}

int compare(struct tree_node *a, struct tree_node *b) {
	return *((int *)b->data) - *((int *)a->data);
}

static struct tree_node *__make_tree_node(void *data) {
	struct tree_node *ret = malloc(sizeof(*ret));
	ret->parent = ret->lchild = ret->rchild = ret;
	ret->data = data;
	return ret;
}

void bst_insert(struct bst *root, void *data) {
	if ((struct tree_node *)root->tree.data == &root->tree) {
		root->tree.data = data;
		return;
	}
	struct tree_node *elem = __make_tree_node(data);
	struct tree_node *parent = __get_parent(root, data);
	if (root->compare(elem, parent) < 0) {
		parent->lchild = elem;
		elem->parent = parent;
	} else {
		parent->rchild = elem;
		elem->parent = parent;
	}
}

bst *bst_new(int (*comparator)(void *, void *)) {
	bst *ret = malloc(sizeof(*ret));
	ret->tree = (struct tree_node){ &(ret->tree), &(ret->tree),
					&(ret->tree), NULL };
	ret->compare = comparator;
	return ret;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
//                                                                                             //
//                                                                                             //
//                                        TREE TRAVERSAL                                       //
//                                                                                             //
//                                                                                             //
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * XXX give function pointers ability to access memory outside of tree and function itself
 * XXX give function pointers arbitrary number of parameters
 */
static void __inorder_traverse(struct tree_node *cur,
			       void (*func)(struct tree_node *node)) {
	if (!cur) {
		return;
	}
	if (cur != cur->lchild) {
		__inorder_traverse(cur->lchild, func);
	}
	func(cur);
	if (cur != cur->rchild) {
		__inorder_traverse(cur->rchild, func);
	}
}

static void __preorder_traverse(struct tree_node *cur,
				void (*func)(struct tree_node *node)) {
	if (!cur) {
		return;
	}
	func(cur);
	if (cur != cur->lchild) {
		__preorder_traverse(cur->lchild, func);
	}
	if (cur != cur->rchild) {
		__preorder_traverse(cur->rchild, func);
	}
}

static void __postorder_traverse(struct tree_node *cur,
				 void (*func)(struct tree_node *node)) {
	if (!cur) {
		return;
	}
	if (cur != cur->lchild) {
		__postorder_traverse(cur, func);
	}
	if (cur != cur->rchild) {
		__postorder_traverse(cur, func);
	}
	func(cur);
}

void bst_dofunc_inorder(bst *t, void (*func)(struct tree_node *)) {
	if (&t->tree) {
		__inorder_traverse(&t->tree, func);
	}
}

void bst_dofunc_preorder(bst *t, void (*func)(struct tree_node *)) {
	if (&t->tree) {
		__preorder_traverse(&t->tree, func);
	}
}

void bst_dofunc_postorder(bst *t, void (*func)(struct tree_node *)) {
	if (&t->tree) {
		__postorder_traverse(&t->tree, func);
	}
}

void __delete(struct tree_node *node) {
	free(node->data);
	free(node);
}

void bst_delete(bst *tree) {
	__postorder_traverse(tree, __delete);
}

void print_data_as_string(struct tree_node *node) {
	printf("poiner: %p data: %s\n", node->data, (const char *)node->data);
}

int comparator(void *a, void *b) {
	printf("inside comparator\n");
	return strcmp((const char *)a, (const char *)b);
}

#define MAX_SIZE 100
int main(void) {
	bst *tree = bst_new(comparator);
	char words[MAX_SIZE][MAX_SIZE];
	for (int i = 0; i < MAX_SIZE; i++) {
		scanf("%s", words[i]);
		printf("input received: %s\n", words[i]);
		if (strcmp(words[i], "exit") == 0) {
			printf("exiting\n");
			break;
		}
		bst_insert(tree, words[i]);
	}
	bst_dofunc_inorder(tree, print_data_as_string);
	return 0;
}
