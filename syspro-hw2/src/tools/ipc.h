#ifndef COMMS_IPC_H
#define COMMS_IPC_H

/* 
 * OPERATION CODES (opcode)
 * ------------------------
 * Integers that show how to interpret the actual message transmitted.
 * Odd  numbers are sent from 'master' to 'worker'.
 * Even numbers are sent from 'worker' to 'master'.
 */

#define UNKNOWN_CMD 0
 
#define READ_DIR_CMD 1
#define FILE_REPORT 2

// Command a "replacement" child to read a dir (SIGCHLD handling)
#define READ_DIR_FORK 21
#define FILE_REPORT_FORK 22

#define SEARCH_PATIENT 3
#define SEARCH_RESULT_SUCCESS 4

#define DISEASE_FREQ 5
#define DISEASE_FREQ_RESULT 6

#define NUM_PAT_ADM 7
#define NUM_PAT_ADM_RESULT 8

#define NUM_PAT_DIS 9
#define NUM_PAT_DIS_RESULT 10

// Report after a SIGUSR1 signal was handled
#define FILE_REPORT_SIG 14

#define AVAILABILITY_CHECK 31
#define WORKER_READY 32

#define EXIT_CMD 91
#define TOPK_AGE 93
#define LIST_COUNTRIES 95
#define NONE 99


// Returns the operation code of the message.
int get_opcode(char *message);

// Returns the actual message transmitted.
char *decode_message(char *message);

// Sends <message> to file descriptor <fd> for operation <opcode>.
int send_message(int fd, int opcode, char *message, int buf_size);

// Reads a message from <fd> and returns it, allocated on the heap.
// Returns NULL on failure (signal interrupt).
char *read_message(int read_end_fd, int buf_size);


#endif