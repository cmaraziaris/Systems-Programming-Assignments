
#include "validate.h"
#include "queries.h"
#include "client_requests.h"

// Globals needed by every function (so that we don't pass these args every time)
static struct hash_table *ht_workers;
static struct list *l_workers;
static int buf_size;

static void unknown_cmd(int sock, char *request);
static void search_patient(char *msg, int sock, char *request);
static void topk_age_ranges(char *msg, int sock, char *request);
static void disease_frequency(char *msg, int sock, char *request);
static void num_patients(int cmd, char *msg, int sock, char *request);

/* =========================================================== */

// Process a client's message/request and send back the result. Also output the result to stdout.
void process_client_request(char *msg, int sock, int pbuf_size, struct hash_table *pht_workers, struct list *pl_workers)
{
  int len = strlen(msg);
  char *request = malloc((len + 1) * sizeof(char));
  strncpy(request, msg, len+1);

  // Trim trailing '\n' for better output
  if (request[len-1] == '\n')
    request[len-1] = '\0';

  ht_workers = pht_workers;  // Initialize globals
  l_workers  = pl_workers;
  buf_size   = pbuf_size;

  int cmd = validate_cmd(msg);
  
  if (cmd == UNKNOWN_CMD) {
    unknown_cmd(sock, request);
  }
  else if (cmd == SEARCH_PATIENT) {
    search_patient(msg, sock, request);
  }
  else if (cmd == TOPK_AGE) {
    topk_age_ranges(msg, sock, request);
  }
  else if (cmd == DISEASE_FREQ) {
    disease_frequency(msg, sock, request);
  }
  else
    num_patients(cmd, msg, sock, request);

  read_message(NULL, sock, buf_size);  // Read EOT
  send_message(sock, END_OF_TRANSMISSION, "0", buf_size);  // Send "ACK"

  free(request);
}

/* =========================================================== */

static void unknown_cmd(int sock, char *request)
{
  send_message(sock, REQUEST_RESULT, "Unknown command.", buf_size);
  send_message(sock, END_OF_TRANSMISSION, "0", buf_size);
  
  printf("\n%s\nUnknown command.\n", request);
}

/* =========================================================== */

static void search_patient(char *msg, int sock, char *request)
{
  char *stok_save;
  strtok_r(msg, " \n", &stok_save);  // Initialize strtok_r

  char *result = calloc(256, sizeof(char));
  strcpy(result, "Patient not found.");
    
  q_search_patient(result, l_workers, buf_size, &stok_save);

  send_message(sock, REQUEST_RESULT, result, buf_size);
  send_message(sock, END_OF_TRANSMISSION, "0", buf_size);
    
  printf("\n%s\n%s\n", request, result);
  free(result);
}

/* =========================================================== */

static void topk_age_ranges(char *msg, int sock, char *request)
{
  char *stok_save;
  strtok_r(msg, " \n", &stok_save);  // Initialize strtok_r

  char *result = calloc(256, sizeof(char));
  q_find_topk(request, result, ht_workers, buf_size, &stok_save);

  send_message(sock, REQUEST_RESULT, result, buf_size);
  send_message(sock, END_OF_TRANSMISSION, "0", buf_size);
    
  printf("\n%s\n%s\n", request, result);
  free(result);
}

/* =========================================================== */

// /numPatientAdmissions
// /numPatientDischarges
static void num_patients(int cmd, char *msg, int sock, char *request)
{
  char *stok_save;
  strtok_r(msg, " \n", &stok_save);  // Initialize strtok_r  

  struct list *results = list_create(free);
  q_operate(cmd, results, l_workers, ht_workers, buf_size, &stok_save);  

  int size = list_size(results);

  if (cmd == NUM_PAT_ADM)
  {
    int total = 0;
    for (int i = 1; i <= size; ++i)  // Sum up the results
      total += atoi(list_get(results, i));

    char buf[16];
    sprintf(buf, "%d", total);
    send_message(sock, REQUEST_RESULT, buf, buf_size);
    send_message(sock, END_OF_TRANSMISSION, "0", buf_size);
    
    printf("\n%s\n%d\n", request, total);
  }
  else
  {
    // Lock stdout as we're going to do multiple I/O ops
    flockfile(stdout);
    printf("\n%s\n", request);

    for (int i = 1; i <= size; ++i)  // Print and send results
    {
      char *res = list_get(results, i);
      printf("%s\n", res);
      send_message(sock, REQUEST_RESULT, res, buf_size);
    }

    funlockfile(stdout);
    send_message(sock, END_OF_TRANSMISSION, "0", buf_size);
  }

  list_destroy(results);
}

/* =========================================================== */

static void disease_frequency(char *msg, int sock, char *request)
{
  char *stok_save;
  strtok_r(msg, " \n", &stok_save);  // Initialize strtok_r  

  struct list *results = list_create(free);
  q_operate(DISEASE_FREQ, results, l_workers, ht_workers, buf_size, &stok_save);

  int total = 0, size = list_size(results);

  for (int i = 1; i <= size; ++i)  // Gather the total summary
    total += atoi(list_get(results, i));

  char res[16];
  sprintf(res, "%d", total);
  send_message(sock, REQUEST_RESULT, res, buf_size);
  send_message(sock, END_OF_TRANSMISSION, "0", buf_size);
  list_destroy(results);

  printf("\n%s\n%d\n", request, total);
}

/* =========================================================== */