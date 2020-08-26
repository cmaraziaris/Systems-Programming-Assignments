#ifndef PATIENTS_H
#define PATIENTS_H

#include <stdbool.h>

struct patient_record
{
  char *record_id;
  char *first_name;
  char *last_name;
  char *disease_id;
  char *country;
  struct date *entry_date;
  struct date *exit_date;
};

// Returns true on success, false on failure.
bool insert_patient_record(char *rec_id, char *first, char *last, char *disease_id, char *country, char *entry_dt, char *exit_dt);

void record_patient_exit(char *rec_id, char *exit_dt);
void num_current_patients(char *disease);

void destroy_patient_record(struct patient_record *prec);

char *patient_get_country(struct patient_record *prec);
char *patient_get_disease_id(struct patient_record *prec);

#endif