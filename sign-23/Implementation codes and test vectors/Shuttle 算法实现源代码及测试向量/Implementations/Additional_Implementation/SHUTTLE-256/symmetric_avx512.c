/*
 * symmetric_avx512.c -- AVX512 lane-batched XOF variant bodies.
 *
 * Implements xof128/256_avx512_init/squeeze on top of the N-way
 * primitives. Same contract as symmetric_avx2.c (lengths shared, pointer
 * arrays per-lane, nothing validated), at the AVX512 lane width.
 *
 *   NGCC_MODE: lanes = XOF_LANES_AVX512 = 16 (SM3 DRBG).  Body =
 *       init_random_number_avx512 / get_random_number_avx512(..,
 * out_len*8). In the fixed 16-stream flow this is a SINGLE pass.
 *
 *   SHA3_MODE: lanes = XOF_LANES_AVX512 = 8 (Keccak).  Body =
 *       shake{128,256}x8_absorb_once + buffered
 * shake{128,256}x8_squeezeblocks. In the 16-stream flow this is TWO passes
 * (8 lanes x 2).
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SHUTTLE_XOF_DECLARE_AVX512 1
#include "symmetric.h"

#if defined(SHA3_MODE)

#    include "fips202x8.h"

/* 8-lane SHAKE squeeze with a buffered final partial block.  out[k] receives
 * out_len bytes for lane k.  We squeeze whole rate blocks straight into the
 * destination, then one extra block into a scratch row to copy the tail.
 *
 * RATE is a COMPILE-TIME CONSTANT (SHAKE128_RATE / SHAKE256_RATE), passed as
 * a template parameter through the SHAKEX8_SQUEEZE_BODY macro so the
 * `out_len / RATE` becomes a constant-divisor multiply-shift rather than a
 * hardware `div`.  Mirrors symmetric_avx2.c's SHAKEX4_SQUEEZE_BODY.  out_len
 * is the (public) squeeze length, never secret, so the divide would be safe
 * regardless -- but keeping it a constant divisor keeps the constant-time
 * scanner (tools/ct_scan.py) clean now that this TU sits on the secret-seeded
 * ExpandS/SampleY path via the N-way batched refill
 * (USE_AVX512_XOF_NWAY). */
#    define SHAKEX8_SQUEEZE_BODY(out, out_len, st, RATE, SQBLK)              \
        do {                                                                \
            /* Division-free nblocks/off (out_len is the PUBLIC squeeze       \
             * length): gcc -Os re-emits a hardware `div` for `out_len/RATE`  \
             * even with RATE a compile-time constant (the KyberSlash class), \
             * which tools/ct_scan.py forbids.  This accumulator loop avoids  \
             * any divide under every compiler/opt; the loop count is public. */ \
            size_t off_ = 0, nblocks_ = 0;                                   \
            while (off_ + (RATE) <= (out_len)) {                             \
                off_ += (RATE);                                             \
                ++nblocks_;                                                 \
            }                                                               \
            size_t tail_ = (out_len) - off_;                                \
            uint8_t b0_[RATE], b1_[RATE], b2_[RATE], b3_[RATE], b4_[RATE],   \
                b5_[RATE], b6_[RATE], b7_[RATE];                            \
            if (nblocks_)                                                    \
                SQBLK((out)[0], (out)[1], (out)[2], (out)[3], (out)[4],      \
                      (out)[5], (out)[6], (out)[7], nblocks_, (st));         \
            if (tail_) {                                                     \
                size_t k_;                                                   \
                uint8_t *bp_[8];                                            \
                bp_[0] = b0_;                                                \
                bp_[1] = b1_;                                                \
                bp_[2] = b2_;                                                \
                bp_[3] = b3_;                                                \
                bp_[4] = b4_;                                                \
                bp_[5] = b5_;                                                \
                bp_[6] = b6_;                                                \
                bp_[7] = b7_;                                                \
                SQBLK(bp_[0], bp_[1], bp_[2], bp_[3], bp_[4], bp_[5],        \
                      bp_[6], bp_[7], 1, (st));                              \
                for (k_ = 0; k_ < 8; k_++)                                   \
                    memcpy((out)[k_] + off_, bp_[k_], tail_);                \
            }                                                               \
        } while (0)

void xof128_avx512_init(xof_ctx_avx512 *ctx,
                        const uint8_t *const seed[XOF_LANES_AVX512],
                        size_t seed_len)
{
    shake128x8_absorb_once(ctx, seed[0], seed[1], seed[2], seed[3],
                           seed[4], seed[5], seed[6], seed[7], seed_len);
}

void xof128_avx512_squeeze(xof_ctx_avx512 *ctx,
                           uint8_t *const out[XOF_LANES_AVX512],
                           size_t out_len)
{
    SHAKEX8_SQUEEZE_BODY(out, out_len, ctx, SHAKE128_RATE,
                         shake128x8_squeezeblocks);
}

void xof256_avx512_init(xof_ctx_avx512 *ctx,
                        const uint8_t *const seed[XOF_LANES_AVX512],
                        size_t seed_len)
{
    shake256x8_absorb_once(ctx, seed[0], seed[1], seed[2], seed[3],
                           seed[4], seed[5], seed[6], seed[7], seed_len);
}

void xof256_avx512_squeeze(xof_ctx_avx512 *ctx,
                           uint8_t *const out[XOF_LANES_AVX512],
                           size_t out_len)
{
    SHAKEX8_SQUEEZE_BODY(out, out_len, ctx, SHAKE256_RATE,
                         shake256x8_squeezeblocks);
}

#else /* NGCC_MODE: 16-way SM3 DRBG ==================================== \
       */

#    include "drng_avx512.h"

void xof256_avx512_init(xof_ctx_avx512 *ctx,
                        const uint8_t *const seed[XOF_LANES_AVX512],
                        size_t seed_len)
{
    (void)init_random_number_avx512(ctx, seed,
                                    (unsigned long long)seed_len);
}

void xof256_avx512_squeeze(xof_ctx_avx512 *ctx,
                           uint8_t *const out[XOF_LANES_AVX512],
                           size_t out_len)
{
    (void)get_random_number_avx512(ctx, out,
                                   (unsigned long long)out_len * 8u);
}

void xof128_avx512_init(xof_ctx_avx512 *ctx,
                        const uint8_t *const seed[XOF_LANES_AVX512],
                        size_t seed_len)
{
    xof256_avx512_init(ctx, seed, seed_len);
}

void xof128_avx512_squeeze(xof_ctx_avx512 *ctx,
                           uint8_t *const out[XOF_LANES_AVX512],
                           size_t out_len)
{
    xof256_avx512_squeeze(ctx, out, out_len);
}

#endif /* SHA3_MODE / NGCC_MODE */
