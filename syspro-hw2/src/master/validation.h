#ifndef INPUT_VALID_MASTER_H
#define INPUT_VALID_MASTER_H

#include "header.h"

#define MAX_CMD_LENGTH (512)


// If the command given is valid, return the <opcode> associated with it (specified in "ipc.h").
// Else return UNKNOWN_CMD.
int validate_cmd(char *line);

// Return true if the command line arguments are valid.
bool validate_args(int argc, char **argv, int *num_workers, int *buf_size, char **input_dir_path, DIR **input_dir);


#endif