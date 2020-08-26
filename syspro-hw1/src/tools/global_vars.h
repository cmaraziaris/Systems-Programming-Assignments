#ifndef GLOBAL_H
#define GLOBAL_H

#include "hash_table.h"
#include "binary_heap.h"
#include "date.h"

struct global_vars
{
  struct hash_table *disease_ht;  // Disease hash table
  struct hash_table *country_ht;  // Country hash table
  struct hash_table *patients_ht; // Patient hash table
};

#endif