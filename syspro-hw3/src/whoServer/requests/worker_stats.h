
#include "header.h"


// Decide how to handle worker statistics.
// Either insert a new worker's info into the app structures, or replace a dead one's info with a replacement's.
void process_worker_stats(int opcode, char *dec_msg, int sock, int buf_size, struct hash_table *ht_workers, struct list *l_workers);