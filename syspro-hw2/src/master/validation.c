
#include <ctype.h>

#include "header.h"
#include "validation.h"

static bool cmd_has_valid_args(int cmd, char **stok_save);

/* ========================================================================= */

// Return true if a string contains *only* digits.
static bool has_only_digits(char *str)
{
  for (int i = 0; str[i]; ++i)
    if (!isdigit(str[i]))
      return false;
  return true;
}

/* ========================================================================= */

// If the command given is valid, return the <opcode> associated with it (specified in "ipc.h").
// Else return UNKNOWN_CMD.
int validate_cmd(char *line)
{
  char full_command[strlen(line)+1];
  strcpy(full_command, line);

  char *stok_save;
  char*command = strtok_r(full_command, " \n", &stok_save);
  if (command == NULL)
    return UNKNOWN_CMD;
  
  int cmd;
  if (!strcmp(command, "/listCountries"))  // Find the command
    cmd = LIST_COUNTRIES;
  else if (!strcmp(command, "/searchPatientRecord"))
    cmd = SEARCH_PATIENT;
  else if (!strcmp(command, "/diseaseFrequency"))
    cmd = DISEASE_FREQ;
  else if (!strcmp(command, "/numPatientAdmissions"))
    cmd = NUM_PAT_ADM;
  else if (!strcmp(command, "/numPatientDischarges"))
    cmd = NUM_PAT_DIS;
  else if (!strcmp(command, "/topk-AgeRanges"))
    cmd = TOPK_AGE;
  else if (!strcmp(command, "/exit"))
    cmd = EXIT_CMD;
  else
    cmd = NONE;

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

    case LIST_COUNTRIES :
    case EXIT_CMD :
      if (strtok_r(NULL, " \n", stok_save) != NULL)
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

// Return true if the command line arguments are valid.
bool validate_args(int argc, char **argv, int *num_workers, int *buf_size, char **input_dir_path, DIR **input_dir)
{
  char usage[] = "> USAGE: ./diseaseAggregator -w <numWorkers> -b <bufferSize> -i <input_dir>\n\n";

  if (argc != 7)
  {
    fprintf(stderr, "%s\n%s", "\n[ERROR] Please give *exactly* 7 arguments.", usage);
    return false;
  }

  char *params[3];

  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-w")) 
      params[0] = argv[++i];
    else if (!strcmp(argv[i], "-b"))
      params[1] = argv[++i];
    else if (!strcmp(argv[i], "-i"))
      params[2] = argv[++i];
    else
    {
      fprintf(stderr, "\n> Invalid command line argument option given: %s\n\n\n", argv[i]);
      return false;
    }
  }

  if (has_only_digits(params[0]))
  {
    *num_workers = atoi(params[0]);
    if (*num_workers <= 0)
    {
      fprintf(stderr, "%s\n%s", "\n[ERROR] Please give a *positive* <numWorkers>.", usage);
      return false;
    }
  }
  else
  {
    fprintf(stderr, "%s\n%s", "\n[ERROR] <numWorkers> given is *not* a positive number.", usage);
    return false;
  }

  if (has_only_digits(params[1]))
  {
    *buf_size = atoi(params[1]);
    if (*buf_size <= 0)
    {
      fprintf(stderr, "%s\n%s", "\n[ERROR] Please give a *positive* <bufferSize>.", usage);
      return false;
    }
  }
  else
  {
    fprintf(stderr, "%s\n%s", "\n[ERROR] <bufferSize> given is *not* a positivenumber.", usage);
    return false;
  }

  *input_dir_path = params[2];
  *input_dir = opendir(*input_dir_path);
  if (*input_dir == NULL)
  {
    fprintf(stderr, "%s\n%s", "[ERROR] <input_dir> given not found.", usage);
    return false;
  }

  return true;  // Command line args are valid
}

/* ========================================================================= */