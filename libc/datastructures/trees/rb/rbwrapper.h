#define NOT_BST 1
#define ROOT_NOT_BLACK 2
#define NOT_BALANCED 3
#define TWO_CONSECUTIVE_RED_NODES 4
#define DIFFERING_BLACKHEIGHTS 5

/* We can construct a container for any object to be a part of an RB tree */
struct myintrbtree {
	struct rbtree rbt;
	int data;
};

void *rb_container_of(struct rbtree *node) {
	return __container_of(node, struct myintrbtree, rbt);
}

int rb_comp(void *a, void *b) {
	return ((struct myintrbtree *)a)->data -
	       ((struct myintrbtree *)b)->data;
}

void rb_printdata(struct tree *elem) {
	struct myintrbtree *ep =
		(struct myintrbtree *)rb_container_of((struct rbtree *)elem);
	printf("%d ", ep->data);
}

void rb_ins_wrapper(struct tree **root, void *elem) {
	struct rbtree *tmp = (struct rbtree *)*root;
	rb_ins(&tmp, elem, rb_container_of, rb_comp);
	*root = (struct tree *)tmp;
}

void rb_del_wrapper(struct tree *node) {
	rb_del((struct rbtree *)node);
}

void rb_printerrinfo(int status) {
	switch (status) {
	case NOT_BST:
		printf("Not a BST\n");
		break;
	case ROOT_NOT_BLACK:
		printf("The root node is not black\n");
		break;
	case NOT_BALANCED:
		printf("Tree is not balanced (longest root-nil path > 2 * shortest)\n");
		break;
	case TWO_CONSECUTIVE_RED_NODES:
		printf("A red node is the child of another red node\n");
		break;
	case DIFFERING_BLACKHEIGHTS:
		printf("Two paths were found from the same node with differing black heights\n");
		break;
	}
}
