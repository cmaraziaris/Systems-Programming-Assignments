
#include "header.h"

#include "glob_structs.h"
#include "operate.h"

/* ========================================================================= */

// Find the port number from the address of a socket and convert it to string.
static void get_sock_port_str(char *port_s, int sock, struct sockaddr_in *addr)
{
  unsigned int len = sizeof(*addr);
  getsockname(sock, (struct sockaddr *) addr, &len);
  sprintf(port_s, "%hu", ntohs(addr->sin_port));  // Store it in <port_s>
}

/* ========================================================================= */

int main(int argc, char *argv[])
{
  int buf_size = atoi(argv[1]);  // Process command line args
  char *read_path = argv[2];
  char *input_dir = argv[3];
  bool is_repl = atoi(argv[4]) ? true : false;

  // Structures needed for queries
  setup_structures(500, 500, 200);
  struct list *countries = list_create(free);  // Keep track of assigned countries

  // Worker quits gracefully with SIGINT when *idle* (blocked in accept())
  configure_sig_int();

  // Open named pipe for reading from master
  int read_pipe;
  if ((read_pipe = open(read_path, O_RDONLY)) == -1)
    error_exit("open");

  // Create a passive socket
  char port_s[16];
  struct sockaddr_in *passive_addr = create_sockaddr(htonl(INADDR_ANY), htons(0));
  int lstn_sock = create_passive_socket(passive_addr, SOMAXCONN);
  get_sock_port_str(port_s, lstn_sock, passive_addr);
  destroy_sockaddr(passive_addr);

  // Read server's address and port from master & connect
  int sv_port, sv_addr_int;
  get_sv_info(read_pipe, buf_size, &sv_addr_int, &sv_port);
  
  struct sockaddr_in *sv_addr = create_sockaddr(sv_addr_int, htons(sv_port));
  int write_sock = connect_to_server(sv_addr);
  
  // Inform the server if this is a replacement worker and send the listening port
  int op_code = (is_repl == false) ? WORKER_LSTN_PORT : WORKER_LSTN_FORK;
  send_message(write_sock, op_code, port_s, buf_size);

  // Read directories from master, forward reports to server
  while (process_cmd_from_master(input_dir, read_pipe, write_sock, buf_size, countries))
    ;
  close_w(read_pipe);  // Terminate connection with master

  // Worker finished sending stats, terminate connection with server
  send_message(write_sock, END_OF_TRANSMISSION, "0", buf_size);
  read_message(NULL, write_sock, buf_size);  // Read EOT

  close_w(write_sock);
  destroy_sockaddr(sv_addr);

  printf("Worker with PID: %d is listening at port: %s\n", getpid(), port_s);

  struct message msg;
  while (1)  // Answer requests from server
  {
    int read_sock;
    if ((read_sock = accept(lstn_sock, NULL, NULL)) == -1)
    {
      if (errno == EINTR)  // We catch interrupts from SIGINT
        break;
      else
        error_exit("accept");
    }

    read_message(&msg, read_sock, buf_size);  // Read request
    process_request(&msg, read_sock, buf_size, countries);

    close_w(read_sock);  // Terminate connection
    destroy_message(&msg);
  }

  list_destroy(countries);  // Cleanup
  cleanup_structures();

  exit(0);
}

/* ========================================================================= */