
#include "header.h"
#include "setup_workers.h"
#include "m_queries.h"

/* ========================================================================= */

// Perform the following queries based on the <opcode> given:
// /diseaseFrequency
// /numPatientAdmissions
// /numPatientDischarges
void q_operate(int operation, int num_workers, struct worker_stats *w_stats, struct hash_table *ht_workers, int buf_size, char **stok_save)
{
  char *disease = strtok_r(NULL, " \n", stok_save);  // Get command arguments
  char *entry_dt = strtok_r(NULL, " \n", stok_save);
  char *exit_dt = strtok_r(NULL, " \n", stok_save);
  char *country = strtok_r(NULL, " \n", stok_save);  // If NULL, operate on *every* country

  char buf[128];  // Compose the message containing arguments
  snprintf(buf, 128, "%s:%s:%s:%s", disease, entry_dt, exit_dt, country == NULL ? " " : country);

  if (country != NULL)  // Send a message to the worker of this country
  {
    struct worker_stats *worker = ht_search(ht_workers, country);  // Find worker
    send_message(worker->writ_fd, operation, buf, buf_size);
  }
  else  // Send a message to every worker
  {
    for (int i = 0; i < num_workers; ++i)
      send_message(w_stats[i].writ_fd, operation, buf, buf_size);
  }

  for (int i = 0; i < num_workers; ++i)
    send_message(w_stats[i].writ_fd, AVAILABILITY_CHECK, "", buf_size);
}
/* ========================================================================= */

// Ask every worker to search for a patient record.
void q_search_patient(struct worker_stats *w_stats, int num_workers, int buf_size, char **stok_save)
{
  char *rec_id = strtok_r(NULL, " \n", stok_save);
  for (int i = 0; i < num_workers; ++i)  // If patient doesn't exist, we don't print anything.
  {
    send_message(w_stats[i].writ_fd, SEARCH_PATIENT, rec_id, buf_size);  // Command to search
    send_message(w_stats[i].writ_fd, AVAILABILITY_CHECK, "", buf_size);
  }
}

/* ========================================================================= */

// For every country, print the PID of the worker assigned to its dir.
void q_list_countries(struct hash_table *ht_workers)
{
  struct bucket_entry *entry;
  while ((entry = ht_traverse(ht_workers)) != NULL)
  {
    char *country = entry->key;
    struct worker_stats *worker = entry->data;
    printf("%s %d\n", country, worker->w_pid);
  }
}

/* ========================================================================= */

struct daily_cases  // Keep the age-range cases for a specific date
{
  struct date *dt;
  int age_range[4];
};


static struct daily_cases *create_daily_case(char *date, int *ranges)
{
  struct daily_cases *dc = malloc(sizeof(struct daily_cases));
  for (int i = 0; i < 4; ++i)
    dc->age_range[i] = ranges[i];
  dc->dt = malloc(sizeof(struct date));
  convert_str_to_date(date, dc->dt, DUMMY_BEGIN);
  return dc;
}


static void destroy_daily_case(void *data)
{
  struct daily_cases *dc = data;
  free(dc->dt);
  free(dc);
}


// Return the index of the max element in int array <sum[4]>.
static int find_max(int sum[4])
{
  int max = sum[0];
  int index = 0;
  for (int i = 1; i < 4; ++i)
    if (sum[i] > max)
    {
      index = i;
      max = sum[i];
    }
  return index;
}

/* ========================================================================= */

/* Structures required for the topk-AgeRanges query and are initialized by `q_add_report`
 *
 * ht_ranges   <has> key: country / data: ht_diseases
 * ht_diseases <has> key: disease / data: list_dates
 * list_dates  <has> struct daily_case
 */

// Print the message received in "<Age range> - <cases>" format.
// If <just_print> is FALSE, also add the contents of the message to the database.
void q_add_report(bool just_print, struct hash_table *ht_ranges, char *msg)
{
  char *stok_save;
  char *country = strtok_r(msg, "/", &stok_save);
  char *date = strtok_r(NULL, "/", &stok_save);

  printf("%s\n%s\n", date, country);

  struct hash_table *ht_diseases;
  if (just_print == false)  // Update the database
  {
    if ((ht_diseases = ht_search(ht_ranges, country)) == NULL)  // Find the disease hash table for this country
    {
      ht_diseases = ht_create(HT_DEF_SIZE, HT_DEF_BUCK_SIZE, list_destroy);
      ht_insert(ht_ranges, country, ht_diseases);  // Add a disease hash table in the country hash table
    }
  }

  while (1)  // Extract reports for every disease
  {
    char *disease = strtok_r(NULL,":", &stok_save);
    if (disease == NULL)
      return;    // We finished parsing the report.

    int ranges[4];
    char *ages[4] = {strtok_r(NULL,":", &stok_save), strtok_r(NULL,":", &stok_save), strtok_r(NULL,":", &stok_save), strtok_r(NULL,";", &stok_save)};

    ranges[0] = atoi(ages[0]);
    ranges[1] = atoi(ages[1]);
    ranges[2] = atoi(ages[2]);
    ranges[3] = atoi(ages[3]);

    printf("%s\nAge range 0-20 years: %d cases\nAge range 21-40 years: %d cases\nAge range \
41-60 years: %d cases\nAge range 60+ years: %d cases\n\n", disease, ranges[0], ranges[1], ranges[2], ranges[3]);

    if (just_print == false)  // Add report stats to the database
    {
      struct list *list_dates;
      if ((list_dates = ht_search(ht_diseases, disease)) == NULL)  // Get the list of dates for this disease
      {
        list_dates = list_create(destroy_daily_case);  // Create a new list_date
        ht_insert(ht_diseases, disease, list_dates);
      }

      struct daily_cases *dc = create_daily_case(date, ranges);  // Add the cases for this date in the disease's list
      list_insert_first(list_dates, dc);
    }
  }
}

/* ========================================================================= */

// Answers the topk-AgeRanges query.
// If country or disease don't exist, does nothing.
// Else, prints to stdout.
void q_find_topk(struct hash_table *ht_ranges, char **stok_save)
{
  int k = atoi(strtok_r(NULL, " \n", stok_save));  // Get arguments
  char *country = strtok_r(NULL, " \n", stok_save);
  char *disease = strtok_r(NULL, " \n", stok_save);
  char *d1 = strtok_r(NULL, " \n", stok_save);
  char *d2 = strtok_r(NULL, " \n", stok_save);

  struct date start;
  struct date end;
  convert_str_to_date(d1, &start, DUMMY_BEGIN);
  convert_str_to_date(d2, &end, DUMMY_END);

  int sum[4] = { 0 };  // Keep the summary of the age ranges found across dates

  struct hash_table *ht_diseases;
  if ((ht_diseases = ht_search(ht_ranges, country)) != NULL)
  {
    struct list *list_dates;
    if ((list_dates = ht_search(ht_diseases, disease)) != NULL)
    {
      int size = list_size(list_dates);
      for (int i = 1; i <= size; ++i)
      {
        struct daily_cases *dc = list_get(list_dates, i);
        if ((compare_dates(dc->dt, &start) >= 0 && compare_dates(dc->dt, &end) <= 0))
        {
          for (int i = 0; i < 4; ++i)    // If the daily cases are in range
            sum[i] += dc->age_range[i];  // add every field to the summary
        }
      }
    }
    else
      return;  // No such country found
  }
  else
    return;  // No such disease found

  int total = sum[0] + sum[1] + sum[2] + sum[3];
  if (total == 0)
    return;

  if (k > 4)
    k = 4;

  for (int i = 0; i < k; ++i)  // Print the <k> top
  {
    int index = find_max(sum);
    float value = 100.0 * sum[index] / total;
    sum[index] = -1;
    if (value == 0)
      continue;

    switch (index)
    {
      case 0:
        printf("0-20: %3.1f%%\n", value);
        break;
      case 1:
        printf("21-40: %3.1f%%\n", value);
        break;
      case 2:
        printf("41-60: %3.1f%%\n", value);
        break;
      case 3:
        printf("60+: %3.1f%%\n", value);
        break;
    }
  }
}

/* ========================================================================= */