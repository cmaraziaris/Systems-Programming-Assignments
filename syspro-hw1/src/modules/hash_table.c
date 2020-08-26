
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"

/* ========================================================================= */

struct hash_table
{
  int size;   // Number of total items stored in the HT.
  int num_of_entries;   // Number of entries each bucket can store.   
  int num_of_buckets;
  struct bucket **buckets;    // Array of (ptrs to) buckets.
  void (*destroy_func)(void *data);
};

struct bucket
{
  struct bucket_entry **entries; // Array of (ptrs to) entries.
  struct bucket *next;  // Pointer to the next bucket, implements list/seperate chaining.
};

/* ========================================================================= */

int ht_size(struct hash_table *ht) {
  return ht->size;
}

/* ========================================================================= */

// Create a hash table with `ht_size` # buckets, with each bucket occupying `bucket_size` bytes.
struct hash_table *ht_create(int ht_size, int bucket_size, void (*destroy_func)(void *data))
{
  if (ht_size == 0)
    return NULL;

  struct hash_table *ht = malloc(sizeof(struct hash_table));

  ht->size = 0;
  ht->num_of_buckets = ht_size;
  ht->buckets = calloc(ht_size, sizeof(struct bucket *)); // Bucket array
  ht->destroy_func = destroy_func;

  // Amount of entries a bucket can hold, rounded to the closest integer to avoid fragmentation.
  ht->num_of_entries = bucket_size / MIN_ACCEPTABLE_BUCKET_SIZE;

  for (int i = 0; i < ht_size; ++i) // Allocate buckets.
  {
    ht->buckets[i] = calloc(1, sizeof(struct bucket));
    ht->buckets[i]->entries = calloc(ht->num_of_entries, sizeof(struct bucket_entry *));
  }

  return ht;
}

/* ========================================================================= */

static int hash_function(char *str, int size){
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;  /* hash * 33 + c */
  return (hash % size);
}

static struct bucket_entry *create_entry(char *key, void *data)
{
  struct bucket_entry *entry = malloc(sizeof(struct bucket_entry));
  entry->key = key;
  entry->data = data;
  return entry;
}

// Linear search the bucket for an empty slot. Insert and return true if such slot was found.
static bool insert_at_empty(int size, struct bucket *b, struct bucket_entry *entry)
{
  for (int i = 0; i < size; ++i)
  {      
    if (b->entries[i] == NULL) // Found empty pos.
    {
      b->entries[i] = entry;  // Insert
      return true;
    }
  }

  return false; // Didn't find an empty slot, bucket is full.
}

// Insert `data` with `key` in the hash table.
void ht_insert(struct hash_table *ht, char *key, void *data)
{
  ++ht->size;

  int index = hash_function(key, ht->num_of_buckets);  // Get index of the bucket.

  struct bucket_entry *entry = create_entry(key, data);
  struct bucket *curr = ht->buckets[index];
  struct bucket *last;    // Keep the last bucket visited.

  while (curr)  // Traverse the chain of buckets.
  {
    if (insert_at_empty(ht->num_of_entries, curr, entry) == true)
      return;   // We inserted the entry.

    last = curr;
    curr = curr->next;
  }
  // Every bucket in the chain is full, so create a new one.
  last->next = calloc(1, sizeof(struct bucket));
  last->next->entries = calloc(ht->num_of_entries, sizeof(struct bucket_entry *));
  // And insert the entry.
  last->next->entries[0] = entry;
}
/* ========================================================================= */

// Return the value associated with the `key` given.
// Return NULL if the `key` was not found.
void *ht_search(struct hash_table *ht, char *key)
{
  int index = hash_function(key, ht->num_of_buckets);

  struct bucket *b = ht->buckets[index];
  while (b)  // Traverse the chain of buckets.
  {
    for (int i = 0; i < ht->num_of_entries; ++i)
    {
      if (b->entries[i] == NULL) // Found empty position in bucket, so stop.
        return NULL;

      if (strcmp(key, b->entries[i]->key) == 0) // Found the data.
        return b->entries[i]->data;
    }

    b = b->next;
  }

  return NULL;  // Visited every bucket in the chain, value not found.
}

/* ========================================================================= */

// Destroy a bucket and its `size` entries.
static void destroy_bucket(int size, struct bucket *b, void (*destroy_func)(void *data))
{
  if (b->next)  // Destroy the linked bucket first.
  {
    destroy_bucket(size, b->next, destroy_func);
    free(b->next);
  }

  for (int i = 0; i < size; ++i)  // Destroy entries.
  {
    if (b->entries[i] == NULL)
      break;

    if (destroy_func != NULL)
    {
      destroy_func(b->entries[i]->data);
      free(b->entries[i]);
    }
  }

  free(b->entries);
}

// Destroy the hash table.
void ht_destroy(struct hash_table *ht)
{
  for (int i = 0; i < ht->num_of_buckets; ++i)  // Destroy every bucket.
  {
    destroy_bucket(ht->num_of_entries, ht->buckets[i], ht->destroy_func);
    free(ht->buckets[i]);
  }

  free(ht->buckets);
  free(ht);
}

/* ========================================================================= */

// Traverse the hash table. In each call, returns a unique `bucket_entry *`
// until every `bucket_entry *` has been traversed, so `NULL` is returned.
struct bucket_entry *ht_traverse(struct hash_table *ht)
{
  static int bucket, entry;   // Counters to track our position at the ht.
  static bool set_ht = true;
  static struct bucket *curr;
  
  if (ht == NULL) return NULL;

  if (set_ht == true) // First call of the parse.
  {
    curr = ht->buckets[0];
    bucket = entry = 0;     // Reset counters.
    set_ht = false;
  }

  while (true)  // While no entry is found and we didn't exhaust the hash table.
  {
    if (entry == ht->num_of_entries)  // We visited every entry in the bucket
    {
      entry = 0;

      if (curr->next != NULL) // Current is not the last link of the  chain.
        curr = curr->next;
      else if (++bucket != ht->num_of_buckets)  // Move on to the next bucket
        curr = ht->buckets[bucket];             // in the standard array.
    }

    if (bucket == ht->num_of_buckets)   // We finished parsing the hash table.
    {
      set_ht = true;      // Prepare for the next parse.
      return NULL;
    }

    if (curr->entries[entry] == NULL)
      entry = ht->num_of_entries;   // We visited every entry in the bucket
    else
      return curr->entries[entry++];
  }
}
/* ========================================================================= */