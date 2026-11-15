/* sign-19-7: call the archived Phoenix-SHAKE-384s H2 index sampler on a
 * digest for which its fixed 512-byte pool runs out, and on a control. The
 * public seed is 48 bytes of 0xA5; digests are a one-byte label, an 8-byte
 * little-endian counter and zeros. Prints both return values. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "context.h"
#include "hash.h"

static int run(unsigned char label, uint64_t counter)
{
    spx_ctx ctx;
    unsigned char d[SPX_N] = {0};
    uint32_t idx[SPX_TFORS_K];

    memset(&ctx, 0, sizeof ctx);
    memset(ctx.pub_seed, 0xA5, SPX_N);
    d[0] = label;
    for (int i = 0; i < 8; i++) d[1 + i] = (unsigned char)(counter >> (8 * i));
    return h2_generate_indices(idx, d, &ctx);
}

int main(void)
{
    printf("exhausting_digest=%d control_digest=%d k=%d k_prime=%d\n",
           run(0, 5220021), run(0, 5220022), SPX_TFORS_K, SPX_TFORS_K_PRIME);
    return 0;
}
