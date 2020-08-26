#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avl.h"
#include "date.h"
#include "patients.h"
#include "utilities.h"
#include "hash_table.h"
#include "global_vars.h"

// Default argument for the internal `hidden` patient hash table
#define DEFAULT_BUCKET_NUM 3000  // in case of empty patient record file.
                                    
struct global_vars global;

/* ========================================================================= */

static int records_from_file;

// Count the number of records in the file.
static void set_line_num(FILE *fp)
{
  records_from_file = 0;
  while (!feof(fp))
    if (getc(fp) == '\n')
      ++records_from_file;

  if (fseek(fp, 0L, SEEK_SET))
  {
    perror("fseek");
    exit(EXIT_FAILURE);
  }
}

/* ========================================================================= */
// Destroy function for the contents (avl trees) of disease and country ht.
static void destroy_avl(void *data)
{
  struct avl *tree = data;
  avl_destroy(tree);
}

 // For the contents of patient ht.
static void destroy_precords(void *data)
{
  struct patient_record *prec = data;
  destroy_patient_record(prec);
}

// Allocate space for the hash tables used by the app.
void setup_structures(int dis_ht_entries, int ctry_ht_entries, int bucket_size)
{
  int pat_bucket_num = records_from_file ? records_from_file : DEFAULT_BUCKET_NUM;
  global.patients_ht = ht_create(pat_bucket_num / 50 + 50, 50 * MIN_ACCEPTABLE_BUCKET_SIZE, destroy_precords);
  global.disease_ht = ht_create(dis_ht_entries,  bucket_size, destroy_avl);
  global.country_ht = ht_create(ctry_ht_entries, bucket_size, destroy_avl);
}

void cleanup_structures(void)
{
  ht_destroy(global.country_ht);
  ht_destroy(global.disease_ht);
  ht_destroy(global.patients_ht);
}

/* ========================================================================= */
// Returns args read by reference to the pointers given.
void handle_cmd_line_args(int argc, const char **argv, FILE **fp, int *disease_s, int *country_s, int *bucket_s)
{
  if (argc != 9)
  {
    fprintf(stderr, "\n> Not enough or excessive arguments given.\n\n\n");
    exit(EXIT_FAILURE);
  }

  const char *pathname;

  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-b")) 
      *bucket_s = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-h1"))
      *disease_s = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-h2"))
      *country_s = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-p"))
      pathname = argv[++i];
    else
    {
      fprintf(stderr, "\n> Invalid command line argument option given: %s\n\n\n", argv[i]);
      exit(EXIT_FAILURE);
    }
  }

  if (*disease_s < 1 || *country_s < 1)
  {
    fprintf(stderr, "\n> Invalid hash table size given.\n> Please give a positive integer.\n\n\n");
    exit(EXIT_FAILURE);
  }

  if (*bucket_s < MIN_ACCEPTABLE_BUCKET_SIZE)
  {
    fprintf(stderr, "\n> Unacceptable bucket size given.\n\
> Please give an integer greater than or equal: %d\n\n\n", MIN_ACCEPTABLE_BUCKET_SIZE);
    exit(EXIT_FAILURE);
  }

  if ((*fp = fopen(pathname, "r")) == NULL)
  {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }
}

/* ========================================================================= */

// Parse a file with patients line by line, and insert every patient to the database.
// Return `true` if the file doesn't contain duplicate patients, else `false`.
bool parse_file(FILE *fp)
{
  bool valid = true;
  char *line = NULL;
  size_t len = 0;

  set_line_num(fp);

  while (getline(&line, &len, fp) != -1)
  {
    char *rec_id = strtok(line, " ");
    char *first = strtok(NULL, " ");
    char *last = strtok(NULL, " ");
    char *disease_id = strtok(NULL, " ");
    char *country = strtok(NULL, " ");
    char *entry_dt = strtok(NULL, " ");
    char *exit_dt = strtok(NULL, " \n");

    struct date d1, d2;     // Make sure the dates are given in order.
    convert_str_to_date(entry_dt, &d1, DUMMY_BEGIN);
    convert_str_to_date(exit_dt,  &d2, DUMMY_END);
    if (compare_dates(&d1, &d2) > 0)
    {
      printf("error\n");
      continue;
    }

    if (!insert_patient_record(rec_id, first, last, disease_id, country, entry_dt, exit_dt))
    {
      valid = false;
      fprintf(stderr, "[ERROR] Duplicate patient given in file.\n Exiting.\n");
      break;
    }
  }

  free(line);
  fclose(fp);

  return valid;
}
/* ========================================================================= */