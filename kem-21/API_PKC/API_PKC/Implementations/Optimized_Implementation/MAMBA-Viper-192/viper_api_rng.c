#include "viper_api_rng.h"
#include "drng.h"
extern DRNG_ctx drng_algorithm;
static int rng_status;
int randombytes(unsigned char *x, unsigned long long xlen) {
  if (!x && xlen) { rng_status = -1; return -1; }
  if (get_random_number(&drng_algorithm, x, xlen * 8ULL) != 0) { rng_status = -2; return -2; }
  return 0;
}
int viper_api_rng_status(void) { return rng_status; }
void viper_api_rng_clear_status(void) { rng_status = 0; }
