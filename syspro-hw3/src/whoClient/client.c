
#include <pthread.h>

#include "header.h"
#include "validate.h"
#include "operate.h"

extern pthread_barrier_t barrier;    // Barrier to synchronize threads
extern struct sockaddr_in *sv_addr;  // Server address to connect


int main(int argc, char *argv[])
{
  int num_threads;
  char *server_ip, *port_s;  // Command line args
  FILE *query_file;

  if (!validate_args(argc, argv, &num_threads, &query_file, &server_ip, &port_s))
    exit(1);   // Validate cmd line args and initialize values

  sv_addr = prepare_server_connection(server_ip, port_s);

  pthread_t threads[num_threads];

  char *query = NULL;  // getline() args
  size_t len = 0;

  bool reached_eof = false;

  while (!reached_eof)  // While there are queries left to read
  {
    int alive_threads = 0;

    if (pthread_barrier_init(&barrier, NULL, num_threads))  // Initialize the barrier
      error_exit("pthread_barrier_init");

    do
    {
      reached_eof = (getline(&query, &len, query_file) == -1);
      
      if (reached_eof)
        // Create "dummy" threads (NULL query) if we already read the file
        pthread_create(&threads[alive_threads], NULL, make_request, NULL);
      else 
        // Create a thread and assign the query
        pthread_create(&threads[alive_threads], NULL, make_request, strdup(query));
    
    } while (++alive_threads != num_threads);  // Spawn exactly <num_threads> threads
    
    for (int i = 0; i < num_threads; ++i)  // Wait for every thread to finish
      pthread_join(threads[i], NULL);

    if (pthread_barrier_destroy(&barrier)) error_exit("pthread_barrier_destroy");
  }

  destroy_sockaddr(sv_addr);        // Cleanup
  if (errno) error_exit("getline");
  if (fclose(query_file) == EOF) error_exit("fclose");
  if (query) free(query);

  exit(0);
}
