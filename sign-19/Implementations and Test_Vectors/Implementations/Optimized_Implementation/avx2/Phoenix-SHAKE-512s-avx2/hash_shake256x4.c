#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "address.h"
#include "params.h"
#include "fips202x4.h"
#include "hashx4.h"

void prf_addrx4(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3,
                const spx_ctx *ctx,
                const uint32_t addrx4[4 * 8])
{
    unsigned char buf0[SPX_N + SPX_ADDR_BYTES + SPX_N];
    unsigned char buf1[SPX_N + SPX_ADDR_BYTES + SPX_N];
    unsigned char buf2[SPX_N + SPX_ADDR_BYTES + SPX_N];
    unsigned char buf3[SPX_N + SPX_ADDR_BYTES + SPX_N];

    memcpy(buf0, ctx->pub_seed, SPX_N);
    memcpy(buf1, ctx->pub_seed, SPX_N);
    memcpy(buf2, ctx->pub_seed, SPX_N);
    memcpy(buf3, ctx->pub_seed, SPX_N);

    memcpy(buf0 + SPX_N, addrx4 + 0*8, SPX_ADDR_BYTES);
    memcpy(buf1 + SPX_N, addrx4 + 1*8, SPX_ADDR_BYTES);
    memcpy(buf2 + SPX_N, addrx4 + 2*8, SPX_ADDR_BYTES);
    memcpy(buf3 + SPX_N, addrx4 + 3*8, SPX_ADDR_BYTES);

    memcpy(buf0 + SPX_N + SPX_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf1 + SPX_N + SPX_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf2 + SPX_N + SPX_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf3 + SPX_N + SPX_ADDR_BYTES, ctx->sk_seed, SPX_N);

    shake256x4(out0, out1, out2, out3, SPX_N,
               buf0, buf1, buf2, buf3, SPX_N + SPX_ADDR_BYTES + SPX_N);
}
