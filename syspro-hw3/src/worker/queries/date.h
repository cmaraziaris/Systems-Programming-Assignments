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
void convert_date_to_str(char *buf, struct date *d);

// Every compare:
// Returns -1 if a < b, 0 if a == b, 1 if a > b
// 
int compare_dates(void *a, void *b);
int compare_date_strings(const void *s1, const void *s2);
int compare_prec_entry_dates(void *a, void *b);

#endif