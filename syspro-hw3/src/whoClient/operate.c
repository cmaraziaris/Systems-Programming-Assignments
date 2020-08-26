
#include "operate.h"

#define BUF_SIZE 512


static void process_answer(int sock, char *request);
static void print_answer(struct list *results, char *request);


pthread_barrier_t barrier;    // Barrier to synchronize threads that make requests
struct sockaddr_in *sv_addr;  // Server address to connect

/* ========================================================================= */

// Sends a <request> to the server, receives and prints the answer.
void *make_request(void *request)
{
  pthread_barrier_wait(&barrier);  // Block here until <num_threads> are spawned

  if (request == NULL)
    return NULL;  // "Dummy" thread, no actual request
  
  // Connect and send the request
  int sock = connect_to_server(sv_addr);
  send_message(sock, REQUEST_CLIENT, (char *) request, BUF_SIZE);

  process_answer(sock, (char *) request);  // Output answer
  send_message(sock, RESPONSE_RECEIVED, "0", BUF_SIZE);
  read_message(NULL, sock, BUF_SIZE);      // Wait for server's "ACK"

  close_w(sock);
  free(request);  // Cleanup

  return NULL;
}

/* ========================================================================= */

// Get the server's answer for the <request>.
static void process_answer(int sock, char *request)
{
  struct message msg;
  struct list *results = list_create(free);  // Store the results here

  while (1)  // Get results
  {
    read_message(&msg, sock, BUF_SIZE);

    if (msg.opcode == END_OF_TRANSMISSION)  // Server finished sending results
    {
      destroy_message(&msg);
      break;
    }

    list_insert_first(results, strdup(msg.body));
    destroy_message(&msg);
  }

  print_answer(results, request);
  list_destroy(results);
}


// Prints the request and its result to stdout.
static void print_answer(struct list *results, char *request)
{
  int size = list_size(results);
  char *arr[size];
  for (int i = 0; i < size; ++i)      // Convert the list to an array
    arr[i] = list_get(results, i+1);  // So we use stdout as little time as possible

  flockfile(stdout);  // Lock stdout from other threads

  printf("\n%s", request);

  for (int i = 0; i < size; ++i)
    printf("%s\n", arr[i]);

  funlockfile(stdout);  // We're done using stdout  
}

/* ========================================================================= */

// Setup the prerequisites needed to connect with the server.
// Return a <struct sockaddr_in *> with the server's info.
struct sockaddr_in *prepare_server_connection(char *server_ip, char *port_s)
{
  struct hostent *rem;
  if ((rem = gethostbyname(server_ip)) == NULL)
  {
    herror("gethostbyname"); 
    exit(1);
  }

  int addr;
  memcpy(&addr, rem->h_addr, rem->h_length);
  return create_sockaddr(addr, htons(atoi(port_s)));
}

/* ========================================================================= */