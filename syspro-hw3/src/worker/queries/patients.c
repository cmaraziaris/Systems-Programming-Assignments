
#include "header.h"
#include "glob_structs.h"
#include "patients.h"

extern struct global_vars global;

/* ========================================================================= */
static bool record_patient_exit(char *rec_id, char *first, char *last, char *disease, char *country, int age, char *exit_dt);

// Create a patient record.
static struct patient_record *create_patient(char *rec_id, char *first, char *last, char *disease_id, char *country, int age, char *entry_dt, char *exit_dt)
{
  struct patient_record *prec = malloc(sizeof(struct patient_record));
  prec->age = age;
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
bool insert_patient_record(char *rec_id, char *first, char *last, char *disease_id, char *country, int age, char *entry_dt, char *exit_dt)
{
  if (exit_dt != NULL)  // If EXIT date is specified, try to update an existing record. 
    return record_patient_exit(rec_id, first, last, disease_id, country, age, exit_dt);

  struct patient_record *prec = create_patient(rec_id, first, last, disease_id, country, age, entry_dt, exit_dt);

  if (ht_search(global.patients_ht, prec->record_id))  // Patient already exists.
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

// Update a patient's exit date. Return false on error.
static bool record_patient_exit(char *rec_id, char *first, char *last, char *disease, char *country, int age, char *exit_dt)
{
  struct patient_record *prec = ht_search(global.patients_ht, rec_id);

  if (prec == NULL) // Record ID not found
    return false;
  
  if (prec->exit_date->active == true)  // Patient has already exitted.
    return false;
  
  if (strcmp(prec->first_name, first) || strcmp(prec->last_name, last) || strcmp(prec->disease_id, disease) || strcmp(prec->country, country))
    return false;  // Credentials don't match.
  
  if (prec->age > age)  // Patient is younger when exitting, than when entering...
    return false;

  convert_str_to_date(exit_dt, prec->exit_date, EXIT);
  if (compare_dates(prec->entry_date, prec->exit_date) > 0)  // Exit date is earlier than entry date.
  {
    prec->exit_date->active = false;
    return false;
  }

  return true;  // Patient exitted successfully.
}

/* ========================================================================= */

char *patient_get_country(struct patient_record *prec) {
  return prec->country;
}

char *patient_get_disease_id(struct patient_record *prec) {
  return prec->disease_id;
}
/* ========================================================================= */