
#include <stdio.h>  // FILE

// Parse a file <fp> with patients line by line, and insert every patient to the database.
// Return a string (report) with patient stats. Update valid/invalid records counters.

char *parse_file(FILE *fp, char *country, char *date, int *successful, int *failed);