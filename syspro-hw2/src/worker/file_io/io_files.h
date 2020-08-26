#ifndef READ_FILES_H
#define READ_FILES_H

#include <stdbool.h>

#define SETUP (true)
// Macros used when signal-checking
#define read_dir_updates() read_directory_updates(false, NULL, NULL, NULL, NULL, NULL, NULL)
#define write_log_file()   write_logs(false, NULL, NULL, NULL, NULL, NULL)


struct country_dir
{
  DIR *dir;
  char *country;
  struct timespec last_checked;  // Time of the last directory check
};


// Read a directory and return stats associated with it (DIR */ country / time of last check).
// Send a report for each file to the parent.
struct country_dir *read_directory(int opcode, char *country, char *input_dir, int write_fd, int buf_size, int *succ, int *fail);


// Check for updated files in the directories assigned to the worker.
// Send a report for each file found to the parent.
void read_directory_updates(bool at_setup, struct list *popen_dirs, int *pwrite_fd, char *pinput_dir, int *pbuf_size, int *psucc, int *pfail);


// Create a log file consisting of every country assigned and successful/failed requests.
// Also cleanup memory assosiated with directories' stats.
void write_logs(bool at_setup, struct list *popen_dirs, int *pwrite_fd, int *read_fd, int *succ, int *fail);


#endif