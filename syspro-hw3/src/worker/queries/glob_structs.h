#ifndef GLOBAL_H
#define GLOBAL_H

#include "hash_table.h"

struct global_vars
{
  struct hash_table *disease_ht;   // Disease hash table
  struct hash_table *country_ht;   // Country hash table
  struct hash_table *patients_ht;  // Patient hash table
  struct hash_table *ht_ranges;    // topk-AgeRange query
};

// Allocate space for the hash tables used by the app.
void setup_structures(int dis_ht_entries, int ctry_ht_entries, int bucket_size);

void cleanup_structures(void);


#endif