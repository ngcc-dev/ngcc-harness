/* randombytes() for the pq-crystals reference code, drawn from the ICCS DRNG
 * context that the harness seeds (api/drng.c, the link shim's drng_algorithm).
 * It replaces the upstream randombytes.c (/dev/urandom), so ML-KEM and ML-DSA
 * take their randomness exactly as the NGCC candidates do: the benchmark's
 * seeded generator, at the same per-call cost. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "drng.h"
#include "randombytes.h"

extern DRNG_ctx drng_algorithm;

void randombytes(uint8_t *out, size_t outlen)
{
    if (outlen && get_random_number(&drng_algorithm, out, 8ULL * outlen) != 0)
        abort();    /* the reference code has no error path for randomness */
}
