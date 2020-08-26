
#include "header.h"
#include "queries.h"
#include "io_files.h"
#include "stats.h"
#include "glob_structs.h"
#include "patients.h"

extern struct global_vars global;

/* ========================================================================= */

// Search in the database for a patient record with id <rec_id>.
// Send a message over <write_fd> with the result.
void q_search_patient(char *rec_id, int write_fd, int buf_size)
{
  struct patient_record *prec = ht_search(global.patients_ht, rec_id);
  if (prec == NULL)
  {
    send_message(write_fd, SEARCH_RESULT_FAILURE, "", buf_size);
    return;  // Patient not found
  }

  char entry_dt[12], exit_dt[12];  // Compose a message with the result
  convert_date_to_str(entry_dt, prec->entry_date);
  convert_date_to_str(exit_dt, prec->exit_date);
  if (exit_dt[0] == '-')
    strcat(exit_dt, "-");

  char buf[256];
  snprintf(buf, 256, "%s %s %s %s %d %s %s", rec_id, prec->first_name, prec->last_name, prec->disease_id, prec->age, entry_dt, exit_dt);
  send_message(write_fd, SEARCH_RESULT_SUCCESS, buf, buf_size);
}

/* ========================================================================= */

// Send a message over <write_fd> with the total number of patients 
// that ENTER'ed in date range [entry_dt, exit_dt] with <disease> from <country>.
void q_disease_frequency(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size)
{
  int cases = disease_frequency(disease, entry_dt, exit_dt, country);
  char buf[16];
  snprintf(buf, 16, "%d", cases);
  send_message(write_fd, DISEASE_FREQ_RESULT, buf, buf_size);  
}

/* ========================================================================= */

// Send a message over <write_fd> with the total number of patients 
// that ENTER'ed in date range [entry_dt, exit_dt] with <disease> from <country>.
// If <country> is NULL, send a message for every country handled by the worker.
void q_num_pat_admissions(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size, struct list *countries)
{
  char buf[16];
  if (country != NULL)  // Send result for 1 country
  {
    int cases = disease_frequency(disease, entry_dt, exit_dt, country);
    sprintf(buf, "%d", cases);
    send_message(write_fd, NUM_PAT_ADM_RESULT, buf, buf_size);
  }
  else  // Send results for *every* country assigned
  {
    int total = 0;
    int size = list_size(countries);
    
    for (int i = 0; i < size; ++i)
      total += disease_frequency(disease, entry_dt, exit_dt, list_get(countries, i+1));

    sprintf(buf, "%d", total);
    send_message(write_fd, NUM_PAT_ADM_RESULT, buf, buf_size);          
  }
}

/* ========================================================================= */

// Send a message over <write_fd> with the total number of patients 
// that EXIT'ted in date range [entry_dt, exit_dt] with <disease> from <country>.
// If <country> is NULL, send a message for every country handled by the worker.
void q_num_pat_discharges(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size, struct list *countries)
{
  char buf[128];
  if (country != NULL)  // 1 country
  {
    int cases = disease_exit_frequency(disease, entry_dt, exit_dt, country);
    snprintf(buf, 128, "%s %d", country, cases);
    send_message(write_fd, NUM_PAT_DIS_RESULT, buf, buf_size);
  }
  else
  {
    int size = list_size(countries);
    for (int i = 0; i < size; ++i)
    {
      char *country = list_get(countries, i+1);
      int cases = disease_exit_frequency(disease, entry_dt, exit_dt, country);
      snprintf(buf, 128, "%s %d", country, cases);
      send_message(write_fd, NUM_PAT_DIS_RESULT, buf, buf_size);
    }
  }
}

/* ========================================================================= */

/* topk-AgeRanges query */

struct daily_cases  // Keep the "age-range" cases for a specific date
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

/* Structures required for the topk-AgeRanges query
 * are initialized by `q_add_report`:
 *
 * ht_ranges   <maps> key: country / data: ht_diseases
 * ht_diseases <maps> key: disease / data: list_dates
 * list_dates  <has> struct daily_case
 *
 * We can easily track the "country" -> then the "disease" -> finally the dates in range
 */

// Add the contents of the report <rep> to the database.
void q_add_report(char *rep)
{
  char *stok_save;
  char *country = strtok_r(rep, "/", &stok_save);
  char *date = strtok_r(NULL, "/", &stok_save);

  struct hash_table *ht_diseases;

  if ((ht_diseases = ht_search(global.ht_ranges, country)) == NULL)  // Find the disease hash table for this country
  {
    ht_diseases = ht_create(HT_DEF_SIZE, HT_DEF_BUCK_SIZE, list_destroy);
    ht_insert(global.ht_ranges, country, ht_diseases);  // Add a newly found disease ht in the country ht
  }

  while (1)  // Extract reports for every disease
  {
    char *disease = strtok_r(NULL,":", &stok_save);
    if (disease == NULL)
      return;    // We finished parsing the report.

    int ranges[4];
    char *ages[4] = {strtok_r(NULL,":", &stok_save), strtok_r(NULL,":", &stok_save), strtok_r(NULL,":", &stok_save), strtok_r(NULL,";", &stok_save)};

    ranges[0] = atoi(ages[0]);  // Get cases for every age-range
    ranges[1] = atoi(ages[1]);
    ranges[2] = atoi(ages[2]);
    ranges[3] = atoi(ages[3]);

    struct list *list_dates;
    if ((list_dates = ht_search(ht_diseases, disease)) == NULL)  // Get the list of dates for this disease
    {
      // Create a new list if the disease is inserted for the first time
      list_dates = list_create(destroy_daily_case);
      ht_insert(ht_diseases, disease, list_dates);
    }

    // Add the cases for this date in the disease's list
    struct daily_cases *dc = create_daily_case(date, ranges);
    list_insert_first(list_dates, dc);
  }
}

/* ========================================================================= */

// Answers the topk-AgeRanges query.
// Sends the result over <write_fd>.
void q_find_topk(char *body, int write_fd, const int buf_size)
{
  char *stok_save;
  strtok_r(body, " \n", &stok_save);  // Initialize strtok

  int k = atoi(strtok_r(NULL, " \n", &stok_save));  // Get arguments
  char *country = strtok_r(NULL, " \n", &stok_save);
  char *disease = strtok_r(NULL, " \n", &stok_save);
  char *d1 = strtok_r(NULL, " \n", &stok_save);
  char *d2 = strtok_r(NULL, " \n", &stok_save);
  struct date start;
  struct date end;
  convert_str_to_date(d1, &start, DUMMY_BEGIN);
  convert_str_to_date(d2, &end, DUMMY_END);

  int sum[4] = { 0 };  // Keep the summary of the cases for every age range across dates

  struct hash_table *ht_diseases = ht_search(global.ht_ranges, country);

  struct list *list_dates;
  if ((list_dates = ht_search(ht_diseases, disease)) != NULL)
  {
    int size = list_size(list_dates);
    for (int i = 1; i <= size; ++i)
    {
      struct daily_cases *dc = list_get(list_dates, i);
      if ((compare_dates(dc->dt, &start) >= 0 && compare_dates(dc->dt, &end) <= 0))
      {
        for (int i = 0; i < 4; ++i)    // If the daily cases are in the date range given
          sum[i] += dc->age_range[i];  // add every field/cases to the summary
      }
    }
  }
  else
  {
    send_message(write_fd, TOPK_AGE_RESULT, "No such disease.", buf_size);
    return;
  }

  int total = sum[0] + sum[1] + sum[2] + sum[3];
  if (total == 0)
  {
    send_message(write_fd, TOPK_AGE_RESULT, "No cases in given range of dates.", buf_size);
    return;
  }

  if (k > 4)  // Make sure <k> is not greater than 4
    k = 4;

  char result[128];
  result[0] = '\0';

  for (int i = 0; i < k; ++i)  // Write the <k> top to the result
  {
    int index = find_max(sum);
    float value = 100.0 * sum[index] / total;
    sum[index] = -1;
    
    if (value == 0)  // Don't send age ranges with 0 cases (0%)
      continue;

    char buf[32];
    switch (index)
    {
      case 0:
        sprintf(buf, "0-20: %3.1f%%\n", value);
        break;
      case 1:
        sprintf(buf, "21-40: %3.1f%%\n", value);
        break;
      case 2:
        sprintf(buf, "41-60: %3.1f%%\n", value);
        break;
      case 3:
        sprintf(buf, "60+: %3.1f%%\n", value);
        break;
    }

    strcat(result, buf);
  }

  int len = strlen(result);
  if (len > 0 && result[len-1] == '\n')
    result[len-1] = '\0';    // Trim trailing '\n' for better output

  send_message(write_fd, TOPK_AGE_RESULT, result, buf_size);
}

/* ========================================================================= */