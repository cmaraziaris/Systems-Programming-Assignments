#ifndef PATIENTS_H
#define PATIENTS_H

#include <stdbool.h>

struct patient_record
{
  int age;
  char *record_id;
  char *first_name;
  char *last_name;
  char *disease_id;
  char *country;
  struct date *entry_date;
  struct date *exit_date;
};

// Insert a patient record in the data structures used by the app.
// Return `true` if the insertion was successful.
bool insert_patient_record(char *rec_id, char *first, char *last, char *disease_id, char *country, int age, char *entry_dt, char *exit_dt);

void destroy_patient_record(struct patient_record *prec);

char *patient_get_country(struct patient_record *prec);
char *patient_get_disease_id(struct patient_record *prec);


#endif