enum RB_COLOR { RED, BLACK };

struct rbtree {
	struct tree node;
	enum RB_COLOR color;
};

struct rbtree *rb(struct tree *t);
struct rbtree *rb_get_root(struct rbtree *node);
struct rbtree *parent(struct rbtree *e);
struct rbtree *uncle(struct rbtree *e);

int rb_ins(struct rbtree **root, struct rbtree *leaf,
	   void *(*container_of)(struct rbtree *), int (*comp)(void *, void *));
int rb_del(struct rbtree *node);
void *rb_find(struct rbtree *root, void *val, int (*comp)(void *, void *),
	      void *container_of(struct rbtree *));
