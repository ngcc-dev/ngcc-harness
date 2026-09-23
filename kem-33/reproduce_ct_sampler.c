/* Count draws in the submitted QUBE decapsulation-key expansion sampler. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "symmetric.h"

static unsigned draws;
static int counted_read_bits(qube_xof_stream_t *ctx, unsigned nbits,
                             uint32_t *value);
#define qube_xof_stream_read_bits counted_read_bits
#include "Implementations/Reference_Implementation/qube-256/src/ref/vector.c"
#undef qube_xof_stream_read_bits

static int counted_read_bits(qube_xof_stream_t *ctx, unsigned nbits,
                             uint32_t *value) {
    ++draws;
    return qube_xof_stream_read_bits(ctx, nbits, value);
}

int main(void) {
    uint8_t seed[SEED_BYTES] = {0};
    uint64_t y[VEC_N_SIZE_64];
    uint32_t support[PARAM_OMEGA_MAX];
    qube_xof_stream_t ctx;
    unsigned baseline = 0, other = 0, n0 = 0, n1 = 0;

    for (unsigned i = 0; i < 256; ++i) {
        memset(seed, 0, sizeof seed);
        seed[0] = (uint8_t)i;
        if (qube_xof_stream_init(&ctx, seed) != 0) return 2;
        draws = 0;
        if (vect_sample_fixed_weight(&ctx, y, support, PARAM_OMEGA_Y1) != 0)
            return 3;
        qube_xof_stream_release(&ctx);
        if (i == 0) {
            baseline = draws;
            n0 = i;
        } else if (draws != baseline) {
            other = draws;
            n1 = i;
            break;
        }
    }
    printf("QUBE-256 secret seed byte %u: %u draws; byte %u: %u draws\n",
           n0, baseline, n1, other);
    if (n1 == 0) return 1;
    puts("CONFIRMED: secret-dependent reference sampler work");
    return 0;
}
