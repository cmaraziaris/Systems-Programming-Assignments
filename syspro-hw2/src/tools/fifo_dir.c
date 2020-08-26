
#include "header.h"
#include "fifo_dir.h"

/* ========================================================================= */

// If <setup> is true, create a directory to store fifos.
// Else, create a fifo and return its end paths in <read_p> <writ_p>.
// Note: <read_p> <writ_p> must be allocated by the caller.
void create_unique_fifo(bool setup, char *read_p, char *writ_p)
{
  static char fifo_name[32];
  static int counter = 0;
  
  if (setup == true)
  {
    char dir_path[] = "named_fifos";
    if (access(dir_path, F_OK) == 0)  // If dir already exists (due to abnormal previous termination, eg: SIGKILL)
      delete_flat_dir(dir_path);      // completely remove it

    if (mkdir(dir_path, 0777) == -1){perror("mkdir @ unique_fifo");exit(1);}
    sprintf(fifo_name, "named_fifos/f");
    return;
  }

  // Create a unique name
  snprintf(read_p, 32, "%s%d%c", fifo_name, counter, 'R');
  snprintf(writ_p, 32, "%s%d%c", fifo_name, counter, 'W');
  ++counter;

  // Create fifos
  if (mkfifo(read_p, 0700) == -1){perror("mkfifo @ unique_fifo");exit(1);}
  if (mkfifo(writ_p, 0700) == -1){perror("mkfifo @ unique_fifo");exit(1);}
}

/* ========================================================================= */

// Remove a flat directory and its contents.
void delete_flat_dir(char *init_flat_path)
{
  char flat_path[32];
  strcpy(flat_path, init_flat_path);

  DIR *dir = opendir(flat_path);
  if (dir == NULL){perror("opendir @ delete_flat_dir"); exit(1);}

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)  // Delete contents/files
  {
    char *f_name = entry->d_name;
    if (!strcmp(f_name, ".") || !strcmp(f_name, ".."))
      continue;

    char f_path[32];
    snprintf(f_path, 32, "%s/%s", flat_path, f_name);  // Remove file
    if (remove(f_path) == -1){perror("remove @ delete_flat_dir"); exit(1);}
  }

  // Remove dir
  if (closedir(dir) == -1){perror("closedir 2 @ delete_flat_dir"); exit(1);}
  if (rmdir(flat_path) == -1){perror("rmdir @ delete_flat_dir"); exit(1);}  
}

/* ========================================================================= */