#ifndef SIG_HANDLING_MSTR_H
#define SIG_HANDLING_MSTR_H

#include <stdbool.h>

#include "setup_workers.h"

#define SETUP true
#define OPERATE false

#define quit_gracefully() actions_quit(false, NULL, NULL, NULL, NULL, NULL)


// If SIGUSR2 is received, increment available reports to be read by `master`.
void actions_usr2(bool at_setup, int *pavailable_upd);


// Quit gracefully. Outputs log, kill `workers`, wait for them to terminate and cleanup memory and open files.
void actions_quit(bool at_setup, struct worker_stats *pw_stats, int *pnum_workers, struct hash_table *pht_worker, int *psucc, int *pfail);


// Replace a child that terminated unexpectedly.
void actions_child_term(bool at_setup, struct hash_table *pht_workers, struct worker_stats *pw_stats, int *pnum_workers, int *pbuf_size, char *pinput_dir, int *pready_workers);


// Cleanup memory and files/fifos/directories used by the app.
void actions_cleanup(bool at_setup, int *pnum_workers, struct worker_stats *pw_stats, struct hash_table *pht_workers, struct hash_table *pht_ranges, DIR *pinput_dir);


#endif