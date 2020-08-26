
#include "header.h"
#include "patients.h"
#include "file_parse.h"

/* ========================================================================= */

static void update_stats(struct hash_table *ht, char *disease, int age);
static char *convert_ht_to_str(char *country, char *date, struct hash_table *stats_ht);

// Parse a file with patients line by line, and insert every patient to the database.
// Return a string (report) with patient stats. Update valid/invalid records counters.
char *parse_file(FILE *fp, char *country, char *date, int *successful, int *failed)
{
  char *stok_save, *line = NULL;
  size_t len = 0;

  struct hash_table *stats_ht = ht_create(40, 50, free);  // Keep track of stats (disease-age_ranges)

  while (getline(&line, &len, fp) != -1)  // Read every record
  {
    char *rec_id = strtok_r(line, " ", &stok_save);
    char *attr = strtok_r(NULL, " ", &stok_save);  // ENTER / EXIT
    char *first = strtok_r(NULL, " ", &stok_save);
    char *last = strtok_r(NULL, " ", &stok_save);
    char *disease_id = strtok_r(NULL, " ", &stok_save);
    char *age_str = strtok_r(NULL, " \n", &stok_save);

    int age = atoi(age_str);
    if (age <= 0 || age > 120)  // Invalid age
    {
      fprintf(stderr, "ERROR\n");
      ++(*failed);
      continue;
    }

    char *entry_dt = NULL, *exit_dt = date;
    if (strcmp(attr, "ENTER") == 0)
    {
      entry_dt = date;
      exit_dt = NULL;
    }

    if (insert_patient_record(rec_id, first, last, disease_id, country, age, entry_dt, exit_dt) == false)
    {
      fprintf(stderr, "ERROR\n");  // Invalid patient record
      ++(*failed);
      continue;
    }

    ++(*successful);  // Valid record

    if (entry_dt != NULL)  // If a patient ENTER'ed today, count him as a case
      update_stats(stats_ht, disease_id, age);
  }

  free(line);

  char *res = convert_ht_to_str(country, date, stats_ht);  // Generate the report
  ht_destroy(stats_ht);
  return res;
}

/* ========================================================================= */

// Update the hash table that keeps track of <disease - age_ranges> pairs.
static void update_stats(struct hash_table *ht, char *disease, int age)
{
  int *range;
  if ((range = ht_search(ht, disease)) == NULL)
  {
    range = calloc(4, sizeof(int));  // If the <disease> doesn't exists
    ht_insert(ht, disease, range);   // create a new (<disease>, age_ranges) pair
  }

  if (age <= 20) ++range[0];
  else if (age <= 40) ++range[1];  // Update the corresponding age range
  else if (age <= 60) ++range[2];
  else ++range[3];
}

/* ========================================================================= */

// Convert the stats of the age-ranges per disease (<stats_ht>) in a single string.
// Return the string (report).
// Note: Caller must deallocate memory returned.
static char *convert_ht_to_str(char *country, char *date, struct hash_table *stats_ht)
{
  char *res = malloc((ht_size(stats_ht) * 42 + 64) * sizeof(char));
  sprintf(res, "%s/%s/", country, date);

  int total = strlen(country) + strlen(date) + 2;  // Keep track of the next char in <res>

  struct bucket_entry *entry;
  while ((entry = ht_traverse(stats_ht)) != NULL)
  {
    char buf[128];
    char *disease = entry->key;
    int *range = entry->data;
    snprintf(buf, 128, "%s:%d:%d:%d:%d;", disease, range[0], range[1], range[2], range[3]);
    strcpy(res+total, buf);
    total += strlen(buf);
  }

  res[total] = '\0';
  return res;
}

// Result string looks like: "Spain/10-12-2019/HIV:0:12:3:2;H1N1:1:2:50:84;"

/* ========================================================================= */