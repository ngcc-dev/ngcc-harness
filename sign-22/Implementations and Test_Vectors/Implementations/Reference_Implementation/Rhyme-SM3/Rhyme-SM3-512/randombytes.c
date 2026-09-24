/* DRNG-backed randombytes for deterministic KAT generation.
 * Replaces the system /dev/urandom source.  The DRNG must be
 * initialized (via init_random_number) before any call that
 * consumes random bytes, which the KAT driver does before each
 * key generation. */
#include <stddef.h>
#include <stdint.h>
#include "drng.h"

extern DRNG_ctx drng_algorithm;

void randombytes(uint8_t *out, size_t outlen) {
	if (outlen > 0)
		get_random_number(&drng_algorithm, out, outlen * 8);
}
