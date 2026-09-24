/*
 * ICCS symmetric primitives (auxfunc sm3hash / pseudohash / pseudoXOF).
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "symmetric.h"
#include "auxfunc.h"

#define ICCS_XOF_MAX_BYTES 8192

void weaver_iccs_xof_absorb(xof_state *state,
                            const uint8_t seed[WEAVER_SYMBYTES],
                            uint8_t x,
                            uint8_t y)
{
  memcpy(state->msg, seed, WEAVER_SYMBYTES);
  state->msg[WEAVER_SYMBYTES] = x;
  state->msg[WEAVER_SYMBYTES + 1] = y;
  state->msg_len = WEAVER_SYMBYTES + 2;
  state->pos = 0;
}

static void iccs_xof_squeeze(uint8_t *out, size_t outlen, xof_state *state)
{
  uint8_t buf[ICCS_XOF_MAX_BYTES];

  if(state->pos + outlen > ICCS_XOF_MAX_BYTES)
    outlen = ICCS_XOF_MAX_BYTES - state->pos;

  pseudoXOF((unsigned long long)(state->pos + outlen) * 8,
            state->msg,
            (unsigned long long)state->msg_len * 8,
            buf);
  memcpy(out, buf + state->pos, outlen);
  state->pos += outlen;
}

void weaver_iccs_xof_squeezeblocks(uint8_t *out, size_t nblocks, xof_state *state)
{
  iccs_xof_squeeze(out, nblocks * XOF_BLOCKBYTES, state);
}

void weaver_iccs_expand_keypair_seeds(uint8_t *out, const uint8_t *in, size_t inlen)
{
  pseudoXOF((unsigned long long)(2 * WEAVER_SYMBYTES) * 8, in, (unsigned long long)inlen * 8, out);
}

void weaver_iccs_hash_kr(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
  pseudoXOF((unsigned long long)outlen * 8, in, (unsigned long long)inlen * 8, out);
}

void weaver_iccs_prf(uint8_t *out, size_t outlen, const uint8_t key[WEAVER_SYMBYTES], uint8_t nonce)
{
  uint8_t extkey[WEAVER_SYMBYTES + 1];

  memcpy(extkey, key, WEAVER_SYMBYTES);
  extkey[WEAVER_SYMBYTES] = nonce;
  pseudoXOF((unsigned long long)outlen * 8, extkey, (unsigned long long)sizeof(extkey) * 8, out);
}

void weaver_iccs_xof_absorb_wrap(xof_state *state,
                                 const uint8_t seed[WEAVER_SYMBYTES],
                                 uint8_t x,
                                 uint8_t y)
{
  weaver_iccs_xof_absorb(state, seed, x, y);
}

void weaver_iccs_xof_squeezeblocks_wrap(uint8_t *out, size_t nblocks, xof_state *state)
{
  weaver_iccs_xof_squeezeblocks(out, nblocks, state);
}

/* NGCC_MODE (default)                                             \
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
