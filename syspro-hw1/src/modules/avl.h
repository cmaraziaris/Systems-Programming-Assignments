
struct avl;
struct avl_node;

typedef int (*avl_cmp_func)(void *, void *);

// Sets the compare function of the tree and returns the old compare function.
avl_cmp_func avl_set_compare_func(struct avl *tree, avl_cmp_func cmp);

struct avl *avl_create(int (*compare)(void *a, void *b), void (*destroy_func)(void *data));

int avl_size(struct avl *tree);

void avl_insert(struct avl *tree, void *data);
void avl_destroy(struct avl *tree);

// Returns the data associated with `node`.
void *avl_node_value(struct avl_node *node);

// Returns the `node` that stores `data`.
// Returns NULL if `data` wasn't found in the tree.
struct avl_node *avl_find_node(struct avl *tree, void *data);

// Returns the first (left-most) node of the AVL tree.
struct avl_node *avl_first(struct avl *tree);

// Returns the next node of `node`.
// Returns NULL if `node` is the last (right-most) node of the tree.
struct avl_node *avl_next(struct avl *tree, struct avl_node *node);