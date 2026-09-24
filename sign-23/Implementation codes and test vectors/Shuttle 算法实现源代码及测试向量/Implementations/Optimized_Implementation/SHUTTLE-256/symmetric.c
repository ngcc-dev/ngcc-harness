/*
 * symmetric.c -- the four scalar XOF primitive bodies.
 *
 * External linkage (matches the non-static prototypes in xof.h).  The MODE
 * switch lives in xof.h; this TU just provides the bodies.
 *
 *   NGCC_MODE (default): SM3 Hash-DRBG, drng.{c,h}.  Bytes->bits *8 shim.
 *   SHA3_MODE: SHAKE128 (xof128) / SHAKE256 (xof256), fips202.{c,h}.
 *
 * Compiled in BOTH modes (the body set is selected by the preprocessor);
 * under SHA3_MODE the fips202.c TU must also be linked, under NGCC_MODE
 * drng.c.
 */

#include <stddef.h>
#include <stdint.h>

#include "test/prof.h" /* PROF_SQ -- ((void)0) unless PROF_RAND defined */
#include "xof.h"

#if defined(SHA3_MODE)

/* ===== SHA3_MODE: SHAKE128 (public) / SHAKE256 (secret) =================
 *
 * Each xofK_init does shake*_init + shake*_absorb_once over the whole
 * pre-concatenated (tag||seed||nonce) buffer; each xofK_squeeze is the
 * incremental, rate-buffered shake*_squeeze.
 *
 * NOTE on the binding (Impl-Design vs Dilithium API): the Impl-Design
 * binds xof128_init = shake128_absorb_once directly, but Dilithium's
 * shake128_absorb_once requires a prior shake128_init to zero state->pos
 * cleanly.  Our wrapper does init THEN absorb_once -- equivalent and
 * KAT-stable.  This is the only place the binding is a 2-call wrapper
 * rather than a literal alias. */

void xof128_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len)
{
    shake128_init(ctx);
    shake128_absorb_once(ctx, seed, seed_len);
}

void xof128_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len)
{
    PROF_SQ(out_len); /* randomness accounting (PROF_RAND only) */
    shake128_squeeze(out, out_len, ctx); /* incremental, rate-buffered */
}

void xof256_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len)
{
    shake256_init(ctx);
    shake256_absorb_once(ctx, seed, seed_len);
}

void xof256_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len)
{
    PROF_SQ(out_len); /* randomness accounting (PROF_RAND only) */
    shake256_squeeze(out, out_len, ctx);
}

#else /* NGCC_MODE (default)                                             \
       * =============================================                   \
       *                                                                 \
       * SM3 Hash-DRBG (drng.{c,h}, UNMODIFIABLE).  xof_ctx == DRNG_ctx. \
       *   xof256_init   = init_random_number  (seed_len already in      \
       * BYTES) xof256_squeeze= get_random_number   (out_len * 8 -- the                 \
       * BYTES->BITS shim) xof128_* are byte-exact aliases of xof256_*                             \
       * (the 128/256 collapse): SM3 cannot reach 256-bit, so both                                       \
       * names drive the identical SM3 DRBG.  We implement them as                                                       \
       * separate function bodies (not #define aliases) so the namespaced                                                              \
       * symbols xofK_init / xofK_squeeze all exist for the linker and   \
       * the AVX backends can mirror the same four-symbol surface. */

void xof256_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len)
{
    /* seed_len in BYTES -- init_random_number also takes BYTES, no
     * conversion */
    (void)init_random_number(ctx, seed, (unsigned long long)seed_len);
}

void xof256_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len)
{
    /* BYTES -> BITS: get_random_number takes a BIT length (drng.h).  We
     * only ever request whole bytes, so the MSB-first high-bit masking of
     * the last byte inside get_random_number never fires on this plumbing
     * path. */
    PROF_SQ(out_len); /* randomness accounting (PROF_RAND only) */
    (void)get_random_number(ctx, out, (unsigned long long)out_len * 8u);
}

void xof128_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len)
{
    xof256_init(ctx, seed, seed_len);
}

void xof128_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len)
{
    xof256_squeeze(ctx, out, out_len);
}

#endif /* SHA3_MODE / NGCC_MODE */
