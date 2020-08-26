/* 
 * Disclaimer:
 * this code is based on the material taught by the instructors in class (producer-consumer)
 * uploaded here: http://cgi.di.uoa.gr/~mema/courses/k24/lectures/topic6-Threads.pdf (pg.76)
 */

#include <stdlib.h>

#include "circular_buf.h"

pthread_cond_t cb_cond_non_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cb_cond_non_full  = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/* =========================================================== */

// Circular buffer
struct cbuf
{
  int *data;  // Array of file descriptors
  int size;
  int start;
  int end;
  int count;  // Current amount of data
};

/* =========================================================== */

// Create and return an empty circular buffer of capacity <size>.
struct cbuf *cb_create(int size)
{
  struct cbuf *cb = malloc(sizeof(struct cbuf));
  cb->data = calloc(size, sizeof(int));
  cb->size = size;
  cb->start = cb->count = 0;
  cb->end = -1;
  return cb;
}


// Destroy buffer <cb> and its associated memory.
void cb_destroy(struct cbuf *cb) {
  free(cb->data);
  free(cb);
  pthread_mutex_destroy(&mtx);
}

/* =========================================================== */

// Insert <fd> in buffer <cb>.
void cb_insert(struct cbuf *cb, int fd)
{
  pthread_mutex_lock(&mtx);
  
  // Wait until there is an empty spot
  while (cb->count >= cb->size)
    pthread_cond_wait(&cb_cond_non_full, &mtx);

  cb->end = (cb->end + 1) % cb->size;
  cb->data[cb->end] = fd;
  ++cb->count;

  pthread_mutex_unlock(&mtx);  
}

/* =========================================================== */

// Remove and return an item from buffer <cb>.
int cb_remove(struct cbuf *cb)
{
  int fd = 0;
  pthread_mutex_lock(&mtx);

  // Wait until there is an item to remove
  while (cb->count <= 0)
    pthread_cond_wait(&cb_cond_non_empty, &mtx);

  fd = cb->data[cb->start];
  cb->start = (cb->start + 1) % cb->size;
  --cb->count;

  pthread_mutex_unlock(&mtx);
  return fd;
}

/* =========================================================== */

// Initialize condition variables.
void cb_cond_init(void)
{
  pthread_cond_init(&cb_cond_non_empty, NULL);
  pthread_cond_init(&cb_cond_non_full, NULL);
  pthread_mutex_init(&mtx, NULL);
}


// Destroy condition variables.
void cb_cond_destroy(int num_threads, struct cbuf *cb)
{
  // Unblock every blocked thread so they can terminate
  for (int i = 0; i < num_threads; ++i)
  {
    cb_insert(cb, -1);
    pthread_cond_signal(&cb_cond_non_empty);
  }
  
  pthread_cond_destroy(&cb_cond_non_full);
  pthread_cond_destroy(&cb_cond_non_empty); 
}

/* =========================================================== */