
#include "header.h"
#include "queries.h"
#include "io_files.h"
#include "stats.h"
#include "glob_structs.h"
#include "patients.h"

extern struct global_vars global;

/* ========================================================================= */

// Search in the database for a patient record with id <rec_id>.
// If found, send a message to the parent with the result (record).
void q_search_patient(char *rec_id, int write_fd, int buf_size)
{
  struct patient_record *prec = ht_search(global.patients_ht, rec_id);
  if (prec == NULL)
    return; // Patient not found

  char entry_dt[12], exit_dt[12]; // Compose a message with the result
  convert_date_to_str(entry_dt, prec->entry_date);
  convert_date_to_str(exit_dt, prec->exit_date);
  if (exit_dt[0] == '-')
    strcat(exit_dt, "-");

  char buf[256];
  snprintf(buf, 256, "%s %s %s %s %d %s %s", rec_id, prec->first_name, prec->last_name, prec->disease_id, prec->age, entry_dt, exit_dt);
  send_message(write_fd, SEARCH_RESULT_SUCCESS, buf, buf_size);
}

/* ========================================================================= */

// Send a message to the parent with the total number of patients 
// that ENTER'ed in date range [entry_dt, exit_dt] with <disease> from <country>.
void q_disease_frequency(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size)
{
  int cases = disease_frequency(disease, entry_dt, exit_dt, country);
  char buf[16];
  snprintf(buf, 16, "%d", cases);
  send_message(write_fd, DISEASE_FREQ_RESULT, buf, buf_size);  
}

/* ========================================================================= */

// Send a message to the parent with the total number of patients 
// that ENTER'ed in date range [entry_dt, exit_dt] with <disease> from <country>.
// If <country> is NULL, send a message for every country handled by the worker.
void q_num_pat_admissions(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size, struct list *open_dirs)
{
  char buf[128];
  if (country != NULL)  // Send results for 1 country
  {
    int cases = disease_frequency(disease, entry_dt, exit_dt, country);
    snprintf(buf, 128, "%s;%d", country, cases);
    send_message(write_fd, NUM_PAT_ADM_RESULT, buf, buf_size);
  }
  else  // Send results for *every* country assigned
  {
    int size = list_size(open_dirs);
    for (int i = 0; i < size; ++i)
    {
      struct country_dir *curr = list_get(open_dirs, i+1);
      int cases = disease_frequency(disease, entry_dt, exit_dt, curr->country);
      snprintf(buf, 128, "%s;%d", curr->country, cases);
      send_message(write_fd, NUM_PAT_ADM_RESULT, buf, buf_size);          
    }
  }
}

/* ========================================================================= */

// Send a message to the parent with the total number of patients 
// that EXIT'ted in date range [entry_dt, exit_dt] with <disease> from <country>.
// If <country> is NULL, send a message for every country handled by the worker.
void q_num_pat_discharges(char *disease, char *country, char *entry_dt, char *exit_dt, int write_fd, int buf_size, struct list *open_dirs)
{
  char buf[128];
  if (country != NULL)
  {
    int cases = disease_exit_frequency(disease, entry_dt, exit_dt, country);
    snprintf(buf, 128, "%s;%d", country, cases);
    send_message(write_fd, NUM_PAT_DIS_RESULT, buf, buf_size);
  }
  else
  {
    int size = list_size(open_dirs);
    for (int i = 0; i < size; ++i)
    {
      struct country_dir *curr = list_get(open_dirs, i+1);
      int cases = disease_exit_frequency(disease, entry_dt, exit_dt, curr->country);
      snprintf(buf, 128, "%s;%d", curr->country, cases);
      send_message(write_fd, NUM_PAT_DIS_RESULT, buf, buf_size);          
    }
  }
}

/* ========================================================================= */