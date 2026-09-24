#include <stdint.h>
#include <string.h>

#include "address.h"
#include "params.h"
#include "hash.h"
#include "hash_sm3.h"
#include "sm3_internal.h"
#include "hashx8.h"
#include "utils.h"

void prf_addrx8(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3,
                unsigned char *out4,
                unsigned char *out5,
                unsigned char *out6,
                unsigned char *out7,
                const spx_ctx *ctx,
                const uint32_t addrx8[8 * 8])
{
    SPX_VLA(uint8_t, buf0, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf1, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf2, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf3, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf4, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf5, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf6, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf7, SPX_SM3_ADDR_BYTES + SPX_N);

    memcpy(buf0, addrx8 + 0 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf1, addrx8 + 1 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf2, addrx8 + 2 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf3, addrx8 + 3 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf4, addrx8 + 4 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf5, addrx8 + 5 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf6, addrx8 + 6 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf7, addrx8 + 7 * 8, SPX_SM3_ADDR_BYTES);

    memcpy(buf0 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf1 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf2 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf3 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf4 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf5 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf6 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf7 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);

    sm3x8_avx2(out0, out1, out2, out3,
               out4, out5, out6, out7,
               buf0, buf1, buf2, buf3,
               buf4, buf5, buf6, buf7,
               SPX_SM3_ADDR_BYTES + SPX_N);
}