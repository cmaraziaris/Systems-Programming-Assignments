
#include <stdbool.h>

// Return `true` if the command line arguments are valid.
bool validate_args(int argc, char **argv, int *num_threads,int *query_port, int *stat_port, int *buf_size);


// If the request given from client is valid, return the <opcode> associated with it (specified in "ipc.h").
// Else return UNKNOWN_CMD.
int validate_cmd(char *request);