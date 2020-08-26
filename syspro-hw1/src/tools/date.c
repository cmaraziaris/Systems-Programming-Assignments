
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "date.h"
#include "patients.h"

/* ========================================================================= */

// Convert a string that contains a date to a struct date and store it in `d`
// with ID specified from `type`.
void convert_str_to_date(char *str, struct date *d, enum date_type type)
{
  static unsigned long long count = 1;  // Unique identifier.
  
  char *buf = strdup(str);
  char *token = NULL;
  if (str)
    token = strtok(buf, "-");

  if (token)
  {
    d->active = true;
    d->day = atoi(token);
    d->month = atoi(strtok(NULL, "-"));
    d->year  = atoi(strtok(NULL, "-"));
    
    if (type == ENTRY) // entry dates, for actual patient records, have a unique id.
      d->id = count++;
    else if (type == DUMMY_BEGIN) // Dummy beginning entry dates have id 0.
      d->id = 0;
    else
      d->id = ULLONG_MAX; // ending dates have id ULLONG_MAX.
  }
  else
    d->active = false;  // "-" given as a date.

  free(buf);
}

/* ========================================================================= */
// Returns -1 if a < b
// Returns  0 if a == b
// Returns  1 if a > b
int compare_dates(void *a, void *b) // Doesn't compare IDs.
{
  struct date *d1 = a, *d2 = b;

  if (d2->active == false && d1->active == false)
    return 0;
  if (d2->active == false)
    return -1;
  if (d1->active == false)
    return 1;
  if (d1->year < d2->year)
    return -1;
  if (d1->year > d2->year)
    return 1;
  if (d1->month < d2->month)
    return -1;
  if (d1->month > d2->month)
    return 1;
  if (d1->day < d2->day)
    return -1;
  if (d1->day > d2->day)
    return 1;

  return 0;
}

/* ========================================================================= */
// Returns -1 if a < b
// Returns  0 if a == b
// Returns  1 if a > b
int compare_prec_entry_dates(void *a, void *b) // Also compares IDs.
{
  struct patient_record *p1 = a;
  struct patient_record *p2 = b;

  struct date *d1 = p1->entry_date;
  struct date *d2 = p2->entry_date;

  int res = compare_dates(d1, d2);

  if (res == 0)
  {
    if (d1->id < d2->id)
      return -1;
    if (d1->id > d2->id)
      return 1;
    
    return 0;
  }

  return res;
}

/* ========================================================================= */

void print_date(struct date *d)
{
  if (d->active)
    printf("%02d-%02d-%4d", d->day, d->month, d->year);
  else
    printf("-");
}
/* ========================================================================= */