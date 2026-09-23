/* Count XOF fetches in the submitted TriQ-KEM secret-key sampler. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "symmetric.h"

static unsigned fetches;
static void counted_xof_get_bytes(triq_xof_ctx *ctx, uint8_t *out,
                                  uint32_t len);
#define xof_get_bytes counted_xof_get_bytes
#include "Implementations/Reference_Implementation/TriQ-KEM-128/src/ref/vector.c"
#undef xof_get_bytes

static void counted_xof_get_bytes(triq_xof_ctx *ctx, uint8_t *out,
                                  uint32_t len) {
    ++fetches;
    xof_get_bytes(ctx, out, len);
}

int main(void) {
    uint8_t seed[SEED_BYTES] = {0};
    triq_xof_ctx ctx;
    uint64_t y[VEC_N_SIZE_64];
    unsigned first_one = 256, first_many = 256;

    for (unsigned i = 0; i < 256; ++i) {
        memset(seed, 0, sizeof seed);
        seed[0] = (uint8_t)i;
        memset(y, 0, sizeof y);
        xof_init(&ctx, seed, sizeof seed);
        fetches = 0;
        vect_sample_fixed_weight1(&ctx, y, PARAM_OMEGA);
        if (fetches == 1 && first_one == 256) first_one = i;
        if (fetches > 1 && first_many == 256) first_many = i;
    }

    printf("TriQ-KEM-128 secret seed byte %u: one XOF fetch; "
           "byte %u: multiple fetches\n", first_one, first_many);
    if (first_one == 256 || first_many == 256) return 1;
    puts("CONFIRMED: secret-dependent reference sampler work");
    return 0;
}
