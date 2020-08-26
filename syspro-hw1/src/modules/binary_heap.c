
#include <stdlib.h> // calloc, free

struct bh_node
{
  struct bh_node *left;
  struct bh_node *right;
  struct bh_node *parent;
  struct bh_node *prev;   // Immediate previous node via level order.
  void *data;
};

struct binary_heap
{
  int size;
  struct bh_node *root;
  struct bh_node *last;   // Last node of the binary heap.
  int (*compare_func)(void *a, void *b);
  void (*destroy_func)(void *data);
};

/* ========================================================================= */

struct binary_heap *bh_create(int (*compare_func)(void *a, void *b), void (*destroy_func)(void *data))
{
  struct binary_heap *heap = calloc(1, sizeof(struct binary_heap));
  heap->compare_func = compare_func;
  heap->destroy_func = destroy_func;
  return heap;
}

/* ========================================================================= */

int bh_size(struct binary_heap *bh) {
  return bh->size;
}

/* ========================================================================= */
// Swaps *only* the data member of 2 nodes.
static void swap(struct bh_node *a, struct bh_node *b)
{
  void *tmp = a->data;
  a->data = b->data;
  b->data = tmp;
}

/* ========================================================================= */

enum child_t { LEFT, RIGHT };

void bh_insert(struct binary_heap *bh, void *data);
static void heapify_up(struct binary_heap *bh, struct bh_node *node);
static void insert_last(struct binary_heap *bh, void *data);
static void insert_root(struct binary_heap *bh, void *data);
static void rec_insert(struct binary_heap *bh, struct bh_node *node, struct bh_node *last, void *data);
static void insert_leftmost(struct binary_heap *bh, struct bh_node *node, void *data);
static void insert_child(enum child_t type, struct binary_heap *bh, struct bh_node *node, void *data);


void bh_insert(struct binary_heap *bh, void *data)
{
  insert_last(bh, data);
  heapify_up(bh, bh->last);
}

// Heapify the tree starting from `node` and going updwards to the root.
static void heapify_up(struct binary_heap *bh, struct bh_node *node)
{
  if (node == bh->root)
    return;

  if (bh->compare_func(node->parent->data, node->data) < 0)
  {
    swap(node->parent, node);  // Child is greater than the Parent so swap.
    heapify_up(bh, node->parent);
  }
}

static void insert_last(struct binary_heap *bh, void *data)
{
  if (bh->root == NULL)
    insert_root(bh, data);
  else
    rec_insert(bh, bh->last->parent, bh->last, data);
}

static void insert_root(struct binary_heap *bh, void *data)
{
  bh->root = calloc(1, sizeof(struct bh_node));
  bh->root->data = data;
  bh->last = bh->root;
  ++bh->size;
}

static void rec_insert(struct binary_heap *bh, struct bh_node *node, struct bh_node *last, void *data)
{
  if (node == NULL)   // We reached the root, so every level is full.
    insert_leftmost(bh, bh->root, data);
  else if (node->right == last)   // Last node is the right child of `node`, so try to insert at the parent of `node`.
    rec_insert(bh, node->parent, node, data);
  else if (node->right == NULL)   // Empty right node, insert there.
    insert_child(RIGHT, bh, node, data);
  else
    insert_leftmost(bh, node->right, data);   // Full left subtree.
}

// Insert at the leftmost leaf of tree with root `node`.
static void insert_leftmost(struct binary_heap *bh, struct bh_node *node, void *data)
{
  if (node->left == NULL)
    insert_child(LEFT, bh, node, data);
  else
    insert_leftmost(bh, node->left, data);
}

// Insert a node as a child of `node`.
static void insert_child(enum child_t type, struct binary_heap *bh, struct bh_node *node, void *data)
{
  struct bh_node **child = &node->right;
  if (type == LEFT)
    child = &node->left;

  *child = calloc(1, sizeof(struct bh_node));
  (*child)->parent = node;
  (*child)->prev = bh->last;  // Keep the previous node via level order.
  (*child)->data = data;
  bh->last = *child;  // Update last node of the heap.
  ++bh->size;
}

/* ========================================================================= */

static void heapify_down(struct binary_heap *bh, struct bh_node *node);
static void remove_leaf(struct binary_heap *bh, struct bh_node *node);

void *bh_remove_max(struct binary_heap *bh)
{
  if (bh->size == 0)
    return NULL;

  void *max = bh->root->data;

  swap(bh->root, bh->last);
  remove_leaf(bh, bh->last);

  heapify_down(bh, bh->root);

  return max;
}

// Heapify the tree, starting from root `node` going downwards until we reach a leaf.
static void heapify_down(struct binary_heap *bh, struct bh_node *node)
{
  if (node == NULL || node->left == NULL) // node is leaf.
    return;

  struct bh_node *max_child = node->left;
  if (node->right && bh->compare_func(node->left->data, node->right->data) < 0)
    max_child = node->right;

  if (bh->compare_func(node->data, max_child->data) < 0)
  {
    swap(node, max_child);  // Child is greater than the node, so swap.
    heapify_down(bh, max_child);
  }
}

static void remove_leaf(struct binary_heap *bh, struct bh_node *node)
{
  bh->last = node->prev;  // Update last as the previous node in level order.

  struct bh_node *parent = node->parent;
  if (parent) // If not root.
  {
    if (parent->left == node) // Update parent link.
      parent->left = NULL;
    else
      parent->right = NULL;

    free(node);
  }
  else
  {
    free(bh->root);
    bh->root = NULL;
  }
  --bh->size;
}

/* ========================================================================= */
// Recursively destroy a subtree with root `node`
static void node_destroy(struct bh_node *node, void (*destroy_func)(void *data))
{
  if (node == NULL)
    return;

  node_destroy(node->left, destroy_func);
  node_destroy(node->right, destroy_func);
  destroy_func(node->data);
  free(node);
}

void bh_destroy(struct binary_heap *bh)
{
  node_destroy(bh->root, bh->destroy_func);
  free(bh);
}

/* ========================================================================= */