// Common headers for master and worker

// standard libs
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// modules
#include "avl.h"
#include "list.h"
#include "hash_table.h"

// tools
#include "ipc.h"
#include "date.h"
#include "fifo_dir.h"
