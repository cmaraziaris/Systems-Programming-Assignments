
#include <sys/socket.h>


// Create a socket address with given address <addr> and <port>.
// Important: Both <addr> and <port> must be in NETWORK representation.
struct sockaddr_in *create_sockaddr(int addr, int port);


// Connect to a server with sockaddr_in <sa>.
// Return a socket to communicate with the server.
int connect_to_server(struct sockaddr_in *sa);


// Create and return a listening socket, bound to sockaddr_in <sa>,
// that accepts at most <max_conn> connections as a backlog queue.
int create_passive_socket(struct sockaddr_in *sa, int max_conn);


// Destroy sockaddr_in <sa>, returned by <create_sockaddr>.
void destroy_sockaddr(void *sa);