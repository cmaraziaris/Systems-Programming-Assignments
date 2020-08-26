
#include <pthread.h>

struct cbuf;

// Create and return an empty circular buffer of capacity <size>.
struct cbuf *cb_create(int size);


// Insert <fd> in buffer <cb>.
void cb_insert(struct cbuf *cb, int fd);


// Remove and return an item from buffer <cb>.
int cb_remove(struct cbuf *cb);


// Destroy a circular buf returned by <cb_create>.
void cb_destroy(struct cbuf *cb);


// Initialize condition variables.
void cb_cond_init(void);


// Destroy condition variables. Also unblock threads blocked on them.
void cb_cond_destroy(int num_threads, struct cbuf *cb);


// Global condition vars as we only use 1 buffer in the app.
extern pthread_cond_t cb_cond_non_empty;
extern pthread_cond_t cb_cond_non_full;
