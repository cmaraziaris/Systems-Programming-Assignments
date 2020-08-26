
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avl.h"
#include "date.h"
#include "stats.h"
#include "helpers.h"
#include "patients.h"
#include "hash_table.h"
#include "binary_heap.h"
#include "global_vars.h"

extern struct global_vars global;

/* ========================================================================= */

// Prints the number of patients for every disease (in range [sdate1, sdate2], if specified).
void global_disease_stats(char *sdate1, char *sdate2)
{
  if (sdate1 == NULL) // No range.
  {
    struct bucket_entry *entry;
    while ((entry = ht_traverse(global.disease_ht)) != NULL)
      printf("%s %d\n", entry->key, avl_size(entry->data));
  }
  else
  {
    struct bucket_entry *entry;                    // Traverse the disease ht to extract stats.
    while ((entry = ht_traverse(global.disease_ht)) != NULL)
      printf("%s %d\n", entry->key, get_diseased_range(entry->data, sdate1, sdate2, NULL, NULL));
  }
}
/* ========================================================================= */

// Prints the number of patients for `disease` in range [sdate1, sdate2].
// If `country` is specified, patients originate from `country`.
void disease_frequency(char *disease, char *sdate1, char *sdate2, char *country)
{
  struct avl *patient_tree = ht_search(global.disease_ht, disease);
  printf("%s %d\n", disease, get_diseased_range(patient_tree, sdate1, sdate2, country, patient_get_country));
}

/* ========================================================================= */

static void free_ints(void *val)
{
  struct bucket_entry *entry = val;
  free(entry->data);  // int *
  free(entry);
}

static int compare_pairs(void *a, void *b)
{
  struct bucket_entry *a1 = a, *b1 = b;
  int res = *(int *)(a1->data) - *(int *)(b1->data);
  if (res == 0)
    return -1 * strcmp(a1->key, b1->key);
  return res;
}

// Extract, print and destroy the k first elements of `bh`.
static void extract_results(struct binary_heap *bh, int k)
{
  for (int i = 1; i <= k; ++i)
  {
    struct bucket_entry *p = bh_remove_max(bh); // Get the entry with the most patients.
    if (p != NULL)
    {
      printf("%s %d\n", p->key, *(int *)(p->data));
      free_ints(p);   // Destroy the element extracted.
    }
    else
      return; // No more elements to extract, binary heap is empty.
  }
}
/* ========================================================================= */

// Prints the `k` most infective diseases in `country` 
// (in range [sdate1, sdate2] if specified) 
void topk_diseases(int k, char *country, char *sdate1, char *sdate2)
{
  struct binary_heap *bh = bh_create(compare_pairs, free_ints);
  
  // Set up the bin heap, based on the `country_ht` and comparing patients using `disease_id`
  if (sdate1 == NULL)
    set_bh_no_range(bh, global.country_ht, country, patient_get_disease_id);
  else
    set_bh_range(bh, global.country_ht, sdate1, sdate2, country, patient_get_disease_id);

  extract_results(bh, k);

  bh_destroy(bh);   // Destroy the binary heap.
}

/* ========================================================================= */

// Prints the `k` most infected countries from `disease` 
// (in range [sdate1, sdate2] if specified) 
void topk_countries(int k, char *disease, char *sdate1, char *sdate2)
{
  struct binary_heap *bh = bh_create(compare_pairs, free_ints); // Create the bin heap

  // Set up the bin heap, based on the `disease_ht` and comparing patients using `country`
  if (sdate1 == NULL)
    set_bh_no_range(bh, global.disease_ht, disease, patient_get_country);
  else
    set_bh_range(bh, global.disease_ht, sdate1, sdate2, disease, patient_get_country);

  extract_results(bh, k);

  bh_destroy(bh);
}
/* ========================================================================= */