#ifndef CUP_TREE_NODE_H
#define CUP_TREE_NODE_H

/*
 * XXX add k-ary trees
 * XXX rename to btree?
 */
struct tree {
	struct tree *parent, *left, *right;
};

#endif
