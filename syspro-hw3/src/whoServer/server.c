
#include "header.h"
#include "validate.h"
#include "circular_buf.h"
#include "worker_stats.h"
#include "client_requests.h"

#define BUF_SIZE 512

static void *handle_request(void *arg);
static void process_msg(int sock, struct hash_table *ht_workers, struct list *l_workers);

static struct hash_table *ht_workers;  // Associates a <file name> (country) with the respective worker's <struct sockaddr_in>
static struct list *l_workers;  // Keep every worker's connection info (<struct sockaddr_in>)
static struct cbuf *circ_buf;   // Circular buffer

static void quit(int signo) { write(STDERR_FILENO, "\n\nExitting...\n\n", 15); } 

/* ========================================================================= */

int main(int argc, char *argv[])
{
  int num_threads, query_port, stats_port, buf_size;  // Command line args

  if (validate_args(argc, argv, &num_threads, &query_port, &stats_port, &buf_size) == false)
    exit(1);   // Validate cmd line args and initialize values

  signal(SIGINT, quit);  // App quits gracefully with SIGINT when *idle* (blocked in select())

  circ_buf   = cb_create(buf_size);
  ht_workers = ht_create(HT_DEF_SIZE, HT_DEF_BUCK_SIZE, NULL);  // Init structures
  l_workers  = list_create(destroy_sockaddr);

  // Create passive sockets
  struct sockaddr_in *stats_addr = create_sockaddr(htonl(INADDR_ANY), htons(stats_port));
  struct sockaddr_in *query_addr = create_sockaddr(htonl(INADDR_ANY), htons(query_port));

  int stats_sock = create_passive_socket(stats_addr, SOMAXCONN);
  int query_sock = create_passive_socket(query_addr, SOMAXCONN);

  // Initialize the set of active (listening) sockets
  fd_set active_fd_set;
  FD_ZERO(&active_fd_set);
  FD_SET(stats_sock, &active_fd_set);
  FD_SET(query_sock, &active_fd_set);

  // Create and initiate threads
  pthread_t threads[num_threads];
  for (int i = 0; i < num_threads; ++i)
    pthread_create(&threads[i], NULL, handle_request, NULL);

  while (1)
  {
    fd_set read_fd_set = active_fd_set;
    if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) == -1)
    {
      if (errno == EINTR)  // We handle sig interrupts from SIGINT
        break;
      error_exit("select");
    }

    for (int i = 0; i < FD_SETSIZE; ++i)
    {
      if (FD_ISSET(i, &read_fd_set))  // Find the listening socket with the incoming connection
      {
        int new_sock;
        if ((new_sock = accept(i, NULL, NULL)) == -1) error_exit("accept");

        // Place the incoming connection in the circular buffer
        cb_insert(circ_buf, new_sock);
        pthread_cond_broadcast(&cb_cond_non_empty);  // Notify threads
      }
    }
  }

  cb_cond_destroy(num_threads, circ_buf);
  for (int i = 0; i < num_threads; ++i)  // Wait for threads to terminate
    pthread_join(threads[i], NULL);

  destroy_sockaddr(stats_addr);
  destroy_sockaddr(query_addr);  // Cleanup
  list_destroy(l_workers);
  ht_destroy(ht_workers);
  cb_destroy(circ_buf);
  close_w(stats_sock);
  close_w(query_sock);

  exit(0);
}

/* ========================================================================= */

// Thread function that handles requests from incoming connections to the server.
static void *handle_request(void *arg)
{
  int sock;

  while ((sock = cb_remove(circ_buf)) != -1)  // Get an incoming connection from buffer
  {
    pthread_cond_signal(&cb_cond_non_full);  // Notify main thread
    process_msg(sock, ht_workers, l_workers);  // Process incoming messages
    close_w(sock);  // Terminate connection
  }

  return NULL;
}


static void process_msg(int sock, struct hash_table *ht_workers, struct list *l_workers)
{
  struct message msg;
  read_message(&msg, sock, BUF_SIZE);

  if (msg.opcode == REQUEST_CLIENT)
  {
    process_client_request(msg.body, sock, BUF_SIZE, ht_workers, l_workers);
  }
  else if (msg.opcode == WORKER_LSTN_PORT || msg.opcode == WORKER_LSTN_FORK)
  {
    process_worker_stats(msg.opcode, msg.body, sock, BUF_SIZE, ht_workers, l_workers);
  }

  destroy_message(&msg);
}

/* ========================================================================= */