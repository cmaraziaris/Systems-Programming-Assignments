
#include <stdio.h>  // FILE
#include <stdbool.h>

// Returns `false` if the file has duplicate records.
bool parse_file(FILE *fp);

void handle_cmd_line_args(int argc, const char **argv, FILE **fp, int *dis, int *ctry, int *b_size);

void setup_structures(int dis_ht_entries, int ctry_ht_entries, int bucket_size);

void cleanup_structures(void);