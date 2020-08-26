
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
bool validate_args(int argc, char **argv, int *num_threads,int *query_port, int *stat_port, int *buf_size)
{
  char usage[] = "> USAGE: ./whoServer -q <queryPortNum> -s <statisticsPortNum> -w <numThreads> -b <bufferSize>\n\n";
  
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
    else if (!strcmp(argv[i], "-s"))
      params[1] = argv[++i];
    else if (!strcmp(argv[i], "-w"))
      params[2] = argv[++i];
    else if (!strcmp(argv[i], "-b"))
      params[3] = argv[++i];
    else
    {
      fprintf(stderr, "\n> Invalid command line argument option given: %s\n\n\n", argv[i]);
      return false;
    }
  }

  *num_threads = check_positive_number("numThreads", usage, params[2]);
  *buf_size    = check_positive_number("bufferSize", usage, params[3]);
  *stat_port   = check_positive_number("statisticsPortNum", usage, params[1]);
  *query_port  = check_positive_number("queryPortNum", usage, params[0]);

  if (!*num_threads || !*buf_size || !*stat_port || !*query_port)
    return false;

  return true;  // Command line args are valid
}

/* ========================================================================= */

static bool cmd_has_valid_args(int cmd, char **stok_save);


// If the command given is valid, return the <opcode> associated with it (specified in "ipc.h").
// Else return UNKNOWN_CMD.
int validate_cmd(char *line)
{
  char full_command[strlen(line)+1];
  strcpy(full_command, line);

  char *stok_save;
  char *command = strtok_r(full_command, " \n", &stok_save);
  if (command == NULL)
    return UNKNOWN_CMD;
  
  int cmd;
  if (!strcmp(command, "/searchPatientRecord"))
    cmd = SEARCH_PATIENT;
  else if (!strcmp(command, "/diseaseFrequency"))
    cmd = DISEASE_FREQ;
  else if (!strcmp(command, "/numPatientAdmissions"))
    cmd = NUM_PAT_ADM;
  else if (!strcmp(command, "/numPatientDischarges"))
    cmd = NUM_PAT_DIS;
  else if (!strcmp(command, "/topk-AgeRanges"))
    cmd = TOPK_AGE;
  else
    return UNKNOWN_CMD;

  // Check its arguments
  if (cmd_has_valid_args(cmd, &stok_save) == false)
    return UNKNOWN_CMD;

  return cmd;
}

/* ========================================================================= */

// Returns true if the command exists and has a valid # of arguments.
static bool cmd_has_valid_args(int cmd, char **stok_save)
{
  char *token = NULL;
  int argc = 0;

  switch (cmd)
  {
    case DISEASE_FREQ :
    case NUM_PAT_ADM :
    case NUM_PAT_DIS :
      while (strtok_r(NULL, " \n", stok_save) != NULL)
        ++argc;
      if (argc < 3 || argc > 4)
        return false;
      return  true;

    case SEARCH_PATIENT :
      while (strtok_r(NULL, " \n", stok_save) != NULL)
        ++argc;
      if (argc != 1)
        return false;
      return true;

    case TOPK_AGE :
      token = strtok_r(NULL, " \n", stok_save);
      if (!has_only_digits(token))
        return false;   // Check if <k> only consists of digits (is a number).
      argc = 1;
      while (strtok_r(NULL, " \n", stok_save) != NULL)
        ++argc;
      if (argc != 5)
        return false;
      return true;

    default :
      return false;
  }
}

/* ========================================================================= */