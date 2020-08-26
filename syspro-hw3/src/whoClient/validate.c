
#include <ctype.h>

#include "header.h"
#include "validate.h"

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
bool validate_args(int argc, char **argv, int *num_threads, FILE **query_file, char **serverIP, char **port)
{
  char usage[] = "> USAGE: ./whoClient -q <queryFile> -w <numThreads> -sp <servPort> -sip <servIP>\n\n";

  if (argc != 9)
  {
    fprintf(stderr, "%s\n%s", "\n[ERROR] Please give *exactly* 9 arguments.", usage);
    return false;
  }

  char *params[4];

  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-q")) 
      params[0] = argv[++i];
    else if (!strcmp(argv[i], "-w"))
      params[1] = argv[++i];
    else if (!strcmp(argv[i], "-sp"))
      params[2] = argv[++i];
    else if (!strcmp(argv[i], "-sip"))
      params[3] = argv[++i];
    else
    {
      fprintf(stderr, "\n> Invalid command line argument option given: %s\n\n\n", argv[i]);
      return false;
    }
  }

  *port = params[2];
  *serverIP = params[3];

  *num_threads = check_positive_number("numThreads", usage, params[1]);
  int pos_port = check_positive_number("servPort",   usage, params[2]);

  if (!*num_threads || !pos_port)
    return false;

  if ((*query_file = fopen(params[0], "r")) == NULL)
  {
    fprintf(stderr, "%s\n%s", "[ERROR] <queryFile> given not found.", usage);
    return false;
  }

  return true;  // Command line args are valid
}

/* ========================================================================= */