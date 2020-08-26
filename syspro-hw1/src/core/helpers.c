
#include <string.h>
#include <stdlib.h>

#include "helpers.h"
#include "patients.h"
#include "global_vars.h"

extern struct global_vars global; // Hash tables.

/* ========================================================================= */

static void *first_node; // Indicates the first node in the desired range.

// Compare 2 entry dates. Store the last *greater* node found.
static int compare_just_greater(void *a, void *b)
{
  int res = compare_prec_entry_dates(a, b);
  if (res <= 0)
    first_node = b;
  return res;
}

// Returns the node storing a date *equal* or *just greater* than `sdate1`.
static struct avl_node *get_first_of_range(struct avl *patients_tree, char *sdate1)
{
  // Create a dummy patient record with the 1st date in range.
  struct date entr;
  struct patient_record dummy_beg = { .entry_date = &entr };
  convert_str_to_date(sdate1, &entr, DUMMY_BEGIN);

  // Change the compare function, so that we keep the 1st node in range.
  int (*old_cmp)(void *, void *) = avl_set_compare_func(patients_tree, compare_just_greater);

  first_node = NULL;  
  struct avl_node *curr = avl_find_node(patients_tree, &dummy_beg); // Search for the dummy (`sdate1`).
  
  if (curr == NULL && first_node == NULL)
    return NULL;  // Empty range

  if (curr == NULL)                                  // If no node with entry date `sdate1` exists
    curr = avl_find_node(patients_tree, first_node); // find the node with date just greater than `sdate1` (first_node).

  avl_set_compare_func(patients_tree, old_cmp);     // Restore the compare function.

  return curr;  // Return the 1st node of the range.
}

/* ========================================================================= */

// Returns the number of patients in the date range [sdate1, sdate2].
// If field, get_field != NULL, then every patient's field given by `get_field` must match `field`.
int get_diseased_range(struct avl *patients_tree, char *sdate1, char *sdate2, char *field, char *(*get_field)(struct patient_record *))
{
  if (patients_tree == NULL)
    return 0;

  struct date ext;               // Set a dummy record to specify the limit of the range.
  struct patient_record dummy_end = { .entry_date = &ext };
  convert_str_to_date(sdate2, &ext, DUMMY_END);

  struct avl_node *curr = get_first_of_range(patients_tree, sdate1);  // Get the 1st node in range.

  int sum = 0;
  if (field == NULL) // Return the num of patients in the date range.
  {
    while (curr && compare_prec_entry_dates(avl_node_value(curr), &dummy_end) < 0)  // We haven't surpassed the limit.
    {
      ++sum;
      curr = avl_next(patients_tree, curr); // We haven't reached the end of the tree.
    }
  }
  else
  {           // For every patient in the desired range.
    while (curr && compare_prec_entry_dates(avl_node_value(curr), &dummy_end) < 0)
    {
      struct patient_record *prec = avl_node_value(curr);
      if (strcmp(get_field(prec), field) == 0)  // If the patient's field matches the desired field, count them.
        ++sum;
      curr = avl_next(patients_tree, curr); // We haven't reached the end of the tree.
    }
  }

  return sum;     // Return the sum of the patients who justify the constraints.
}
/* ========================================================================= */

// Update the hash table with info extracted from `node`.
static void update_hash_table(struct hash_table *ht, struct avl_node *node, char *(*get_field)(struct patient_record *))
{
  struct patient_record *prec = avl_node_value(node);   // Get the patient record of the node.
  int *found;
  if ((found = ht_search(ht, get_field(prec))) != NULL)
    (*found)++;     // If the specific field of the prec already exists in the ht, update counter.
  else
  {     // If the specific field of the prec wasn't found in the ht, create and insert an entry with counter 1.
    int *pat_num = malloc(sizeof(int));
    *pat_num = 1;
    ht_insert(ht, get_field(prec), pat_num);
  }  
}

/* ========================================================================= */

// Set up a binary heap with patients extracted from `info`, in range [sdate1, sdate2],
// that have `field` in common and might differ in `get_field` outcome.
void set_bh_range(struct binary_heap *bh, struct hash_table *info, char *sdate1, char *sdate2, char *field, char *(*get_field)(struct patient_record *))
{
  struct date ext;
  convert_str_to_date(sdate2, &ext, DUMMY_END); // Set a dummy record to specify the limit of the range.
  struct patient_record dummy_end = { .entry_date = &ext };

  struct avl *patients_tree = ht_search(info, field); // Get patients that have `field` in common.
  if (patients_tree == NULL)
    return;

  struct avl_node *curr = get_first_of_range(patients_tree, sdate1);  // Get the 1st node in range.

  // Create a temporary hash table that associates `get_field` outcome, with the number of patients that share this field.
  struct hash_table *tmp = ht_create(avl_size(patients_tree) / 5 + 5, 5 * MIN_ACCEPTABLE_BUCKET_SIZE, NULL);
                                                        
  for (; curr && compare_prec_entry_dates(avl_node_value(curr), &dummy_end) < 0; curr = avl_next(patients_tree, curr))
    update_hash_table(tmp, curr, get_field);  // While we haven't surpassed the limit of the range, update the ht with the current value.

  struct bucket_entry *entry;
  while ((entry = ht_traverse(tmp)) != NULL)
    bh_insert(bh, entry);   // Traverse the hash table and insert every entry in the binary heap.

  ht_destroy(tmp);   // Destroy the temporary hash table.
}

/* ========================================================================= */

// Set up a binary heap with patients extracted from `info`, that have `field` in common and might differ in `get_field` outcome.
void set_bh_no_range(struct binary_heap *bh, struct hash_table *info, char *field, char *(*get_field)(struct patient_record *))
{
  struct avl *patients_tree = ht_search(info, field);  // Get patients that have `field` in common.
  if (patients_tree == NULL)
    return;

  // Create a temporary hash table that associates `get_field` outcome, with the number of patients that share this field.
  struct hash_table *tmp = ht_create(avl_size(patients_tree) / 10 + 10, 10 * MIN_ACCEPTABLE_BUCKET_SIZE, NULL);

  for (struct avl_node *node = avl_first(patients_tree); node != NULL; node = avl_next(patients_tree, node))
    update_hash_table(tmp, node, get_field);    // While we haven't surpassed the limit of the range, update ht with the current value.

  struct bucket_entry *entry;
  while ((entry = ht_traverse(tmp)) != NULL)
    bh_insert(bh, entry);     // Traverse the hash table and insert every entry in the binary heap.

  ht_destroy(tmp);   // Destroy the temporary hash table.
}
/* ========================================================================= */