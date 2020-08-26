#ifndef SETUP_WORKERS_H
#define SETUP_WORKERS_H


struct worker_stats
{
  pid_t w_pid;  //  worker's PID
  int writ_fd;  // `master`/parent can write here
};


// Create a worker and his named fifo. Store his stats in <w_stats[index]>. Return his pid.
// Inform the new worker if he's a replacement for a dead one.
pid_t create_worker(struct worker_stats *w_stats, int index, char *buf_s_str, char *input_dir, int is_replacement);


// Create <n> workers. Store the stats for each worker in the array <w_stats>.
void create_n_workers(struct worker_stats *w_stats, int n, int buf_size, char *input_dir);


// Assign countries (dirs) located in <input_dir> to workers in <w_stats>.
// Map every country to the PID associated with it in <ht_workers>. 
void assign_countries(struct worker_stats *w_stats, int num_workers, int buf_size, struct hash_table *ht_workers, DIR *input_dir);


// Send the server's <serverIP> the <port> number to every worker.
void send_server_info(struct worker_stats *w_stats, int num_workers, int buf_size, char *serverIP, char *port);


// Send END_OF_TRANSMISSION message to every worker, so that they can terminate the communication.
void send_end_of_transmission(struct worker_stats *w_stats, int num_workers, int buf_size);


#endif