
#include "header.h"
#include "patients.h"
#include "glob_structs.h"
#include "date.h"

extern struct global_vars global;

// Returns the number of patients in the date range [sdate1, sdate2].
static int get_diseased_range(struct avl *patients_tree, char *sdate1, char *sdate2, char *field, char *(*get_field)(struct patient_record *));

/* ========================================================================= */

// Returns the number of patients with <disease> that ENTER'ed in range [sdate1, sdate2].
// Patients originate from <country>, if specified (not NULL).
int disease_frequency(char *disease, char *sdate1, char *sdate2, char *country)
{
  struct avl *patient_tree = ht_search(global.disease_ht, disease);
  return get_diseased_range(patient_tree, sdate1, sdate2, country, patient_get_country);
}

/* ========================================================================= */

// Returns the number of patients with <disease> from country <country> that EXIT'ted in range [sdate1, sdate2].
int disease_exit_frequency(char *disease, char *sdate1, char *sdate2, char *country)
{
  struct avl *patient_tree = ht_search(global.disease_ht, disease);
  struct date d1, d2;
  convert_str_to_date(sdate1, &d1, DUMMY_BEGIN);
  convert_str_to_date(sdate2, &d2, DUMMY_END);
  int total = 0;

  for (struct avl_node *node = avl_first(patient_tree); node != NULL; node = avl_next(patient_tree, node))
  {
    struct patient_record *prec = avl_node_value(node);
    if (compare_dates(prec->exit_date, &d1) < 0 || compare_dates(prec->exit_date, &d2) > 0 || strcmp(country, prec->country))
      continue;

    ++total;
  }

  return total;
}

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

// Returns the node storing a date *equal* or *just greater* than <sdate1>.
static struct avl_node *get_first_of_range(struct avl *patients_tree, char *sdate1)
{
  // Create a dummy patient record with the 1st date in range.
  struct date entr;
  struct patient_record dummy_beg = { .entry_date = &entr };
  convert_str_to_date(sdate1, &entr, DUMMY_BEGIN);

  // Change the compare function, so that we keep the 1st node in range.
  int (*old_cmp)(void *, void *) = avl_set_compare_func(patients_tree, compare_just_greater);

  first_node = NULL;  
  struct avl_node *curr = avl_find_node(patients_tree, &dummy_beg); // Search for the dummy (<sdate1>).
  
  if (curr == NULL && first_node == NULL)
    return NULL;  // Empty range

  if (curr == NULL)                                  // If no node with entry date <sdate1> exists
    curr = avl_find_node(patients_tree, first_node); // find the node with date just greater than <sdate1> (first_node).

  avl_set_compare_func(patients_tree, old_cmp);     // Restore the compare function.

  return curr;  // Return the 1st node of the range.
}

/* ========================================================================= */

// Returns the number of patients in the date range [sdate1, sdate2].
// If field, get_field != NULL, then every patient's field given by <get_field> must match <field>.
static int get_diseased_range(struct avl *patients_tree, char *sdate1, char *sdate2, char *field, char *(*get_field)(struct patient_record *))
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