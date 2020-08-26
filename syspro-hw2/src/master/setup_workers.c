
#include "header.h"
#include "setup_workers.h"

/* ========================================================================= */

// Create a worker and his named fifos. Store his stats in <w_stats[index]>.
// Return his pid.
pid_t create_worker(struct worker_stats *w_stats, int index, char *buf_size_str, char *input_dir)
{
  char read_p[32];  // Read  end of the parent
  char writ_p[32];  // Write end of the parent
  create_unique_fifo(false, read_p, writ_p);  // Create fifos

  // Store fifos in <w_stats> array
  w_stats[index].writ_fd = open(read_p, O_RDWR | O_NONBLOCK);  // parent uses it for *writing only*
  w_stats[index].read_fd = open(writ_p, O_RDONLY | O_NONBLOCK);

  if (w_stats[index].read_fd == -1){perror("open @ 24");exit(1);}
  if (w_stats[index].writ_fd == -1){perror("open @ 25");exit(1);}

  pid_t pid = fork();
  switch (pid)
  {
    case -1:
      perror("fork");
      exit(1);
    case 0:     // Create the worker, `read_p` is the worker's write end of the fifo, `writ_p` is his read end.
      execl("./diseaseAggregator_worker", "diseaseAggregator_worker", buf_size_str, read_p, writ_p, input_dir, NULL);
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
  create_unique_fifo(true, NULL, NULL);  // Setup fifos

  char dir_path[] = "logs";         // Create a dir for logs
  if (access(dir_path, F_OK) == 0)  // If dir already exists (due to abnormal previous termination, eg: SIGKILL)
    delete_flat_dir(dir_path);

  if (mkdir("logs", 0777) == -1){perror("mkdir");exit(1);}

  char b_size_str[15];
  sprintf(b_size_str, "%d", buf_size);

  for (int i = 0; i < num_workers; ++i)
    create_worker(w_stats, i, b_size_str, input_dir);
}

/* ========================================================================= */

// Assign countries (dirs) located in <input_dir> to workers in <w_stats>.
// Map every country to the PID associated with it in <ht_workers>. 
void assign_countries(struct worker_stats *w_stats, int num_workers, struct hash_table *ht_workers, DIR *input_dir, int buf_size)
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
  for (int i = 0; i < num_workers; ++i)  // Send End of Task / Availability check
    send_message(w_stats[i].writ_fd, AVAILABILITY_CHECK, "", buf_size);
}

/* ========================================================================= */