#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "../include/tree.h"
#include "rb.h"
#include "rbwrapper.h"

#define MAX_ITERATIONS 50
#define MAX_TREE_SIZE 50
#define MAX_ELEM_SIZE 10000

// XXX IMPORTANT: remove container_of from rb.h - not necessary
// TODO: should user handle insert/delete from API perspective with rb_fix?

struct results {
	int errno;
	int index_failed;
	struct tree *node_failed;
};

struct tree_context {
	char *name;
	struct tree **root;
	int num_elems;
	void (*tree_ins)(struct tree **, void *elem);
	void (*tree_del)(struct tree *);
	void *(*generate_elem)(int val);
	void (*verify)(struct tree_context *, struct results *);
	void (*printerrinfo)(int errno);
	// void *(*find)(void *val);
};

struct benchmark_context {
	int max_iterations;
	int max_tree_size;
	int max_elem_size;
	int max_deletions;
	int max_finds;
};

/* We need to give it a function that returns our container given a struct rbtree field */
void *container_of(struct rbtree *n) {
	return __container_of(n, struct myintrbtree, rbt);
}

/* And a comparator function to compare two of our containers */
int comp(void *a, void *b) {
	return ((struct myintrbtree *)a)->data -
	       ((struct myintrbtree *)b)->data;
}

/* We can use this function on the inorder traversal to print out the tree */
void printdata(struct tree *elem) {
	struct myintrbtree *ep =
		(struct myintrbtree *)container_of((struct rbtree *)elem);
	printf("%d ", ep->data);
}

int min(int a, int b) {
	return a < b ? a : b;
}

int max(int a, int b) {
	return a > b ? a : b;
}

void getminmaxpath(struct tree *elem, int *shortest, int *longest, int *cur) {
	if (!elem->left && !elem->right) {
		*shortest = min(*shortest, *cur);
		*longest = max(*longest, *cur);
	}

	if (elem->left && elem->left != elem) {
		(*cur)++;
		getminmaxpath(elem->left, shortest, longest, cur);
	}

	if (elem->right && elem->right != elem) {
		(*cur)++;
		getminmaxpath(elem->right, shortest, longest, cur);
	}
	(*cur)--;
}

int consecutivereds = 0;

void checkredcoloring(struct tree *elem) {
	if (parent((struct rbtree *)elem) &&
	    parent((struct rbtree *)elem)->color == RED) {
		if (((struct rbtree *)elem)->color == RED) {
			consecutivereds = 1;
		}
	}
}

int checkblackheight(struct tree *elem, int *standard, int *cur) {
	if (parent((struct rbtree *)elem) &&
	    ((struct rbtree *)elem)->color == BLACK) {
		(*cur)++;
	}
	if (!standard && !elem->left && !elem->right) {
		*standard = *cur;
	}
	if (!elem->left && !elem->right && *cur != *standard) {
		return 1;
	}
	if (elem->left && elem->left != elem) {
		checkblackheight(elem->left, standard, cur);
	}
	if (elem->right && elem->right != elem) {
		checkblackheight(elem->right, standard, cur);
	}

	return 0;
}

void fill_with_random(struct myintrbtree **root, int tree_size,
		      int max_elem_size) {
	for (int i = 0; i < tree_size; i++) {
		struct myintrbtree *elem = malloc(sizeof(struct myintrbtree));
		struct rbtree *rt = &(*root)->rbt;
		if (!elem) {
			fprintf(stderr, "malloc error\n");
			exit(EXIT_FAILURE);
		}

		elem->rbt.node.left = elem->rbt.node.right = NULL;
		elem->data = rand() % max_elem_size;
		rb_ins(&rt, &elem->rbt, container_of, comp);
	}
}

void rb_verify(struct tree_context *tctx, struct results *res) {
	res->errno = 0;
	res->index_failed = -1;
	res->node_failed = NULL;

	struct tree *iterator = *tctx->root;
	int prev = ((struct myintrbtree *)*tctx->root)->data;
	for (int i = 0; (iterator = inorder_successor(iterator)); i++) {
		if (prev > ((struct myintrbtree *)container_of(
				    (struct rbtree *)iterator))
				   ->data) {
			res->errno = NOT_BST;
			res->index_failed = i;
			res->node_failed = iterator;
			return;
		}
		prev = ((struct myintrbtree *)container_of(
				(struct rbtree *)iterator))
			       ->data;
	}

	int shortest = tctx->num_elems, longest = 1, cur = 1;
	getminmaxpath(*tctx->root, &shortest, &longest, &cur);
	if (shortest * 2 < longest) {
		res->errno = NOT_BALANCED;
		res->node_failed = *tctx->root;
		return;
	}

	if (do_inorder(*tctx->root, checkredcoloring)) {
		res->errno = TWO_CONSECUTIVE_RED_NODES;
		res->node_failed = *tctx->root;
		return;
	}

	int standard = 1;
	cur = 1;

	// TODO check every path on every node, this just checks every root-nil path
	if (checkblackheight(*tctx->root, &standard, &cur)) {
		res->errno = DIFFERING_BLACKHEIGHTS;
		res->node_failed = *tctx->root;
		return;
	}
}

void rmnode(struct tree *n) {
	struct myintrbtree *i = (struct myintrbtree *)n;
	free(i);
}

// XXX remove all elements from tree and start over fresh
void benchmark(struct benchmark_context *bctx, struct tree_context *tctx) {
	clock_t start, end;
	double avgt = 0, totalt = 0;

	do_inorder(*tctx->root, printdata);
	puts("");

	for (int i = 0; i < bctx->max_iterations; i++) {
		for (int j = 0; j < bctx->max_tree_size; j++) {
			start = clock();
			void *p = tctx->generate_elem(rand() %
						      bctx->max_elem_size);
			tctx->tree_ins(tctx->root, p);
			end = clock();
			double t =
				(1000 * (double)(end - start) / CLOCKS_PER_SEC);
			avgt += t / bctx->max_iterations;
			totalt += t;
		}

		struct results res;
		tctx->verify(tctx, &res);

		if (res.errno) {
			fprintf(stderr, "%s FAILED VERIFICATION: ", tctx->name);
			tctx->printerrinfo(res.errno);
			printf("near: %d",
			       ((struct myintrbtree *)res.node_failed)->data);
			puts("");
			do_inorder(*tctx->root, printdata);
			puts("");
			exit(EXIT_FAILURE);
		} else {
			printf("Case %d PASSED\n", i);
		}

		do_inorder(*tctx->root, rmnode);
	}
	printf("%s ins\nTotal time = %.2fms, Avg time = %.2fms\n", tctx->name,
	       totalt, avgt);
}

void *rb_genelem(int val) {
	struct myintrbtree *ret = malloc(sizeof(struct myintrbtree));
	memset(ret, 0, sizeof(*ret));
	if (!ret) {
		fprintf(stderr, "malloc error!\n");
		return NULL;
	}
	ret->data = val;
	return ret;
}

int getdata(struct tree *t) {
	return ((struct myintrbtree *)t)->data;
}

/*void interactive(void) {
  char buff[100];
  int state = 0, first = 1;
  struct myintrbtree *root = malloc(sizeof(struct myintrbtree));
  struct rbtree *rbroot = &root->rbt;
  do {
  scanf("%s", buff);
  switch (*buff) {
  case 'i':
  state = 1;
  break;
  case 'p':
  state = 0;
  printf("> ");
  do_inorder(rbroot, printdata);
  puts("");
  break;
  default:
  if (!state) {
  printf("assuming insert\n");
  state = 1;
  } 
  if (first) {
  first = 0;
  root->data = atoi(buff);
  break;
  }
  struct myintrbtree *tmp = malloc(sizeof(struct myintrbtree));
  if (!tmp) {
  fprintf(stderr, "malloc returned null. exiting\n");
  exit(EXIT_FAILURE);
  }
  tmp->data = atoi(buff);
  rb_ins_wrapper(rbroot, (struct rbtree *) tmp);
  }
  } while (strcmp(buff, "exit") != 0);

  }*/

int main(int argc, char **argv) {
	srand(time(NULL));

	struct benchmark_context bctx = { MAX_ITERATIONS, MAX_TREE_SIZE,
					  MAX_ELEM_SIZE };

	struct myintrbtree *rb = malloc(sizeof(struct myintrbtree));
	struct tree *rbroot;

	if (!rb) {
		fprintf(stderr, "malloc returned null\n");
		exit(EXIT_FAILURE);
	}

	rbroot = &rb->rbt.node;
	rbroot->parent = rbroot->left = rbroot->right = NULL;

	struct tree_context trees[1] = {
		{ "Red Black Tree", &rbroot, MAX_TREE_SIZE, rb_ins_wrapper,
		  rb_del_wrapper, rb_genelem, rb_verify, rb_printerrinfo }
	};

	if (argc > 1) {
		bctx.max_iterations = atoi(argv[1]);
	}

	if (argc > 2) {
		bctx.max_tree_size = atoi(argv[2]);
	}

	if (argc > 3) {
		bctx.max_elem_size = atoi(argv[3]);
	}

	benchmark(&bctx, &trees[0]);

	return 0;
}
