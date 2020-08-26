
#include "header.h"
#include "network.h"



// Create a socket address with given address <addr> and <port>.
// Important: Both <addr> and <port> must be in NETWORK representation.
struct sockaddr_in *create_sockaddr(int addr, int port)
{
  struct sockaddr_in *sa = calloc(1, sizeof(struct sockaddr_in));
  sa->sin_family = AF_INET;
  sa->sin_addr.s_addr = addr;
  sa->sin_port = port;
  return sa;
}



// Connect to a server with sockaddr_in <sa>.
// Return a socket to communicate with the server.
int connect_to_server(struct sockaddr_in *sa)
{
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    error_exit("socket @ net.c");

  if (connect(sock, (struct sockaddr *) sa, sizeof(*sa)) == -1)
    error_exit("connect");

  return sock;
}



// Create and return a listening socket, bound to sockaddr_in <sa>,
// that accepts at most <max_conn> connections as a backlog queue.
int create_passive_socket(struct sockaddr_in *sa, int max_conn)
{
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    error_exit("socket @ net.c");

  int enable = 1;  // Mark the socket's address to be re-usable
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
    error_exit("setsockopt @ net.c");

  if (bind(sock, (struct sockaddr *) sa, sizeof(*sa)) == -1)
    error_exit("bind");

  if (listen(sock, max_conn) == -1)
    error_exit("listen @ net.c");
  
  return sock;
}



// Destroy sockaddr_in <sa>, returned by <create_sockaddr>.
void destroy_sockaddr(void *sa) {
  free((struct sockaddr_in *) sa);
}