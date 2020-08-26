
#include "hash_table.h"
#include "binary_heap.h"
#include "avl.h"
#include "patients.h"

int get_diseased_range(struct avl *tree, 
                       char *field, 
                       char *sdate1, 
                       char *sdate2, 
                       char *(*get_field)(struct patient_record *));

void set_bh_range(struct binary_heap *bh, 
                  struct hash_table *info,
                  char *sdate1, 
                  char *sdate2, 
                  char *field, 
                  char *(*get_field)(struct patient_record *));

void set_bh_no_range(struct binary_heap *bh, 
                     struct hash_table *info, 
                     char *field, 
                     char *(*get_field)(struct patient_record *));