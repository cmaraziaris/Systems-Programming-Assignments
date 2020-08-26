
#include <stdbool.h>

// If <setup> is true, create a directory to store fifos.
// Else, create a fifo and return its paths in <path>.
// Note: <path> must be allocated by the caller.
void create_unique_fifo(bool setup, char *path);


// Remove a flat directory and its contents.
void delete_flat_dir(char *init_flat_path);