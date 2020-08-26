
#include <stdlib.h>

#include "list.h"

/* ========================================================================= */

struct list_node
{
  void *data;
  struct list_node *next;
};


struct list
{
  int size;
  struct list_node *first;
  void (*destroy_func)(void *data);
};

/* ========================================================================= */
int list_size(struct list *l) {
  return l->size;
}

/* ========================================================================= */
struct list *list_create(void (*destroy_func)(void *data))
{
  struct list *new_list = calloc(1, sizeof(struct list));
  new_list->destroy_func = destroy_func;
  return new_list;
}

/* ========================================================================= */
static struct list_node *create_node(void *data, struct list_node *next)
{
  struct list_node *node = malloc(sizeof(struct list_node));
  node->data = data;
  node->next = next;
  return node;
}

void list_insert_first(struct list *lis, void *data)
{
  struct list_node *node = create_node(data, lis->first);
  lis->first = node;
  ++lis->size;
}

/* ========================================================================= */
void *list_get(struct list *lis, int index)
{
  if (index > lis->size)
    return NULL;

  struct list_node *node = lis->first;
  for (int i = 1; i < index; ++i)
    node = node->next;

  return node->data;
}

/* ========================================================================= */
void list_destroy(void *plist)
{
  struct list *lis = plist;
  struct list_node *node = lis->first;
  for (int i = 0; i < lis->size; ++i)
  {
    if (lis->destroy_func)
      lis->destroy_func(node->data);
    struct list_node *tmp = node->next;
    free(node);
    node = tmp;
  }
  free(lis);
}
/* ========================================================================= */