/*
 * PRNG and interface to the deterministic RNG.
 */

#include <assert.h>

#include "inner.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

/* see inner.h */
int
ctl_get_seed(void *seed, size_t len)
{
	if (len == 0) {
		return 1;
	}
	return get_random_number(&drng_algorithm, seed,
		(unsigned long long)len * 8) == 0;
}
