#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "address.h"
#include "hash_sm3.h"
#include "hash_sm3x4.h"
#include "params.h"
#include "sm3_internal.h"
#include "utils.h"

static void sm3_xofx4_scalar(unsigned char *out0, unsigned char *out1,
                             unsigned char *out2, unsigned char *out3,
                             unsigned long outlen,
                             const unsigned char *in0,
                             const unsigned char *in1,
                             const unsigned char *in2,
                             const unsigned char *in3,
                             unsigned long inlen)
{
    sm3_xof(out0, outlen, in0, inlen);
    sm3_xof(out1, outlen, in1, inlen);
    sm3_xof(out2, outlen, in2, inlen);
    sm3_xof(out3, outlen, in3, inlen);
}

static void store_be32(unsigned char out[4], uint32_t x)
{
    out[0] = (unsigned char)(x >> 24);
    out[1] = (unsigned char)(x >> 16);
    out[2] = (unsigned char)(x >> 8);
    out[3] = (unsigned char)x;
}

void sm3_xofx4(unsigned char *out0, unsigned char *out1,
               unsigned char *out2, unsigned char *out3,
               unsigned long outlen,
               const unsigned char *in0, const unsigned char *in1,
               const unsigned char *in2, const unsigned char *in3,
               unsigned long inlen)
{
    if (outlen == 0) {
        return;
    }

#if defined(AVX2_OPTIMIZED)
    size_t msg_len = (size_t)inlen + 4;
    unsigned char *buf0 = (unsigned char *)malloc(msg_len);
    unsigned char *buf1 = (unsigned char *)malloc(msg_len);
    unsigned char *buf2 = (unsigned char *)malloc(msg_len);
    unsigned char *buf3 = (unsigned char *)malloc(msg_len);
    unsigned char block0[SPX_SM3_OUTPUT_BYTES], block1[SPX_SM3_OUTPUT_BYTES];
    unsigned char block2[SPX_SM3_OUTPUT_BYTES], block3[SPX_SM3_OUTPUT_BYTES];
    unsigned char tmp4[SPX_SM3_OUTPUT_BYTES], tmp5[SPX_SM3_OUTPUT_BYTES];
    unsigned char tmp6[SPX_SM3_OUTPUT_BYTES], tmp7[SPX_SM3_OUTPUT_BYTES];
    unsigned long produced = 0;
    uint32_t ct = 1;

    if (buf0 == NULL || buf1 == NULL || buf2 == NULL || buf3 == NULL) {
        free(buf0);
        free(buf1);
        free(buf2);
        free(buf3);
        sm3_xofx4_scalar(out0, out1, out2, out3, outlen,
                         in0, in1, in2, in3, inlen);
        return;
    }

    memcpy(buf0, in0, inlen);
    memcpy(buf1, in1, inlen);
    memcpy(buf2, in2, inlen);
    memcpy(buf3, in3, inlen);

    while (produced < outlen) {
        unsigned long take = outlen - produced;
        if (take > SPX_SM3_OUTPUT_BYTES) {
            take = SPX_SM3_OUTPUT_BYTES;
        }

        store_be32(buf0 + inlen, ct);
        store_be32(buf1 + inlen, ct);
        store_be32(buf2 + inlen, ct);
        store_be32(buf3 + inlen, ct);

        sm3x8_avx2(block0, block1, block2, block3,
                   tmp4, tmp5, tmp6, tmp7,
                   buf0, buf1, buf2, buf3,
                   buf0, buf1, buf2, buf3,
                   msg_len);

        memcpy(out0 + produced, block0, take);
        memcpy(out1 + produced, block1, take);
        memcpy(out2 + produced, block2, take);
        memcpy(out3 + produced, block3, take);
        produced += take;
        ct++;
    }

    free(buf0);
    free(buf1);
    free(buf2);
    free(buf3);
#else
    sm3_xofx4_scalar(out0, out1, out2, out3, outlen,
                     in0, in1, in2, in3, inlen);
#endif
}

void prf_addrx4(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3,
                const spx_ctx *ctx,
                const uint32_t addrx4[4 * 8])
{
    SPX_VLA(uint8_t, buf0, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf1, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf2, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf3, SPX_SM3_ADDR_BYTES + SPX_N);

    memcpy(buf0, addrx4 + 0 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf1, addrx4 + 1 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf2, addrx4 + 2 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf3, addrx4 + 3 * 8, SPX_SM3_ADDR_BYTES);

    memcpy(buf0 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf1 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf2 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf3 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);

    sm3_xofx4(out0, out1, out2, out3, SPX_N,
              buf0, buf1, buf2, buf3,
              SPX_SM3_ADDR_BYTES + SPX_N);
}
