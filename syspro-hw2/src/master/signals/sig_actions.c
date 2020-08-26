
#include "header.h"
#include "setup_workers.h"
#include "m_queries.h"

#include "sig_actions.h"

/* ========================================================================= */

// If SIGUSR2 is received, increment available reports to be read by `master`.
void actions_usr2(bool at_setup, int *pavailable)
{
  static int *available_updates;
  if (at_setup == true)
    available_updates = pavailable;
  else
    ++(*available_updates);
}

/* ========================================================================= */

static void output_logs(struct hash_table *ht_workers, int succ, int fail);

// Quits gracefully. Outputs logs, kills children, waits for them to terminate and cleanups memory and open files.
void actions_quit(bool at_setup, struct worker_stats *pw_stats, int *pnum_workers, struct hash_table *pht_worker, int *psucc, int *pfail)
{
  static struct worker_stats *w_stats;
  static struct hash_table *ht_worker;
  static int num_workers;
  static int *succ, *fail;

  if (at_setup == true)   // Setup process
  {
    w_stats = pw_stats;
    ht_worker = pht_worker;   // this function can be called
    num_workers = *pnum_workers;   // when checking for signals
    succ = psucc;
    fail = pfail;
    return;
  }

  output_logs(ht_worker, *succ, *fail);

  for (int i = 0; i < num_workers; ++i)  // Kill every `worker`
    kill(w_stats[i].w_pid, SIGKILL);
  
  for (int i = 0; i < num_workers; ++i)  // Wait for children
    wait(NULL);

  actions_cleanup(false, NULL, NULL, NULL, NULL, NULL);  // Cleanup mem used
}

/* ========================================================================= */

// Create a logfile and output stats.
static void output_logs(struct hash_table *ht_workers, int succ, int fail)
{
  int wrtn;
  char buf[50];
  snprintf(buf, 50, "logs/log_file.%d", getpid());

  int fd = creat(buf, 0666);
  if (fd == -1){perror("creat"); exit(EXIT_FAILURE);}

  struct bucket_entry *entry;
  while ((entry = ht_traverse(ht_workers)) != NULL)
  {
    char *country = entry->key;

    wrtn = write(fd, country, strlen(country));
    if (wrtn == -1){perror("write"); exit(EXIT_FAILURE);}

    wrtn = write(fd, "\n", 1);
    if (wrtn == -1){perror("write"); exit(EXIT_FAILURE);}
  }

  int total = succ + fail;
  snprintf(buf, 50, "TOTAL %d\nSUCCESS %d\nFAIL %d\n", total, succ, fail);

  wrtn = write(fd, buf, strlen(buf));
  if (wrtn == -1){perror("write"); exit(EXIT_FAILURE);}

  if (close(fd) == -1){perror("fclose"); exit(EXIT_FAILURE);}  
}

/* ========================================================================= */

// Cleanup memory and files/fifos/directories used by the app.
void actions_cleanup(bool at_setup, int *pnum_workers, struct worker_stats *pw_stats, struct hash_table *pht_workers, struct hash_table *pht_ranges, DIR *pinput_dir)
{
  static int num_workers;
  static struct worker_stats *w_stats;
  static struct hash_table *ht_workers, *ht_ranges;
  static DIR *input_dir;

  if (at_setup == true)   // Setup process
  {
    num_workers = *pnum_workers;  // this function can be called
    w_stats = pw_stats;           // when checking for signals
    ht_workers = pht_workers;
    ht_ranges = pht_ranges;
    input_dir = pinput_dir;
    return;
  }

  ht_destroy(ht_ranges);
  ht_destroy(ht_workers);

  for (int i = 0; i < num_workers; ++i)  // Close fifos for every worker
  {
    if (close(w_stats[i].writ_fd) == -1){perror("close @ cleanup"); exit(EXIT_FAILURE);}
    if (close(w_stats[i].read_fd) == -1){perror("close @ cleanup"); exit(EXIT_FAILURE);}
  }
  free(w_stats);
  if (closedir(input_dir) == -1){perror("closedir @ cleanup"); exit(EXIT_FAILURE);}

  delete_flat_dir("named_fifos");  // Delete dir "named_fifos"
}

/* ========================================================================= */

static void reassign_countries(pid_t new_pid, struct worker_stats *w_stats, struct hash_table *ht_workers, int index, int buf_size);

// Replace a child that terminated unexpectedly.
void actions_child_term(bool at_setup, struct hash_table *pht_workers, struct worker_stats *pw_stats, int *pnum_workers, int *pbuf_size, char *pinput_dir, int *pready_workers)
{
  static struct hash_table *ht_workers;
  static struct worker_stats *w_stats;
  static int num_workers, buf_size, *ready_workers;
  static char *input_dir;
  
  if (at_setup == true)   // Setup process
  {
    ht_workers = pht_workers;     // this function can be called
    w_stats = pw_stats;           // when checking for signals
    num_workers = *pnum_workers;
    buf_size = *pbuf_size;
    input_dir = pinput_dir;
    ready_workers = pready_workers;
    return;
  }

  char b_size_str[15];
  sprintf(b_size_str, "%d", buf_size);

  pid_t child;     // Catch every worker that died (in case of multiple SIGCHLD signals)
  while ((child = waitpid(-1, NULL, WNOHANG)) > 0)
  {
    --(*ready_workers);
  
    int index; 
    for (index = 0; index < num_workers; ++index)  // Find the terminated child's stats
      if (w_stats[index].w_pid == child)
        break;

    // Close connections with the term'ed child
    if (close(w_stats[index].read_fd) == -1){perror("close @ child_term"); exit(1);}
    if (close(w_stats[index].writ_fd) == -1){perror("close @ child_term"); exit(1);}

    // Create a new worker and replace the term'ed pid in <w_stats>
    pid_t new_pid = create_worker(w_stats, index, b_size_str, input_dir);

    reassign_countries(new_pid, w_stats, ht_workers, index, buf_size);
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

    char *country = entry->key;  // Command worker to read the dir.
    send_message(w_stats[index].writ_fd, READ_DIR_FORK, country, buf_size);
  }

  send_message(w_stats[index].writ_fd, AVAILABILITY_CHECK, "", buf_size);
}

/* ========================================================================= */