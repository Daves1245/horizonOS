// For reduced memory usage
struct node {
	struct node *parent, *lchild, *rchild;
};

struct bst {
	struct node root;
	int (*comparator)(void *, void *);
}
