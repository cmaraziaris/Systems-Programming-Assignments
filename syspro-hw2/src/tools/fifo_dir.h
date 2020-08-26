

// If <setup> is true, create a directory to store fifos.
// Else, create a fifo and return its end paths in <read_p> <writ_p>.
// Note: <read_p> <writ_p> must be allocated by the caller.
void create_unique_fifo(bool setup, char *read_p, char *writ_p);


// Remove a flat directory and its contents.
void delete_flat_dir(char *init_flat_path);