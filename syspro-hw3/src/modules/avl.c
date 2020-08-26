
#include <stdlib.h>

#include "avl.h"

struct avl_node
{
  int height;
  void *data;
  struct avl_node *left;
  struct avl_node *right;
};

struct avl
{
  int size;
  struct avl_node *root;
  int (*compare_func)(void *a, void *b);  // Sets the order of the elements in the tree.
  void (*destroy_func)(void *data); // Destroys the data when the tree is destroyed.
};

/* ========================================================================= */

struct avl *avl_create(int (*compare_func)(void *a, void *b), void (*destroy_func)(void *value))
{
  struct avl *tree = calloc(1, sizeof(struct avl));
  tree->compare_func = compare_func;
  tree->destroy_func = destroy_func;
  return tree;
}

/* ========================================================================= */

// Returns the data associated with `node`.
void *avl_node_value(struct avl_node *node) {
  return node ? node->data : NULL;
}

int avl_size(struct avl *tree) {
  return tree ? tree->size : 0;
}

/* ========================================================================= */

#define MAX_INT(X,Y) (X >= Y ? (X) : (Y)) // Max between 2 ints.

static int node_height(struct avl_node *node) {
  return node == NULL ? 0 : node->height;
}

static void node_update_height(struct avl_node *node) {
  node->height = 1 + MAX_INT(node_height(node->left), node_height(node->right));
}

// Rotations: Each function takes as an argument the node that must be rotated
// and returns the root of the new subtree.

// Single left rotation
static struct avl_node *node_rotate_left(struct avl_node *node)
{
  struct avl_node *right_node = node->right;
  struct avl_node *left_subtree = right_node->left;

  right_node->left = node;
  node->right = left_subtree;

  node_update_height(node);
  node_update_height(right_node);
  
  return right_node;
}

// Single right rotation
static struct avl_node *node_rotate_right(struct avl_node *node)
{
  struct avl_node *left_node = node->left;
  struct avl_node *left_right = left_node->right;

  left_node->right = node;
  node->left = left_right;

  node_update_height(node);
  node_update_height(left_node);
  
  return left_node;
}

// Double left-right rotation
static struct avl_node *node_rotate_left_right(struct avl_node *node)
{
  node->left = node_rotate_left(node->left);
  return node_rotate_right(node);
}

// Double right-left rotation
static struct avl_node *node_rotate_right_left(struct avl_node *node)
{
  node->right = node_rotate_right(node->right);
  return node_rotate_left(node);
}

// >  1 if the left subtree is unbalanced
// < -1 if the right subtree is unbalanced
static int node_height_diff(struct avl_node *node) {
  return node_height(node->left) - node_height(node->right);
}

// Restore the AVL property if needed.
static struct avl_node *node_repair_balance(struct avl_node *node)
{
  node_update_height(node);

  int balance = node_height_diff(node);
  if (balance > 1)  // Left subtree is unbalanced.
  {
    if (node_height_diff(node->left) >= 0) 
      return node_rotate_right(node);

    return node_rotate_left_right(node);
  }
  
  if (balance < -1)   // Right subtree is unbalanced.
  {
    if (node_height_diff(node->right) <= 0)
      return node_rotate_left(node);

    return node_rotate_right_left(node);
  }

  return node; // The subtree is balanced.
}

/* ========================================================================= */

static struct avl_node *node_create(void *value) {
  struct avl_node *node = calloc(1, sizeof(struct avl_node));
  node->data = value;
  node->height = 1;
  return node;
}

/* ========================================================================= */

// Insert a node in the tree and return its root.
static struct avl_node *node_insert(struct avl_node *node, int (*compare)(void *a, void *b), void *value)
{
  if (node == NULL)   // Empty tree.
    return node_create(value);

  int compare_res = compare(value, node->data);

  if (compare_res < 0)  // value is lesser than current node's data, so insert left.
    node->left = node_insert(node->left, compare, value); 
  else
    node->right = node_insert(node->right, compare, value);

  return node_repair_balance(node); // Repair the balance of the tree and return the root.
}

void avl_insert(struct avl *tree, void *data)
{
  if (tree == NULL) return;
  ++tree->size;
  tree->root = node_insert(tree->root, tree->compare_func, data);
}

/* ========================================================================= */

// Return the node for which `node->data` equals `value`, else NULL if such node was not found.
static struct avl_node *node_find_equal(struct avl_node *node, int (*compare)(void *a, void *b), void *value)
{
  if (node == NULL) // Empty tree.
    return NULL;

  int compare_res = compare(value, node->data);
  
  if (compare_res < 0)  // node is in the left subtree.
    return node_find_equal(node->left, compare, value);
  if (compare_res > 0)
    return node_find_equal(node->right, compare, value);

  return node;  // Found the node.
}

struct avl_node *avl_find_node(struct avl *tree, void *data) {
  return tree ? node_find_equal(tree->root, tree->compare_func, data) : NULL;
}

/* ========================================================================= */

// Returns the left-most node of the subtree with root `node`.
static struct avl_node *node_find_min(struct avl_node *node)
{
  return node != NULL && node->left != NULL
    ? node_find_min(node->left) // Left-subtree exists, search there.
    : node;              // We reached a leaf, return it.
}

// Returns the first (left-most) node of the AVL tree.
struct avl_node *avl_first(struct avl *tree) {
  return tree ? node_find_min(tree->root) : NULL;
}

/* ========================================================================= */

// Returns the `next` node of node `target`. Searches in the subtree with root `node`.
// Returns NULL if `target` is the right-most node of the tree.
static struct avl_node *node_find_next(struct avl_node *node, int (*compare)(void *a, void *b), struct avl_node *target)
{
  if (node == target) // Found the target, the next node is the min of its right subtree.
    return node_find_min(node->right);

  if (compare(target->data, node->data) > 0)  // Target is in the right subtree.
    return node_find_next(node->right, compare, target);

  struct avl_node *res = node_find_next(node->left, compare, target); // `target` is in the left subtree.
  return res == NULL ? node : res;     // If `target` is the right-most node of the left subtree, then the next is its parent, `node`.
}

// Returns the next node of `node` with the order specified by `tree->compare_func`.
// Returns NULL if `node` is the last (right-most) node of the tree.
struct avl_node *avl_next(struct avl *tree, struct avl_node *node) {
  return (tree && node) ? node_find_next(tree->root, tree->compare_func, node) : NULL;
}

/* ========================================================================= */

// Destroy the tree with root `node`.
static void node_destroy(struct avl_node *node, void (*destroy_func)(void *value))
{
  if (node == NULL)
    return;
  
  // Destroy the children, then the node.
  node_destroy(node->left, destroy_func);
  node_destroy(node->right, destroy_func);

  if (destroy_func != NULL)
    destroy_func(node->data);

  free(node);
}

// Destroy an AVL tree.
void avl_destroy(struct avl *tree)
{
  if (tree != NULL)
  {
    node_destroy(tree->root, tree->destroy_func);
    free(tree);
  }
}

/* ========================================================================= */

// Sets the compare function of the tree and returns the old compare function.
avl_cmp_func avl_set_compare_func(struct avl *tree, avl_cmp_func cmp)
{
  int (*old_func)(void *, void *) = tree->compare_func;
  tree->compare_func = cmp;
  return old_func;
}
/* ========================================================================= */