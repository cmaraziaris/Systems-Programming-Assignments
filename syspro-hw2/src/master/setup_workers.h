#ifndef SETUP_WORKERS_H
#define SETUP_WORKERS_H


struct worker_stats
{
  pid_t w_pid;  //  worker's PID
  int read_fd;  // `master`/parent can read from here
  int writ_fd;  // `master`/parent can write here
};


// Create a worker and his named fifos. Store his stats in <w_stats[index]>.
// Return his pid.
pid_t create_worker(struct worker_stats *w_stats, int index, char *buf_size_str, char *input_dir);


// Create <n> workers. Store the stats for each worker in the array <w_stats>.
void create_n_workers(struct worker_stats *w_stats, int n, int buf_size, char *input_dir);


// Assign countries (dirs) located in <input_dir> to workers in <w_stats>.
// Map every country to the PID associated with it in <ht_workers>. 
void assign_countries(struct worker_stats *w_stats, int num_workers, struct hash_table *ht_workers, DIR *input_dir, int buf_size);


#endif