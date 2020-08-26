
#include "operate.h"
#include "queries.h"
#include "io_files.h"

/* ========================================================================= */

// Receive and process a single command from master.
// Return `false` on end of transmission.
bool process_cmd_from_master(char *input_dir, int read_fd, int write_fd, int buf_size, struct list *countries)
{
  bool received_eot = true;  // Indicates End of Transmission from master
  struct message msg;
  read_message(&msg, read_fd, buf_size);

  if (msg.opcode == READ_DIR_CMD || msg.opcode == READ_DIR_FORK)  // Message is a dir (country) to read from
  {
    // Read the country directory and send reports to the server
    char *country = msg.body; 
    read_directory(msg.opcode, country, input_dir, write_fd, buf_size);
    
    list_insert_first(countries, strdup(country));  // Update countries assigned to this worker
    received_eot = false;
  }

  destroy_message(&msg);
  return (!received_eot);
}

/* ========================================================================= */

static void get_args(char *body, char **disease, char **country, char **entry_dt, char **exit_dt);

// Process a request received from server.
// The called "q_*" function will find and send the answer to the server.
void process_request(struct message *msg, int write_fd, int buf_size, struct list *countries)
{
  int op = msg->opcode;

  if (op == SEARCH_PATIENT)
  {
    char *rec_id = msg->body;
    q_search_patient(rec_id, write_fd, buf_size);
  }
  else if (op == TOPK_AGE) {
    q_find_topk(msg->body, write_fd, buf_size);
  }
  else
  {
    char *disease, *country, *entry_dt, *exit_dt;
    get_args(msg->body, &disease, &country, &entry_dt, &exit_dt);

    if (op == DISEASE_FREQ) {
      q_disease_frequency(disease, country, entry_dt, exit_dt, write_fd, buf_size);
    }
    else if (op == NUM_PAT_ADM) {
      q_num_pat_admissions(disease, country, entry_dt, exit_dt, write_fd, buf_size, countries);
    }
    else if (op == NUM_PAT_DIS) {
      q_num_pat_discharges(disease, country, entry_dt, exit_dt, write_fd, buf_size, countries);
    }
  }

  send_message(write_fd, END_OF_TRANSMISSION, "0", buf_size);
  read_message(NULL, write_fd, buf_size);  // Read EOT
  send_message(write_fd, END_OF_TRANSMISSION, "0", buf_size);  // Send "ACK"
}

/* ========================================================================= */

// Parse arguments for various queries.
static void get_args(char *body, char **disease, char **country, char **entr_dt, char **exit_dt)
{
  char *save;
  *disease = strtok_r(body, ": \n", &save);
  *entr_dt = strtok_r(NULL, ": \n", &save);
  *exit_dt = strtok_r(NULL, ": \n", &save);
  *country = strtok_r(NULL, ": \n", &save);  
}

/* ========================================================================= */

// Read server's address and port from master (read_fd).
// Store them in <*sv_addr> and <*port>.
void get_sv_info(int read_fd, int buf_size, int *sv_addr, int *port)
{
  char *stok_save, serv_ip[256];
  struct message msg;
  read_message(&msg, read_fd, buf_size);
  
  strncpy(serv_ip, strtok_r(msg.body, "! ", &stok_save), 256);
  *port = atoi(strtok_r(NULL, " !", &stok_save));  // <*port> is in host format

  destroy_message(&msg);

  // Convert server's address
  struct hostent *rem;
  if ((rem = gethostbyname(serv_ip)) == NULL) {herror("gethostbyname"); exit(1);}
  memcpy(sv_addr, rem->h_addr, rem->h_length);
}

/* ========================================================================= */

static void quit(int signo)
{ /* Just get out of the while loop by interrupting accept() */ }

// Install a signal handler for SIGINT.
// The purpose is to break out of the <while> loop in main and terminate normally.
void configure_sig_int(void)
{
  sigset_t inter_set;
  struct sigaction act;
  
  memset(&act, 0, sizeof(struct sigaction));

  sigfillset(&act.sa_mask);  // Block all other signals while handling one
  
  act.sa_handler = quit;  // Install signal handler
  sigaction(SIGINT,  &act, NULL);

  sigemptyset(&inter_set);
  sigaddset(&inter_set, SIGINT);
  sigprocmask(SIG_UNBLOCK, &inter_set, NULL);  // Unblock SIGINT
}

/* ========================================================================= */