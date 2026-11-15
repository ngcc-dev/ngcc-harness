/* kem-17-6: the HEP-QC seed-expansion XOF keeps 26 bytes of state.
 * Initialises the archived XOF from a random SEED_BYTES seed, checks that the
 * 208-byte context is zero beyond byte 26, rebuilds a second context from
 * those 26 bytes alone, and requires both to produce the same stream through
 * three squeezes of the size the key expansion uses. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "parameters.h"
#include "symmetric.h"

int main(void)
{
    uint8_t seed[SEED_BYTES], a[3][4096], b[3][4096];
    unsigned x = 2463534242u, tail = 0, same = 1;
    shake256_xof_ctx full = {0}, cut = {0};

    for (size_t i = 0; i < sizeof seed; i++) {
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        seed[i] = (uint8_t)x;
    }
    xof_init(&full, seed, SEED_BYTES);
    for (size_t i = CTX_LEN; i < sizeof full.ctx; i++)
        tail |= ((const uint8_t *)full.ctx)[i];
    memcpy(cut.ctx, full.ctx, CTX_LEN);
    for (int r = 0; r < 3; r++) {
        xof_get_bytes(&full, a[r], sizeof a[r]);
        xof_get_bytes(&cut, b[r], sizeof b[r]);
        same &= !memcmp(a[r], b[r], sizeof a[r]);
    }
    printf("level=%d seed_bits=%d context_bytes=%zu state_bits=%d tail_nonzero=%u stream_from_state=%u\n",
           PARAM_SECURITY, SEED_BYTES * 8, sizeof full.ctx, CTX_LEN * 8, tail != 0, same);
    return 0;
}
