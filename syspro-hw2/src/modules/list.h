#ifndef LIST_MODULE_H
#define LIST_MODULE_H

struct list;


struct list *list_create(void (*destroy_func)(void *data));

int list_size(struct list *l);

void list_insert_first(struct list *lis, void *data);

// Returns NULL if out of bounds.
// First item has an index of 1.
void *list_get(struct list *lis, int index);

void list_destroy(void *lis);


#endif