#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "drng.h"
#include "randombytes.h"

// DRNG_ctx is initialized in KAT_KEM.c before kem_keygen/kem_enc are called.
extern DRNG_ctx drng_algorithm;

void randombytes(uint8_t *out, size_t outlen)
{
	if (out == NULL || outlen == 0)
		return;

	if (get_random_number(&drng_algorithm, out, (unsigned long long)outlen * 8ULL) != 0)
	{
		fprintf(stderr, "ERROR: get_random_number failed in randombytes_api.c\n");
		abort();
	}
}
