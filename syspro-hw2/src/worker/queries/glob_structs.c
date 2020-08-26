
#include "header.h"
#include "patients.h"
#include "glob_structs.h"

// Default argument for the internal `hidden` patient hash table
#define DEFAULT_BUCKET_NUM (5000)

struct global_vars global;

/* ========================================================================= */

// Destroy function for the contents (avl trees) of disease and country ht.
static void destroy_avl(void *data)
{
  struct avl *tree = data;
  avl_destroy(tree);
}

 // For the contents of patient ht.
static void destroy_precords(void *data)
{
  struct patient_record *prec = data;
  destroy_patient_record(prec);
}

/* ========================================================================= */

// Allocate space for the hash tables used by the app.
void setup_structures(int dis_ht_entries, int ctry_ht_entries, int bucket_size)
{
  global.patients_ht = ht_create(DEFAULT_BUCKET_NUM / 50 + 50, 50 * HT_MIN_ACCEPTABLE_BUCKET_SIZE, destroy_precords);
  global.disease_ht = ht_create(dis_ht_entries,  bucket_size, destroy_avl);
  global.country_ht = ht_create(ctry_ht_entries, bucket_size, destroy_avl);
}


void cleanup_structures(void)
{
  ht_destroy(global.country_ht);
  ht_destroy(global.disease_ht);
  ht_destroy(global.patients_ht);
}

/* ========================================================================= */