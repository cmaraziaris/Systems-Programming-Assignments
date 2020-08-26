
#include <pthread.h>

#include "worker_stats.h"

// These structures are shared among threads
static pthread_mutex_t list_mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t hash_mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t waddr_mtx = PTHREAD_MUTEX_INITIALIZER;

static void replace_worker_port(int sock, int port, int buf_size, struct hash_table *ht_workers);
static void upd_worker_file_reports(int sock, int buf_size, struct sockaddr_in *work_addr, struct hash_table *ht_workers);

/* ========================================================================= */

// Decide how to handle worker statistics, either by inserting a new worker or replacing a dead one.
void process_worker_stats(int opcode, char *port_s, int sock, int buf_size, struct hash_table *ht_workers, struct list *l_workers)
{
  int port = atoi(port_s);  // Get worker's port

  if (opcode == WORKER_LSTN_FORK)  // Port belongs to a replacement worker
  {
    replace_worker_port(sock, port, buf_size, ht_workers);
    send_message(sock, RESPONSE_RECEIVED, "0", buf_size);
    return;
  }

  struct sockaddr_in sa;
  unsigned int len = sizeof(sa);
  getpeername(sock, (struct sockaddr *) &sa, &len);  // Get the worker's address from the socket

  // Compose the worker's connection info
  struct sockaddr_in *worker_addr = create_sockaddr(sa.sin_addr.s_addr, htons(port));

  pthread_mutex_lock(&list_mtx);
  list_insert_first(l_workers, worker_addr);  // Store his info along with the other workers
  pthread_mutex_unlock(&list_mtx);

  // Map every country assigned to this worker to his connection info
  upd_worker_file_reports(sock, buf_size, worker_addr, ht_workers);
  send_message(sock, RESPONSE_RECEIVED, "0", buf_size);
}

/* ========================================================================= */

// Read summary statistics from a worker over socket <sock>.
// Use the info to associate every country with the <struct sockaddr_in> of the worker assigned to it.
static void upd_worker_file_reports(int sock, int buf_size, struct sockaddr_in *work_addr, struct hash_table *ht_workers)
{
  struct message msg;
  
  while (1)
  {
    read_message(&msg, sock, buf_size);

    if (msg.opcode == END_OF_TRANSMISSION)
      break;

    char *stok_save;
    char *country = strtok_r(msg.body, "/ ", &stok_save);

    if (ht_search(ht_workers, country) == NULL)
    {
      pthread_mutex_lock(&hash_mtx);
      // Link the country with the worker's connection info
      ht_insert(ht_workers, country, work_addr);
      pthread_mutex_unlock(&hash_mtx);
    }
    
    destroy_message(&msg);
  }

  destroy_message(&msg);
}

/* ========================================================================= */

// Replace the connection port of a dead worker with the replacement worker's <port>.
// We just need to update 1 <struct sockaddr_in *> of the dead worker.
static void replace_worker_port(int sock, int port, int buf_size, struct hash_table *ht_workers)
{
  bool replaced = false;
  struct message msg;

  while (1)
  {
    read_message(&msg, sock, buf_size);

    if (msg.opcode == END_OF_TRANSMISSION)
    {
      destroy_message(&msg);
      break;
    }

    if (replaced == true)     // We ignore the following reports, as we already
    {                         // changed the info for every country
      destroy_message(&msg);  // previously assigned to the dead worker
      continue;
    }

    char *stok_save;  // <country> was previously managed by the dead worker
    char *country = strtok_r(msg.body, "/ ", &stok_save);

    // Get the address linked with the dead worker
    struct sockaddr_in *worker_addr = ht_search(ht_workers, country);
    
    pthread_mutex_lock(&waddr_mtx);
    worker_addr->sin_port = htons(port);  // Replace the port
    pthread_mutex_unlock(&waddr_mtx);

    replaced = true;  // We succefully updated the info
    destroy_message(&msg);
  }
}

/* ========================================================================= */