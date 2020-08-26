
#include "header.h"

// Read server's address and port from master (read_fd).
// Store them in <*sv_addr> and <*port>.
// Note: <*port> is in host's binary representation.
void get_sv_info(int read_fd, int buf_size, int *sv_addr, int *port);


// Receive and process a single command from master.
// Return `false` on end of transmission.
bool process_cmd_from_master(char *input_dir, int read_fd, int write_fd, int buf_size, struct list *countries);


// Process a request received from server.
void process_request(struct message *msg, int write_fd, int buf_size, struct list *countries);


// Install a signal handler for SIGINT.
// The goal is to break out of the <while> loop in main and terminate normally.
void configure_sig_int(void);