
#include "header.h"

#include "io_files.h"
#include "file_parse.h"


static char *get_report(char *f_path, char *country, char *date, int *succ, int *fail);
static void send_report(int opcode, char *report, int write_fd, int buf_size);


/* ========================================================================= */

// Return 1 if tm1 < tm2 (tm2 is later than tm1), 0 if equal, -1 if tm1 > tm2.
static int compare_tmspec(struct timespec *tm1, struct timespec *tm2)
{
  time_t sec1  = tm1->tv_sec, sec2 = tm2->tv_sec;
  long   nsec1 = tm1->tv_nsec, nsec2 = tm2->tv_nsec;

  if (sec1 < sec2 || (sec1 == sec2 && nsec1 < nsec2))
    return 1;
  if (sec1 == sec2 && nsec1 == nsec2)
    return 0;
  return -1;
}

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

/* ========================================================================= */

// Read a directory and return stats associated with it (DIR */ country / time of last check).
// Send a report for each file to the parent.
struct country_dir *read_directory(int opcode, char *country, char *input_dir, int write_fd, int buf_size, int *succ, int *fail)
{
  char path[256];   // Compose path for dir to open
  snprintf(path, 256, "%s/%s", input_dir, country);

  DIR *dir = opendir(path);
  if (dir == NULL){perror("opendir @ read_directory");exit(1);}

  struct country_dir *cdir = malloc(sizeof(struct country_dir));
  cdir->dir = dir;
  cdir->country = strdup(country);  // Associate the DIR * with the <country> it represents
      
  int total_files = count_files(dir);

  char *file_names[total_files];
  get_file_names(file_names, total_files, dir);  // Get files via date order

  struct stat s;
  if (stat(path, &s) == -1){perror("stat @ read_directory"); exit(1);}
  
  cdir->last_checked = s.st_mtim;  // Mark the time of the last parse of the dir
  
  // Notify the parent if this is an initial child or a forked/replacement child (after SIGCHLD)
  int send_opcode = (opcode == READ_DIR_CMD) ? FILE_REPORT : FILE_REPORT_FORK;

  for (int i = 0; i < total_files; ++i)  // Parse every file in the dir
  {
    char file_path[256];
    snprintf(file_path, 256, "%s/%s", path, file_names[i]);

    char *rep = get_report(file_path, cdir->country, file_names[i], succ, fail);
    send_report(send_opcode, rep, write_fd, buf_size);

    free(file_names[i]);
  }

  return cdir;  // Return info associated with the directory
}

/* ========================================================================= */

// Return the report of a new file located at <f_path>.
static char *get_report(char *f_path, char *country, char *date, int *succ, int *fail)
{
  FILE *fp = fopen(f_path, "r");
  if (fp == NULL){perror("fopen @ get_report"); exit(1);};

  char *report = parse_file(fp, country, date, succ, fail);

  if (fclose(fp) == -1){perror("fclose @ get_report"); exit(1);}
  return report;
}

// Send a <report> of a new file to the parent.
static void send_report(int opcode, char *report, int write_fd, int buf_size) {
  send_message(write_fd, opcode, report, buf_size);
  free(report); 
}

/* ========================================================================= */

// Check for updated files in the directories assigned to the worker.
// Send a report for each file found to the parent.
void read_directory_updates(bool at_setup, struct list *popen_dirs, int *pwrite_fd, char *pinput_dir, int *pbuf_size, int *psucc, int *pfail)
{
  static struct list *open_dirs;
  static int write_fd, buf_size;
  static int *succ, *fail;
  static char *input_dir;

  if (at_setup == true)  // Setup process
  {
    open_dirs = popen_dirs;  // this function can be called
    write_fd = *pwrite_fd;   // when checking for signals
    buf_size = *pbuf_size;
    input_dir = pinput_dir;
    succ = psucc;
    fail = pfail;
    return;
  }

  struct stat s;

  int size = list_size(open_dirs);
  for (int i = 1; i <= size; ++i)  // Check every directory
  {
    struct country_dir *cdir = list_get(open_dirs, i);
    
    char path[256];   // Compose the directory path
    snprintf(path, 256, "%s%s", input_dir, cdir->country);

    if (stat(path, &s) == -1){perror("stat @ read_dir_updates"); exit(1);}
    
    struct timespec ts = s.st_mtim;  // Check if the modification time is equal to our last check
    if (compare_tmspec(&cdir->last_checked, &ts) <= 0)    // If it is then no additions on this dir
      continue;

    struct dirent *entry;
    while ((entry = readdir(cdir->dir)) != NULL)  // Check every file
    {
      char *f_name = entry->d_name;
      if (!strcmp(f_name, ".") || !strcmp(f_name, ".."))
        continue;

      char f_path[256];
      snprintf(f_path, 256, "%s/%s", path, f_name);

      if (stat(f_path, &s) == -1){perror("stat @ read_dir_updates"); exit(1);}
      
      struct timespec ts = s.st_mtim;
      if (compare_tmspec(&cdir->last_checked, &ts) <= 0)
        continue;     // mod time is earlier or equal to our latest check, ignore the dir

      // Found a file to parse
      char *rep = get_report(f_path, cdir->country, f_name, succ, fail);
      send_report(FILE_REPORT_SIG, rep, write_fd, buf_size);
      
      // Notify parent that a new report is available
      if (kill(getppid(), SIGUSR2) == -1){perror("kill @ read_dir_updates"); exit(1);}
    }

    rewinddir(cdir->dir);  // Rewind for later use
  }
}

/* ========================================================================= */

// Create a log file consisting of every country assigned and successful/failed requests.
// Also cleanup memory assosiated with directories' stats (closedir, country_dirs etc).
void write_logs(bool at_setup, struct list *popen_dirs, int *pwrite_fd, int *pread_fd, int *psucc, int *pfail)
{
  static struct list *open_dirs;
  static int *succ, *fail;
  static int read_fd, writ_fd;

  if (at_setup == true)  // Setup process
  {
    open_dirs = popen_dirs;
    succ = psucc;         // this function can be called
    fail = pfail;         // when checking for signals
    read_fd = *pread_fd;
    writ_fd = *pwrite_fd;
    return;
  }

  int written;
  char buf[128];
  snprintf(buf, 128, "logs/log_file.%d", getpid());

  int fd = creat(buf, 0666);  // Create log file
  if (fd == -1){perror("creat @ write_logs"); exit(1);}

  int size = list_size(open_dirs);
  for (int i = 1; i <= size; ++i)  // Output every country
  {
    struct country_dir *cdir = list_get(open_dirs, i);

    written = write(fd, cdir->country, strlen(cdir->country));
    if (written == -1){perror("write @ write_logs"); exit(1);}

    written = write(fd, "\n", 1);
    if (written == -1){perror("write @ write_logs"); exit(1);}

    free(cdir->country);  // Cleanup memory
    closedir(cdir->dir);
  }

  int total = *succ + *fail;
  snprintf(buf, 128, "TOTAL %d\nSUCCESS %d\nFAIL %d\n", total, *succ, *fail);

  written = write(fd, buf, strlen(buf));
  if (written == -1){perror("write @ write_logs"); exit(1);}
  if (close(fd) == -1){perror("close @ write_logs"); exit(1);}

  list_destroy(open_dirs);  // Cleanup memory
  // Terminate communication with parent
  if (close(read_fd) == -1){perror("close @ write_logs"); exit(1);}
  if (close(writ_fd) == -1){perror("close @ write_logs"); exit(1);}
}

/* ========================================================================= */