#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "symmetric.h"
#include "fips202.h"

void weaver_expand_keypair_seeds(uint8_t out[2 * WEAVER_SYMBYTES], const uint8_t *in, size_t inlen)
{
  shake256(out, 2 * WEAVER_SYMBYTES, in, inlen);
}

/*************************************************
* Name:        weaver_shake128_absorb
*
* Description: Absorb step of the mode-selected XOF.
*
* Arguments:   - keccak_state *state: pointer to (uninitialized) output Keccak state
*              - const uint8_t *seed: pointer to WEAVER_SYMBYTES input to be absorbed into state
*              - uint8_t i: additional byte of input
*              - uint8_t j: additional byte of input
**************************************************/
void weaver_shake128_absorb(keccak_state *state,
                           const uint8_t seed[WEAVER_SYMBYTES],
                           uint8_t x,
                           uint8_t y)
{
  uint8_t extseed[WEAVER_SYMBYTES+2];

  memcpy(extseed, seed, WEAVER_SYMBYTES);
  extseed[WEAVER_SYMBYTES+0] = x;
  extseed[WEAVER_SYMBYTES+1] = y;

#if WEAVER_MODE == 1
  shake128_absorb_once(state, extseed, sizeof(extseed));
#else
  shake256_absorb_once(state, extseed, sizeof(extseed));
#endif
}

/*************************************************
* Name:        weaver_shake256_prf
*
* Description: Usage of SHAKE256 as a PRF, concatenates secret and public input
*              and then generates outlen bytes of SHAKE256 output
*
* Arguments:   - uint8_t *out: pointer to output
*              - size_t outlen: number of requested output bytes
*              - const uint8_t *key: pointer to the key (of length WEAVER_SYMBYTES)
*              - uint8_t nonce: single-byte nonce (public PRF input)
**************************************************/
void weaver_shake256_prf(uint8_t *out, size_t outlen, const uint8_t key[WEAVER_SYMBYTES], uint8_t nonce)
{
  uint8_t extkey[WEAVER_SYMBYTES+1];

  memcpy(extkey, key, WEAVER_SYMBYTES);
  extkey[WEAVER_SYMBYTES] = nonce;

  shake256(out, outlen, extkey, sizeof(extkey));
}


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
