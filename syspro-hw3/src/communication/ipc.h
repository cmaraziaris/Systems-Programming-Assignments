#ifndef COMMS_IPC_H
#define COMMS_IPC_H

/* 
 * OPERATION CODES (opcode)
 * ------------------------
 * Integers that show how to interpret the actual message transmitted.
 * Odd  numbers are sent from 'server' or 'master' to 'worker'.
 * Even numbers are sent from 'worker' to 'server'.
 */

#define UNKNOWN_CMD 0

#define READ_DIR_CMD 1
#define FILE_REPORT 2

// Command a "replacement" child to read a dir (SIGCHLD handling)
#define READ_DIR_FORK 21
#define FILE_REPORT_FORK 22

#define SEARCH_PATIENT 3
#define SEARCH_RESULT_SUCCESS 4
#define SEARCH_RESULT_FAILURE 11

#define DISEASE_FREQ 5
#define DISEASE_FREQ_RESULT 6

#define NUM_PAT_ADM 7
#define NUM_PAT_ADM_RESULT 8

#define NUM_PAT_DIS 9
#define NUM_PAT_DIS_RESULT 10

#define TOPK_AGE 12
#define TOPK_AGE_RESULT 13

#define SERVER_INFO 33

#define WORKER_LSTN_PORT 38
#define WORKER_LSTN_FORK 40  // Replacement worker port

#define REQUEST_CLIENT 50
#define REQUEST_RESULT 51

// Common in every communication
#define END_OF_TRANSMISSION 66
#define RESPONSE_RECEIVED 67


struct message
{
  int opcode;  // Operation code of the message.
  char *body;  // Actual message transmitted.
};


// Sends <message> to file descriptor <fd> for operation <opcode>.
void send_message(int fd, int opcode, char *message, int buf_size);


// Reads a message from <fd> and fills the struct message <received>.
// Returns 1 on failure, 0 on success.
// Reasons to fail are either ECONNRESET or EINTR.
int read_message(struct message *received, int fd, int buf_size);


// Destroys the struct message <received> filled by <read_message>.
void destroy_message(struct message *received);


#endif