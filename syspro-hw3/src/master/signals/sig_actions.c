
#include "header.h"
#include "fifo_dir.h"
#include "setup_workers.h"
#include "sig_actions.h"

/* ========================================================================= */

static void reassign_countries(pid_t new_pid, struct worker_stats *w_stats, struct hash_table *ht_workers, int index, int buf_size);

// Replace a child that terminated unexpectedly.
void actions_child_term(bool at_setup, struct hash_table *pht_workers, struct worker_stats *pw_stats, int *pnum_workers, int *pbuf_size, char *pinput_dir, char *pserv_ip, char *pport)
{
  static struct hash_table *ht_workers;
  static struct worker_stats *w_stats;
  static int num_workers, buf_size;
  static char *input_dir, *server_ip, *port;
  
  if (at_setup == true)   // Setup process
  {
    ht_workers = pht_workers;     // this function can be called
    w_stats = pw_stats;           // when checking for signals
    num_workers = *pnum_workers;
    buf_size = *pbuf_size;
    input_dir = pinput_dir;
    server_ip = pserv_ip;
    port = pport;
    return;
  }

  char b_size_str[15];
  sprintf(b_size_str, "%d", buf_size);

  pid_t child;     // Catch every worker that died (in case of multiple SIGCHLD signals)
  while ((child = waitpid(-1, NULL, WNOHANG)) > 0)
  {
    int index; 
    for (index = 0; index < num_workers; ++index)  // Find the terminated child's stats
      if (w_stats[index].w_pid == child)
        break;

    // Close connection with the term'ed child
    if (close(w_stats[index].writ_fd) == -1) error_exit("close @ act");

    // Create a new worker and replace the term'ed pid in <w_stats>
    pid_t new_pid = create_worker(w_stats, index, b_size_str, input_dir, 1);

    // Inform the new worker of the server info
    char server_info[128];
    sprintf(server_info, "%s!%s", server_ip, port);
    send_message(w_stats[index].writ_fd, SERVER_INFO, server_info, buf_size);

    reassign_countries(new_pid, w_stats, ht_workers, index, buf_size);
    send_message(w_stats[index].writ_fd, END_OF_TRANSMISSION, "", buf_size);
  }
}

/* ========================================================================= */

// Assign to worker with pid <new_pid> the countries (DIRs) that belonged to a terminated child, indicated by <index>.
static void reassign_countries(pid_t new_pid, struct worker_stats *w_stats, struct hash_table *ht_workers, int index, int buf_size)
{
  struct bucket_entry *entry;
  while ((entry = ht_traverse(ht_workers)) != NULL)  // Find every directory assigned to the terminated child.
  {
    struct worker_stats *worker = entry->data;   // We replaced the terminated pid with the new pid (create_worker).
    if (worker->w_pid != new_pid)                // So, countries are already associated with the new pid.
      continue;

    char *country = entry->key;  // Command the replacement worker to read the dir.
    send_message(w_stats[index].writ_fd, READ_DIR_FORK, country, buf_size);
  }
}

/* ========================================================================= */

// Quits gracefully. Outputs logs, kills children, waits for them to terminate and cleanups memory and open files.
void actions_quit(bool at_setup, struct worker_stats *pw_stats, int *pnum_workers)
{
  static struct worker_stats *w_stats;
  static int num_workers;

  if (at_setup == true)   // Setup process
  {
    w_stats = pw_stats;            // this function can be called
    num_workers = *pnum_workers;   // when checking for signals
    return;
  }

  for (int i = 0; i < num_workers; ++i)  // Kill every `worker`
    kill(w_stats[i].w_pid, SIGKILL);
  
  for (int i = 0; i < num_workers; ++i)  // Wait for children
    wait(NULL);

  actions_cleanup(false, NULL, NULL, NULL, NULL);  // Cleanup mem used
}

/* ========================================================================= */

// Cleanup memory and files/fifos/directories used by the app.
void actions_cleanup(bool at_setup, int *pnum_workers, struct worker_stats *pw_stats, struct hash_table *pht_workers, DIR *pinput_dir)
{
  static int num_workers;
  static struct worker_stats *w_stats;
  static struct hash_table *ht_workers;
  static DIR *input_dir;

  if (at_setup == true)   // Setup process
  {
    num_workers = *pnum_workers;  // this function can be called
    w_stats = pw_stats;           // when checking for signals
    ht_workers = pht_workers;
    input_dir = pinput_dir;
    return;
  }

  ht_destroy(ht_workers);

  for (int i = 0; i < num_workers; ++i)  // Close fifo for every worker
    if (close(w_stats[i].writ_fd) == -1){perror("close @ cleanup"); exit(1);}

  free(w_stats);
  if (closedir(input_dir) == -1){perror("closedir @ cleanup"); exit(1);}

  delete_flat_dir("named_fifos");  // Delete dir "named_fifos"
}

/* ========================================================================= */