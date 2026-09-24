/*
Local replacement for pq-crystals/dilithium/ref/randombytes.c.

The upstream randombytes() calls the OS RNG (getrandom / RtlGenRandom).
For ICCS KAT reproducibility we instead route every randombytes() call
through:

  1. adkex_sig_random_hook() — if the SIG adapter has injected coins,
     they're consumed first. Used to make keygen derand-friendly.

  2. drng_algorithm — the ICCS DRNG seeded by KAT_KEX.c, so any
     unscheduled randomness (e.g. randomised signing) is still
     reproducible from the KAT seed.

This file is per-ADKEX-variant because each builds its own KAT_KEX
linked against its own drng_algorithm instance; the vendored ML-DSA
sources only ever see this randombytes() implementation.
*/

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "dilithium/randombytes.h"
#include "adkex_sig.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

void randombytes(uint8_t *out, size_t outlen)
{
    if (adkex_sig_random_hook(out, (unsigned long long)outlen) == 0) return;
    get_random_number(&drng_algorithm, out, (unsigned long long)outlen * 8);
}
