/* Reproduce the secret-seed-dependent XOF fetch count in TriQ's reference sampler.
 * Build from kex-09 as shown in constant_time.md.  No implementation source
 * is changed: vector.c is included only to count its XOF calls. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "symmetric.h"

static unsigned refills;
static void counted_xof_get_bytes(triq_xof_ctx *ctx, uint8_t *out,
                                  uint32_t len);
#define xof_get_bytes counted_xof_get_bytes
#include "TriQ-KEX/Implementations/Reference_Implementation/TriQ-KEX-128/src/ref/vector.c"
#undef xof_get_bytes

static void counted_xof_get_bytes(triq_xof_ctx *ctx, uint8_t *out,
                                  uint32_t len) {
    ++refills;
    xof_get_bytes(ctx, out, len);
}

int main(void) {
    uint8_t seed[SEED_BYTES] = {0};
    triq_xof_ctx ctx;
    uint64_t y[VEC_N_SIZE_64];
    unsigned first_one = 256, first_two = 256;

    for (unsigned i = 0; i < 256; ++i) {
        memset(seed, 0, sizeof seed);
        seed[0] = (uint8_t)i;
        memset(y, 0, sizeof y);
        xof_init(&ctx, seed, sizeof seed);
        refills = 0;
        vect_sample_fixed_weight1(&ctx, y, PARAM_OMEGA);
        if (refills == 1 && first_one == 256) first_one = i;
        if (refills > 1 && first_two == 256) first_two = i;
    }

    printf("TriQ-128 secret-seed expansion: seed[0]=%u uses 1 XOF fetch; "
           "seed[0]=%u uses >1\n", first_one, first_two);
    if (first_one == 256 || first_two == 256) return 1;
    puts("CONFIRMED: reference key-expansion control flow depends on the secret seed");
    return 0;
}
