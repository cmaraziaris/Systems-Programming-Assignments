
#include "header.h"
#include "queries.h"

/* ========================================================================= */

// Perform the following queries based on the <operation> given:
// /diseaseFrequency
// /numPatientAdmissions
// /numPatientDischarges
void q_operate(int operation, struct list *results, struct list *l_workers, struct hash_table *ht_workers, int buf_size, char **stok_save)
{
  char *disease = strtok_r(NULL, " \n", stok_save);  // Get command arguments
  char *entr_dt = strtok_r(NULL, " \n", stok_save);
  char *exit_dt = strtok_r(NULL, " \n", stok_save);
  char *country = strtok_r(NULL, " \n", stok_save);  // If NULL, operate on *every* country

  char buf[128];  // Compose the message containing arguments
  snprintf(buf, 128, "%s:%s:%s:%s", disease, entr_dt, exit_dt, country == NULL ? " " : country);

  if (country != NULL)  // Send a message to the worker of this country
  {
    struct sockaddr_in *wa = ht_search(ht_workers, country);  // Find worker
    if (wa == NULL)
    {
      list_insert_first(results, strdup("Country not found."));
      return;
    }

    int sock = connect_to_server(wa);
    send_message(sock, operation, buf, buf_size);  // Ask worker

    while (1)  // Get answer
    {
      struct message msg;
      read_message(&msg, sock, buf_size);

      if (msg.opcode == END_OF_TRANSMISSION)
      {
        destroy_message(&msg);
        break;
      }

      list_insert_first(results, strdup(msg.body));
      destroy_message(&msg);
    }

    send_message(sock, RESPONSE_RECEIVED, "0", buf_size);
    read_message(NULL, sock, buf_size);
    close_w(sock);
  }
  else  // Send a message to every worker
  {
    int l_size = list_size(l_workers);
    for (int i = 1; i <= l_size; ++i)
    {
      struct sockaddr_in *wa = list_get(l_workers, i);

      int sock = connect_to_server(wa);
      send_message(sock, operation, buf, buf_size);
      
      while (1)  // Read answers
      {
        struct message msg;
        read_message(&msg, sock, buf_size);

        if (msg.opcode == END_OF_TRANSMISSION)
        {
          destroy_message(&msg);
          break;
        }

        list_insert_first(results, strdup(msg.body));
        destroy_message(&msg);
      }

      send_message(sock, RESPONSE_RECEIVED, "0", buf_size);
      read_message(NULL, sock, buf_size);
      close_w(sock);
    }
  }
}

/* ========================================================================= */

// Ask every worker to search for a patient record.
// If found, return the record in <record> which must be allocated by the caller.
bool q_search_patient(char *record, struct list *l_workers, int buf_size, char **stok_save)
{
  bool found = false;
  char *rec_id = strtok_r(NULL, " \n", stok_save);
  
  int l_size = list_size(l_workers);
  for (int i = 1; i <= l_size && !found; ++i)  // Loop for every worker
  {
    struct sockaddr_in *wa = list_get(l_workers, i);  // Get the worker's address
    int sock = connect_to_server(wa);

    send_message(sock, SEARCH_PATIENT, rec_id, buf_size);  // Ask worker
    
    struct message msg;
    read_message(&msg, sock, buf_size);  // Get answer
    send_message(sock, RESPONSE_RECEIVED, "0", buf_size);

    if (msg.opcode == SEARCH_RESULT_SUCCESS)
    {
      strncpy(record, msg.body, 256);
      found = true;  // No need to check other workers
    }

    destroy_message(&msg);
    read_message(NULL, sock, buf_size);
    close_w(sock);  // Terminate connection
  }

  return found;
}

/* ========================================================================= */

// Answers the topk-AgeRanges query by asking a specific worker.
// Stores the result in caller-allocated <result>.
void q_find_topk(char *request, char *result, struct hash_table *ht_workers, int buf_size, char **stok_save)
{
  strtok_r(NULL, " \n", stok_save);
  char *country = strtok_r(NULL, " \n", stok_save);

  struct sockaddr_in *wa = ht_search(ht_workers, country);  // Find worker
  if (wa == NULL)
  {
    strcpy(result, "No recorded cases in this country.");
    return;
  }

  int sock = connect_to_server(wa);
  send_message(sock, TOPK_AGE, request, buf_size);  // Ask worker

  struct message msg;
  read_message(&msg, sock, buf_size);  // Get answer
  send_message(sock, RESPONSE_RECEIVED, "0", buf_size);

  strncpy(result, msg.body, 256);
  destroy_message(&msg);

  read_message(NULL, sock, buf_size);
  close_w(sock);  // Terminate connection
}

/* ========================================================================= */