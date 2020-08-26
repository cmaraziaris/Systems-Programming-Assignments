
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avl.h"
#include "date.h"
#include "patients.h"
#include "hash_table.h"
#include "global_vars.h"

extern struct global_vars global;

/* ========================================================================= */

// Create a patient record.
static struct patient_record *create_patient(char *rec_id, char *first, char *last, char *disease_id, char *country, char *entry_dt, char *exit_dt)
{
  struct patient_record *prec = malloc(sizeof(struct patient_record));
  prec->record_id  = strdup(rec_id);
  prec->first_name = strdup(first);
  prec->last_name  = strdup(last);
  prec->disease_id = strdup(disease_id);
  prec->country = strdup(country);
  prec->entry_date = malloc(sizeof(struct date));
  prec->exit_date = malloc(sizeof(struct date));

  convert_str_to_date(entry_dt, prec->entry_date, ENTRY);
  if (exit_dt)
    convert_str_to_date(exit_dt, prec->exit_date, EXIT);
  else
    prec->exit_date->active = false;

  return prec;
}

void destroy_patient_record(struct patient_record *prec)
{
  free(prec->record_id);
  free(prec->first_name);
  free(prec->last_name);
  free(prec->disease_id);
  free(prec->country);
  free(prec->entry_date);
  free(prec->exit_date);
  free(prec);
}
/* ========================================================================= */

// Insert a patient record in the data structures used by the app.
// Return `true` if the insertion was successful.
// Return `false` if the patient already exists.
bool insert_patient_record(char *rec_id, char *first, char *last, char *disease_id, char *country, char *entry_dt, char *exit_dt)
{
  struct patient_record *prec = create_patient(rec_id, first, last, disease_id, country, entry_dt, exit_dt);

  if (ht_search(global.patients_ht, prec->record_id))
  {
    destroy_patient_record(prec);
    return false;
  }

  // Add patient to the patient ht.
  ht_insert(global.patients_ht, prec->record_id, prec);

  struct avl *patient_tree = NULL;
  // Add patient to the disease ht.
  if ((patient_tree = ht_search(global.disease_ht, prec->disease_id)) != NULL)
    avl_insert(patient_tree, prec);             // If the disease is already in the db.
  else
  {
    patient_tree = avl_create(compare_prec_entry_dates, NULL);
    avl_insert(patient_tree, prec);
    ht_insert(global.disease_ht, prec->disease_id, patient_tree);  // Insert the disease in the db.
  }
  // Add patient to the country ht.
  if ((patient_tree = ht_search(global.country_ht, prec->country)) != NULL)
    avl_insert(patient_tree, prec);             // If the country is already in the db.
  else
  {
    patient_tree = avl_create(compare_prec_entry_dates, NULL);
    avl_insert(patient_tree, prec);
    ht_insert(global.country_ht, prec->country, patient_tree);  // Insert the country in the db.
  }

  return true;
}
/* ========================================================================= */

// Update a patient's exit date. Print a msg on error.
void record_patient_exit(char *rec_id, char *exit_dt)
{
  struct patient_record *prec = ht_search(global.patients_ht, rec_id);

  if (prec == NULL)
    printf("Not found\n");
  else
  {
    struct date old = *(prec->exit_date);
    convert_str_to_date(exit_dt, prec->exit_date, EXIT);

    if (compare_dates(prec->entry_date, prec->exit_date) > 0)
    {
      printf("record_patient_exit: Exit date given is lesser than current entry date.\n");
      printf("Entry date: \n");
      print_date(prec->entry_date);
      printf("Exit date given: \n");
      print_date(prec->exit_date);
      *(prec->exit_date) = old;  // Restore the old exit date.
    }
    else
      printf("Record updated\n");
  }
}

/* ========================================================================= */

// Prints the number of patients still hospitalised for `disease` if given,
// else for every disease.
void num_current_patients(char *disease)
{
  if (disease != NULL)
  {
    struct avl *tree = ht_search(global.disease_ht, disease); // Get the disease avl.
    int sum = 0;
    for (struct avl_node *node = avl_first(tree); node != NULL; node = avl_next(tree, node))
    {
      struct patient_record *prec = avl_node_value(node);
      if (prec->exit_date->active == false)  // Patient still hospitalised.
        ++sum;
    }
    printf("%s %d\n", disease, sum);
  }
  else  // Traverse every disease avl in the ht.
  {
    struct bucket_entry *entry;
    while ((entry = ht_traverse(global.disease_ht)) != NULL)
    {
      int sum = 0;
      for (struct avl_node *node = avl_first(entry->data); node != NULL; node = avl_next(entry->data, node))
      {
        struct patient_record *prec = avl_node_value(node);
        if (prec->exit_date->active == false)  // Patient still hospitalised.
          ++sum;
      }
      printf("%s %d\n", entry->key, sum);
    }
  }
}

/* ========================================================================= */

char *patient_get_country(struct patient_record *prec) {
  return prec->country;
}

char *patient_get_disease_id(struct patient_record *prec) {
  return prec->disease_id;
}
/* ========================================================================= */