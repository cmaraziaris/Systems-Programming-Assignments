
#include <ctype.h>

#include "header.h"
#include "validation.h"

/* ========================================================================= */

// Return true if a string contains *only* digits.
static bool has_only_digits(char *str)
{
  for (int i = 0; str[i]; ++i)
    if (!isdigit(str[i]))
      return false;
  return true;
}

// Return <num> if number is positive, else 0.
static int check_positive_number(char *var_name, char *usage, char *value)
{
  int num;
  if (!has_only_digits(value) || ((num = atoi(value)) && num <= 0))
  {
    fprintf(stderr, "\n[ERROR] <%s> given is *not* a positive number.\n\n%s", var_name, usage);
    return 0;
  }
  return num;
}

/* ========================================================================= */

// Return true if the command line arguments are valid.
bool validate_args(int argc, char **argv, int *num_workers, int *buf_size, char **input_dir_path, DIR **input_dir, char **serverIP, char **port)
{
  char usage[] = "> USAGE: ./master -w <numWorkers> -b <bufferSize> -s <serverIP> -p <serverPort> -i <input_dir>\n\n";

  if (argc != 11)
  {
    fprintf(stderr, "%s\n%s", "\n[ERROR] Please give *exactly* 11 arguments.", usage);
    return false;
  }

  char *params[5];

  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-w")) 
      params[0] = argv[++i];
    else if (!strcmp(argv[i], "-b"))
      params[1] = argv[++i];
    else if (!strcmp(argv[i], "-i"))
      params[2] = argv[++i];
    else if (!strcmp(argv[i], "-s"))
      params[3] = argv[++i];
    else if (!strcmp(argv[i], "-p"))
      params[4] = argv[++i];
    else
    {
      fprintf(stderr, "\n> Invalid command line argument option given: %s\n\n\n", argv[i]);
      return false;
    }
  }

  *serverIP = params[3];
  *port = params[4];

  *num_workers = check_positive_number("numWorkers", usage, params[0]);
  *buf_size    = check_positive_number("bufferSize", usage, params[1]);
  int port_num = check_positive_number("serverPort", usage, params[4]);

  if (!*num_workers || !*buf_size || !port_num)
    return false;

  *input_dir_path = params[2];
  if ((*input_dir = opendir(*input_dir_path)) == NULL)
  {
    fprintf(stderr, "%s\n%s", "[ERROR] <input_dir> given not found.", usage);
    return false;
  }

  return true;  // Command line args are valid
}

/* ========================================================================= */