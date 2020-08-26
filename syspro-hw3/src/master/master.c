
#include "header.h"

#include "setup_workers.h"
#include "sig_manage.h"
#include "sig_actions.h"
#include "validation.h"


int main(int argc, char *argv[])
{
  signals_config();
  signals_block();   // Block signals during setup

  int num_workers, buf_size;  // Command line args
  char *input_dir_path, *server_ip, *port;
  DIR *input_dir;

  if (!validate_args(argc, argv, &num_workers, &buf_size, &input_dir_path, &input_dir, &server_ip, &port))
    exit(1);   // Validate cmd line args and initialize values

  // Structures to store worker info necessary to replace him
  struct worker_stats *w_stats;
  w_stats = malloc(num_workers * sizeof (struct worker_stats));

  // Associates a <dir name> (country) with the respective worker's <worker_stats>
  struct hash_table *ht_workers;
  ht_workers = ht_create(HT_DEF_SIZE, HT_DEF_BUCK_SIZE, NULL);

  // Set up signal handlers / cleanup functions with data they need
  actions_quit(SETUP, w_stats, &num_workers);
  actions_child_term(SETUP, ht_workers, w_stats, &num_workers, &buf_size, input_dir_path, server_ip, port);
  actions_cleanup(SETUP, &num_workers, w_stats, ht_workers, input_dir);

  // Create and initialize workers
  create_n_workers(w_stats, num_workers, buf_size, input_dir_path);
  send_server_info(w_stats, num_workers, buf_size, server_ip, port);
  assign_countries(w_stats, num_workers, buf_size, ht_workers, input_dir);
  send_end_of_transmission(w_stats, num_workers, buf_size);

  signals_unblock();  // Finished the setup

  while (1)
  {
    signals_check();
    pause();  // Block until a signal arrives (SIGCHLD or termination sig)
  }

  exit(0);
}