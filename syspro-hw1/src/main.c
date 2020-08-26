
#include <stdbool.h>
#include <stdio.h>

#include "utilities.h"
#include "interface.h"


int main(int argc, const char *argv[])
{
  FILE *fp;
  int dis_ht_entries, ctry_ht_entries, bucket_size;

  handle_cmd_line_args(argc, argv, &fp, &dis_ht_entries, &ctry_ht_entries, &bucket_size);

  setup_structures(dis_ht_entries, ctry_ht_entries, bucket_size);

  if (parse_file(fp) == true)
    interface();     // If the file given doesn't have duplicate patient records, proceed.

  cleanup_structures();

  return 0;
}