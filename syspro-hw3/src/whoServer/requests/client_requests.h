
#include "header.h"


// Process a client's <request> from socket <sock> and send back the result. Also, output the result to stdout.
void process_client_request(char *request, int sock, int buf_size, struct hash_table *ht_workers, struct list *l_workers);
