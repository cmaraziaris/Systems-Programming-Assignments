
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "date.h"
#include "stats.h"
#include "patients.h"
#include "utilities.h"
#include "interface.h"

/* ========================================================================= */

enum function_t
{
  GLOB_DIS_STATS, DIS_FREQ, TOP_DIS, TOP_CTR, INS_PAT_REC,
  REC_PAT_EXT, NUM_CURR_PAT, EXT
};

static bool is_valid(enum function_t func, char **args);
static bool has_valid_args(enum function_t func, char **args);
static bool is_valid_range(char *sdate1, char *sdate2);

static bool is_valid_range(char *sdate1, char *sdate2)
{
  if (sdate1 == NULL && sdate2 == NULL)
    return true;

  struct date d1, d2;
  convert_str_to_date(sdate1, &d1, DUMMY_BEGIN);
  convert_str_to_date(sdate2, &d2, DUMMY_END);
  if (compare_dates(&d1, &d2) > 0)
    return false;

  return true;
}

static bool is_valid(enum function_t func, char **args)
{
  if (has_valid_args(func, args))
    return true;

  printf("error\n");
  return false;
}

static bool has_valid_args(enum function_t func, char **args)
{
  if (strtok(NULL, " \n"))  // More arguments given.
    return false;

  switch (func)
  {
    case GLOB_DIS_STATS:
      if ((args[0] != NULL && args[1] == NULL) || !is_valid_range(args[0], args[1]))
        return false;
      return true;

    case DIS_FREQ:
      if (!args[0] || !args[1] || !args[2] || isalpha(args[1][0]) || isalpha(args[2][0]))
        return false;
      return true;

    case TOP_DIS:
    case TOP_CTR:
      if ((args[2] != NULL && args[3] == NULL) || !is_valid_range(args[2], args[3]))
        return false;
      return true;

    case INS_PAT_REC:
      if (!args[0] || !args[1] || !args[2] || !args[3] || !args[4] || !args[5])
        return false;

      if (args[6] && !is_valid_range(args[5], args[6]))
        return false;

      return true;

    case REC_PAT_EXT:
      if (!args[0] || !args[1])
        return false;
      return true;

    case NUM_CURR_PAT:
    case EXT:
    default:
      return true;
  }

  return true;
}


/* ========================================================================= */

void interface(void)
{
  char *line = NULL;
  size_t len = 0;

  while (true)
  {
    if (getline(&line, &len, stdin) == -1)
      break;

    char *command = strtok(line, " \n");
    
    if (!command)
      continue;

    if (!strcmp(command, "/globalDiseaseStats"))
    {
      char *args[2];
      for (int i = 0; i < 2; ++i)
        args[i] = strtok(NULL, " \n");

      if (is_valid(GLOB_DIS_STATS, args))
        global_disease_stats(args[0], args[1]);
    }

    else if (!strcmp(command, "/diseaseFrequency"))
    {
      char *args[4];
      for (int i = 0; i < 4; ++i)
        args[i] = strtok(NULL, " \n");

      if (is_valid(DIS_FREQ, args))
        disease_frequency(args[0], args[1], args[2], args[3]);
    }

    else if (!strcmp(command, "/topk-Diseases"))
    {
      char *args[4];
      for (int i = 0; i < 4; ++i)
        args[i] = strtok(NULL, " \n");
      
      if (is_valid(TOP_DIS, args))
        topk_diseases(atoi(args[0]), args[1], args[2], args[3]);
    }

    else if (!strcmp(command, "/topk-Countries"))
    {
      char *args[4];
      for (int i = 0; i < 4; ++i)
        args[i] = strtok(NULL, " \n");

      if (is_valid(TOP_CTR, args))
        topk_countries(atoi(args[0]), args[1], args[2], args[3]);      
    }

    else if (!strcmp(command, "/insertPatientRecord"))
    {
      char *args[7];
      for (int i = 0; i < 7; ++i)
        args[i] = strtok(NULL, " \n");

      if (is_valid(INS_PAT_REC, args))
      {
        if (!insert_patient_record(args[0], args[1], args[2], args[3], args[4], args[5], args[6]))
        {
          printf("error\nexiting\n");
          break;  // Terminate the app if the insertion failed.
        }
        else
          printf("Record added\n");
      }
    }

    else if (!strcmp(command, "/recordPatientExit"))
    {
      char *args[2];
      for (int i = 0; i < 2; ++i)
        args[i] = strtok(NULL, " \n");

      if (is_valid(REC_PAT_EXT, args))
        record_patient_exit(args[0], args[1]);
    }

    else if (!strcmp(command, "/numCurrentPatients"))
    {
      char *arg = strtok(NULL, " \n");

      if (is_valid(NUM_CURR_PAT, NULL))
        num_current_patients(arg);
    }

    else if (!strcmp(command, "/exit"))
    {
      if (is_valid(EXT, NULL))
      {
        printf("exiting\n");
        break;
      }
    }

    else
      printf("error\n");
  }

  free(line);
}
/* ========================================================================= */