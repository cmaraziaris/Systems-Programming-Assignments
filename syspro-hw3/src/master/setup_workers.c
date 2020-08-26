
#include "header.h"
#include "setup_workers.h"
#include "fifo_dir.h"

/* ========================================================================= */

// Create a worker and his named fifos. Store his stats in <w_stats[index]>. Return his pid.
// Inform the new worker if he's a replacement for a dead one.
pid_t create_worker(struct worker_stats *w_stats, int index, char *buf_size_str, char *input_dir, int is_replacement)
{
  char fifo_path[32];
  create_unique_fifo(false, fifo_path);  // Create fifo

  // Store fifo in <w_stats> array
  w_stats[index].writ_fd = open(fifo_path, O_RDWR | O_NONBLOCK);  // parent uses it for *writing only*
  if (w_stats[index].writ_fd == -1) error_exit("open");

  pid_t pid = fork();
  switch (pid)
  {
    case -1:
      perror("fork");
      exit(1);
    case 0: 
      // Create the worker, `fifo_path` is the worker's read end of the fifo.
      execl("./worker", "worker", buf_size_str, fifo_path, input_dir, is_replacement ? "1" : "0", NULL);
      perror("execl");
      exit(1);
  }

  w_stats[index].w_pid = pid;  // Store his pid
  return pid;
}

/* ========================================================================= */

// Create <num_workers> workers. Store the stats for each worker in the array <w_stats>.
void create_n_workers(struct worker_stats *w_stats, int num_workers, int buf_size, char *input_dir)
{
  create_unique_fifo(true, NULL);  // Setup named fifos

  char b_size_str[15];
  sprintf(b_size_str, "%d", buf_size);

  for (int i = 0; i < num_workers; ++i)
    create_worker(w_stats, i, b_size_str, input_dir, 0);
}

/* ========================================================================= */

// Assign countries (dirs) located in <input_dir> to workers in <w_stats>.
// Map every country to the PID associated with it in <ht_workers>. 
void assign_countries(struct worker_stats *w_stats, int num_workers, int buf_size, struct hash_table *ht_workers, DIR *input_dir)
{
  int curr_w = 0;  // Current worker index
  struct dirent *entry;
  while ((entry = readdir(input_dir)) != NULL)
  {
    char *f_name = entry->d_name;
    if (!strcmp(f_name, ".") || !strcmp(f_name, ".."))  // Ignore . and .. dirs
      continue;

    // Command worker to read the directory
    send_message(w_stats[curr_w].writ_fd, READ_DIR_CMD, f_name, buf_size);
    ht_insert(ht_workers, f_name, &w_stats[curr_w]);
    
    curr_w = (curr_w + 1) % num_workers;  // Assign dirs in round-robin fashion
  }
}

/* ========================================================================= */

// Send the server's <serverIP> the <port> number to every worker.
void send_server_info(struct worker_stats *w_stats, int num_workers, int buf_size, char *server_ip, char *port)
{
  char server_info[128];
  sprintf(server_info, "%s!%s", server_ip, port);

  for (int i = 0; i < num_workers; ++i) {
    send_message(w_stats[i].writ_fd, SERVER_INFO, server_info, buf_size);
  }
}

/* ========================================================================= */

void send_end_of_transmission(struct worker_stats *w_stats, int num_workers, int buf_size)
{
  for (int i = 0; i < num_workers; ++i)
    send_message(w_stats[i].writ_fd, END_OF_TRANSMISSION, "", buf_size);
}

/* ========================================================================= */