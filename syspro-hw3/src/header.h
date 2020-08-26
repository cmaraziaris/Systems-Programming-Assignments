// Common headers for master/worker/whoServer/whoClient
#ifndef COMMON_HEADER_H
#define COMMON_HEADER_H


// standard libs
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// modules
#include "avl.h"
#include "list.h"
#include "hash_table.h"

// comms
#include "ipc.h"
#include "network.h"


// error-related global functions
static inline void error_exit(char *msg)
{
  perror(msg);
  exit(1);
}

static inline void close_w(int fd)
{
  if (close(fd) == -1)
    error_exit("close");
}



#endif