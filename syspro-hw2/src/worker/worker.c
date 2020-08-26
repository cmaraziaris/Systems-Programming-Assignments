
#include "header.h"

#include "io_files.h"
#include "queries.h"
#include "signal_handling.h"
#include "glob_structs.h"

static void process_command(int opcode, char *dec_msg, char *input_dir, int write_fd, int buf_size, struct list *open_dirs);
static void get_args(char *dec_msg, char **disease, char **country, char **entry_dt, char **exit_dt);

static int success, fail;

/* ========================================================================= */

int main(int argc, char *argv[])
{
  signals_config();
  signals_block();  // Block signals during setup

  int buf_size = atoi(argv[1]);  // Process command line args
  char *read_p = argv[2];
  char *writ_p = argv[3];
  char *input_dir = argv[4];

  setup_structures(500, 500, 200);  // Structures needed for queries

  struct list *open_dirs = list_create(free);  // Keep track of open dirs

  int read_fd = open(read_p, O_RDONLY);  // Open named pipe for reading
  if (read_fd == -1){perror("open @ worker.c 1"); exit(1);}

  int write_fd = open(writ_p, O_RDWR | O_NONBLOCK);  // Open named pipe for writing
  if (write_fd == -1){perror("open @ worker.c 2"); exit(1);}

  // Setup signal-handling functions
  write_logs(SETUP, open_dirs, &write_fd, &read_fd, &success, &fail);
  read_directory_updates(SETUP, open_dirs, &write_fd, input_dir, &buf_size, &success, &fail);

  signals_unblock();  // Finished setup

  do
  {
    signals_block();
    signals_check();
    signals_unblock();

    char *msg = read_message(read_fd, buf_size);

    signals_block();

    if (msg == NULL)  // If `read_message` got interrupted by a signal, catch the signal
      continue;       // and try to read again

    int opcode = get_opcode(msg);
    char dec_msg[strlen(msg)];
    strcpy(dec_msg, decode_message(msg));
    free(msg);

    process_command(opcode, dec_msg, input_dir, write_fd, buf_size, open_dirs);

  } while (1);

  exit(0);
}

/* ========================================================================= */

// Process the command received
static void process_command(int opcode, char *dec_msg, char *input_dir, int write_fd, int buf_size, struct list *open_dirs)
{
  if (opcode == AVAILABILITY_CHECK) {
    send_message(write_fd, WORKER_READY, "", buf_size);  // Send "Ready" signal
  }
  else if (opcode == READ_DIR_CMD || opcode == READ_DIR_FORK)  // Message is a dir to read from
  {
    char *country = dec_msg;
    struct country_dir *cdir = read_directory(opcode, country, input_dir, write_fd, buf_size, &success, &fail);
    list_insert_first(open_dirs, cdir);  // Update current open dirs
  }
  else if (opcode == SEARCH_PATIENT)
  {
    char *rec_id = dec_msg;
    q_search_patient(rec_id, write_fd, buf_size);
  }
  else
  {
    char *disease, *country, *entry_dt, *exit_dt;
    get_args(dec_msg, &disease, &country, &entry_dt, &exit_dt);

    if (opcode == DISEASE_FREQ) {
      q_disease_frequency(disease, country, entry_dt, exit_dt, write_fd, buf_size);
    }
    else if (opcode == NUM_PAT_ADM) {
      q_num_pat_admissions(disease, country, entry_dt, exit_dt, write_fd, buf_size, open_dirs);
    }
    else if (opcode == NUM_PAT_DIS) {
      q_num_pat_discharges(disease, country, entry_dt, exit_dt, write_fd, buf_size, open_dirs);
    }
  }
}

/* ========================================================================= */

// Parse arguments for various queries
static void get_args(char *dec_msg, char **disease, char **country, char **entry_dt, char **exit_dt)
{
  char *save;
  *disease = strtok_r(dec_msg, ": \n", &save);
  *entry_dt = strtok_r(NULL, ":  \n", &save);
  *exit_dt = strtok_r(NULL, ": \n", &save);
  *country = strtok_r(NULL, ": \n", &save);  
}

/* ========================================================================= */