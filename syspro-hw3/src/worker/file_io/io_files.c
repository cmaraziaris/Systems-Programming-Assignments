
#include "header.h"

#include "date.h"
#include "io_files.h"
#include "file_parse.h"
#include "queries.h"

/* ========================================================================= */

// Returns the number of files in directory <dir>.
static int count_files(DIR *dir)
{
  int count = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
  {
    char *f_name = entry->d_name;
    if (!strcmp(f_name, ".") || !strcmp(f_name, ".."))
      continue;  // Ignore . and ..
    ++count;
  }
  rewinddir(dir);
  return count;
}


// Store file names in <names>, sorted via chronological order (name).
// Note: Caller must deallocate memory stored in <names[]> .
static void get_file_names(char *names[], int total, DIR *dir)
{
  struct dirent *entry;
  int curr_file = 0;
  while ((entry = readdir(dir)) != NULL) // Store every file name in array <names>
  {
    char *f_name = entry->d_name;
    if (!strcmp(f_name, ".") || !strcmp(f_name, ".."))
      continue;

    names[curr_file++] = strdup(f_name);
  }
  
  qsort(names, total, sizeof(names[0]), compare_date_strings); // Sort via chronological order // @164
  rewinddir(dir);
}


// Return the report of a new file located at <f_path>.
static char *get_report(char *f_path, char *country, char *date)
{
  FILE *fp;
  if ((fp = fopen(f_path, "r")) == NULL) error_exit("fopen");

  char *report = parse_file(fp, country, date);

  if (fclose(fp) == -1) error_exit("fclose");
  return report;
}

/* ========================================================================= */

// Read every file of the directory in path "<input_dir>/<country>".
// Send a report for each file to <write_fd>.
void read_directory(int opcode, char *country, char *input_dir, int write_fd, int buf_size)
{
  char path[256];   // Compose path for dir to open
  snprintf(path, 256, "%s/%s", input_dir, country);

  DIR *dir;
  if ((dir = opendir(path)) == NULL) error_exit("opendir");
     
  int total_files = count_files(dir);

  char *file_names[total_files];
  get_file_names(file_names, total_files, dir);  // Get file names sorted by date-name 
  
  // Notify <write_fd> if this is an original worker or a forked/replacement one (after SIGCHLD)
  int send_opcode = (opcode == READ_DIR_CMD) ? FILE_REPORT : FILE_REPORT_FORK;

  for (int i = 0; i < total_files; ++i)  // Parse every file in the dir & send a report
  {
    char file_path[256];
    snprintf(file_path, 256, "%s/%s", path, file_names[i]);

    char *stats = get_report(file_path, country, file_names[i]);
    send_message(write_fd, send_opcode, stats, buf_size);
    
    q_add_report(stats);  // Add report to the database

    free(stats);
    free(file_names[i]);
  }

  if (closedir(dir) == -1) error_exit("closedir");
}

/* ========================================================================= */