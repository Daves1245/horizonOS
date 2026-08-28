
typedef struct node {
	int data;
	struct node *left;
	struct node *right;
	struct node *next; // use a linked list for identical elements
} node;

/* Creation and Deletion of tree */
node *bst_new(int data);
void bst_delete(node *root);

/* Tree manipulations */
void bst_insert(node *root, int elem);
node *bst_remove(node *root, int data);

/* Non-modifiers */
node *bst_find(node *root, int data);
/* Tree traversal */
void bst_preorder(void (*fun)(node *n), node *n); // These functions perform
void bst_inorder(void (*fun)(node *n), node *n); // the function given to them
void bst_postorder(void (*fun)(node *n),
		   node *n); // on each node in the tree in the
// appropriate order
