
#include "header.h"

// Perform the following queries based on the <operation> given:
// /diseaseFrequency
// /numPatientAdmissions
// /numPatientDischarges
// Stores the result(s) in list <results>.
void q_operate(int operation, struct list *results, struct list *l_workers, struct hash_table *ht_workers, int buf_size, char **stok_save);


// Ask every worker to search for a patient record.
// Return `true` if the patient was found, and store it in caller-allocated <record>.
bool q_search_patient(char *record, struct list *l_workers, int buf_size, char **stok_save);


// Answers the topk-AgeRanges query by asking a specific worker.
// Stores the result in caller-allocated <result>.
void q_find_topk(char *request, char *result, struct hash_table *ht_workers, int buf_size, char **stok_save);