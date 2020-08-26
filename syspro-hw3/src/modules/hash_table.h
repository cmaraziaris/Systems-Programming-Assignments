#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>

struct hash_table;

struct bucket_entry
{
  char *key;
  void *data;
};

#define HT_MIN_ACCEPTABLE_BUCKET_SIZE ((int)(sizeof(struct bucket_entry) + sizeof(struct bucket_entry *)))
#define HT_DEF_BUCK_SIZE (500)  // Default hash table size values
#define HT_DEF_SIZE (500)

// Create a hash table with `ht_size` # buckets, with each bucket occupying `bucket_size` bytes.
struct hash_table *ht_create(int ht_size, int bucket_size, void (*destroy_func)(void *data));

void ht_insert(struct hash_table *ht, char *key, void *data);
void ht_destroy(void *ht);

void *ht_search(struct hash_table *ht, char *key);

int ht_size(struct hash_table *ht);

// Traverse the hash table. In each call, returns a unique `bucket_entry *`.
// Returns NULL if every `bucket_entry *` has been traversed.
struct bucket_entry *ht_traverse(struct hash_table *ht);


#endif