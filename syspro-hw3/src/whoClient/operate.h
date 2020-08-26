
#include <pthread.h>

#include "header.h"


// Sends a <request> to the server, receives and prints the answer.
void *make_request(void *request);


// Setup the prerequisites needed to connect with the server.
// Return a <struct sockaddr_in *> with the server's info.
struct sockaddr_in *prepare_server_connection(char *server_ip, char *port_s);


// Barrier to synchronize threads that make requests
extern pthread_barrier_t barrier;


// Server address to connect
extern struct sockaddr_in *sv_addr;