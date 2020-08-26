#ifndef QUERIES_MASTER_H
#define QUERIES_MASTER_H

#include "setup_workers.h"

#define JUST_PRINT true
#define UPDATE_DATA false


// For every country, print the PID of the worker assigned to its dir.
void q_list_countries(struct hash_table *ht_workers);


// Answers the topk-AgeRanges query.
// If country or disease don't exist, does nothing.
// Else, prints to stdout.
void q_find_topk(struct hash_table *ht_countries, char **stok_save);


// Print the message received in "<Age range> - <cases>" format.
// If <just_print> is FALSE, also add the contents of the report to the database.
void q_add_report(bool print_only, struct hash_table *ht_countries, char *msg);


// Perform the following queries based on the <opcode> given:
// /diseaseFrequency
// /numPatientAdmissions
// /numPatientDischarges
void q_operate(int opcode, int num_workers, struct worker_stats *w_stats, struct hash_table *ht_workers, int buf_size, char **stok_save);


// Ask every worker to search for a patient record.
void q_search_patient(struct worker_stats *w_stats, int num_workers, int buf_size, char **stok_save);


#endif