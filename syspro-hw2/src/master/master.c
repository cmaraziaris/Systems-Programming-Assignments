
#include "header.h"

#include "m_queries.h"
#include "setup_workers.h"
#include "sig_manage.h"
#include "sig_actions.h"
#include "validation.h"

// Process a user command. If needed, forward the request to the required workers.
// Returns 1 if the command given was /exit, else 0.
static int  process_cmd(char *line, int cmd, int *ready_workers, int *total, int num_workers, struct worker_stats *w_stats, struct hash_table *ht_workers, struct hash_table *ht_ranges, int buf_size);

// Handle data provided by workers. Determine the action on the <dec_msg> based on its <opcode>.
static void process_msg(int opcode, char *dec_msg, int *total, int *ready_workers, struct hash_table *ht_ranges);


static int available_updates;   // # of reports to be received (after USR1 signals)
static int successful, failed;  // Queries Pass/Fail counters

/* ========================================================================= */

int main(int argc, char *argv[])
{
  signals_config();  // Signals blocked during a command process
  signals_block();   // Block signals during setup

  int num_workers, buf_size;  // Command line args
  char *input_dir_path;
  DIR *input_dir;

  if (validate_args(argc, argv, &num_workers, &buf_size, &input_dir_path, &input_dir) == false)
    exit(1);   // Validate cmd line args and initialize values

  // Structures to store worker info
  struct worker_stats *w_stats;  // Stores worker pid & pipe ends
  w_stats = calloc(num_workers, sizeof (struct worker_stats));

  struct hash_table *ht_workers;  // Associates a <file name> (country) with the respective worker's <worker_stats>
  ht_workers = ht_create(HT_DEF_SIZE, HT_DEF_BUCK_SIZE, NULL);

  create_n_workers(w_stats, num_workers, buf_size, input_dir_path);
  assign_countries(w_stats, num_workers, ht_workers, input_dir, buf_size);

  // Structure required for the topk-AgeRanges query
  struct hash_table *ht_ranges = ht_create(HT_DEF_SIZE, HT_DEF_BUCK_SIZE, ht_destroy);

  int ready_workers = 0;  // # of workers ready to receive commands

  // Set up signal handlers / cleanup functions with data they need
  actions_usr2(SETUP, &available_updates);
  actions_quit(SETUP, w_stats, &num_workers, ht_workers, &successful, &failed);
  actions_child_term(SETUP, ht_workers, w_stats, &num_workers, &buf_size, input_dir_path, &ready_workers);
  actions_cleanup(SETUP, &num_workers, w_stats, ht_workers, ht_ranges, input_dir);

  int total = 0;   // Counter for the diseaseFrequency query
  int cmd = NONE;  // Last command given by the user

  fd_set read_ends;  // select()

  signals_unblock();  // Finished the setup
  signals_block();
  signals_check();

  while (1)
  {
    if (ready_workers != num_workers || available_updates)  // Wait for worker(s)' response
    {
      FD_ZERO(&read_ends);
      for (int i = 0; i < num_workers; ++i)
        FD_SET(w_stats[i].read_fd, &read_ends);  // Init read pipes' ends

      int status = select(FD_SETSIZE, &read_ends, NULL, NULL, NULL);
      if (status == -1){ perror("select"); continue; }
      for (int i = 0; i < num_workers; ++i)
      {
        if (FD_ISSET(w_stats[i].read_fd, &read_ends))  // Find the pipe that has data to read from
        {
          char *msg = read_message(w_stats[i].read_fd, buf_size);
          int opcode = get_opcode(msg);  // Operation code
          
          char dec_msg[strlen(msg)];     // Decoded message
          strcpy(dec_msg, decode_message(msg));
          free(msg);

          process_msg(opcode, dec_msg, &total, &ready_workers, ht_ranges);
        }
      }
    }
    else   // Every worker is ready to receive requests
    {
      if (cmd == DISEASE_FREQ)  // If the last command given was diseaseFrequency
      {
        printf("%d\n", total);
        cmd = NONE;
      }

      int len = MAX_CMD_LENGTH;
      char line[len];

      signals_unblock();
      signals_block();
      signals_check();
      signals_unblock();

      if (fgets(line, len, stdin) == NULL)  // Get user input
      {
        signals_block();
        signals_check();
        continue;
      }

      signals_block();  // Block signals while processing commands.

      cmd = validate_cmd(line);
      if (cmd == UNKNOWN_CMD)
      {
        ++failed;  // Invalid command
        continue;
      }

      ++successful;
      
      if (process_cmd(line, cmd, &ready_workers, &total, num_workers, w_stats, ht_workers, ht_ranges, buf_size) == 1)
        break;  // /exit command was given
    }
  }

  exit(0);
}

/* ========================================================================= */

// Handle data provided by workers. Determine the action on the <dec_msg> based on its <opcode>.
static void process_msg(int opcode, char *dec_msg, int *total, int *ready_workers, struct hash_table *ht_ranges)
{
  if (opcode == WORKER_READY) {  // Ready check
    ++(*ready_workers);
  }
  else if (opcode == FILE_REPORT) {  // Worker is sending a file report after init assignment
    q_add_report(UPDATE_DATA, ht_ranges, dec_msg);
  }
  else if (opcode == FILE_REPORT_FORK) {  // Replacement worker (after SIGCHLD)
    q_add_report(JUST_PRINT, NULL, dec_msg);
  }
  else if (opcode == FILE_REPORT_SIG)  // Worker is sending a file report after SIGUSR1
  {
    q_add_report(UPDATE_DATA, ht_ranges, dec_msg);
    --available_updates;
  }
  else if (opcode == SEARCH_RESULT_SUCCESS) {
    printf("%s\n", dec_msg);
  }
  else if (opcode == DISEASE_FREQ_RESULT) {  // Query results
    *total += atoi(dec_msg);
  }
  else if (opcode == NUM_PAT_ADM_RESULT || opcode == NUM_PAT_DIS_RESULT)
  {
    char *stok_save;
    char *country = strtok_r(dec_msg, ";", &stok_save);
    int cases = atoi(strtok_r(NULL, " \n", &stok_save));
    if (cases == 0)
      return;
    printf("%s %d\n", country, cases);
  }
}

/* ========================================================================= */

// Process a user command. If needed, forward the request to the required workers.
// Returns 1 if the command given was /exit, else 0.
static int process_cmd(char *line, int cmd, int *ready_workers, int *total, int num_workers, struct worker_stats *w_stats, struct hash_table *ht_workers, struct hash_table *ht_ranges, int buf_size)
{
  char *stok_save;
  strtok_r(line, " \n", &stok_save);  // Initialize strtok_r
  
  if (cmd == LIST_COUNTRIES) {
    q_list_countries(ht_workers);
  }
  else if (cmd == SEARCH_PATIENT) {
    *ready_workers = 0;
    q_search_patient(w_stats, num_workers, buf_size, &stok_save);
  }
  else if (cmd == DISEASE_FREQ)
  {
    *total = 0;  // Result will be stored here
    *ready_workers = 0;
    q_operate(DISEASE_FREQ, num_workers, w_stats, ht_workers, buf_size, &stok_save);
  }
  else if (cmd == NUM_PAT_ADM)
  {
    *ready_workers = 0;
    q_operate(NUM_PAT_ADM, num_workers, w_stats, ht_workers, buf_size, &stok_save);
  }
  else if (cmd == NUM_PAT_DIS) {
    *ready_workers = 0;
    q_operate(NUM_PAT_DIS, num_workers, w_stats, ht_workers, buf_size, &stok_save);
  }
  else if (cmd == TOPK_AGE) {
    q_find_topk(ht_ranges, &stok_save);
  }
  else if (cmd == EXIT_CMD)
  {
    quit_gracefully();
    return 1;
  }

  return 0;
}

/* ========================================================================= */