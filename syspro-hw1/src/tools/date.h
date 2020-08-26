#ifndef DATE_H
#define DATE_H

#include <stdbool.h>

struct date
{
  bool active;  // true if it contains a date, false if date not set ("-").
  int day;
  int month;
  int year;
  unsigned long long id;  // Unique identifier between entry dates.
};

enum date_type
{
  EXIT,
  ENTRY,
  DUMMY_BEGIN,
  DUMMY_END,
};

void print_date(struct date *d);

void convert_str_to_date(char *str, struct date *d, enum date_type type);

int compare_dates(void *a, void *b);
int compare_prec_entry_dates(void *a, void *b);

#endif