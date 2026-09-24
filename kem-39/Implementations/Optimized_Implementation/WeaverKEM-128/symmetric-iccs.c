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

void weaver_iccs_hash_h(uint8_t *out, const uint8_t *in, size_t inlen)
{
#if WEAVER_HBYTES == 32
  sm3hash(256, in, (unsigned long long)inlen * 8, out);
#elif WEAVER_HBYTES == 64
  pseudohash(512, in, (unsigned long long)inlen * 8, out);
#elif WEAVER_HBYTES == 128
  pseudohash(1024, in, (unsigned long long)inlen * 8, out);
#else
#error "Unsupported WEAVER_HBYTES"
#endif
}

void weaver_iccs_hash_g(uint8_t *out, const uint8_t *in, size_t inlen)
{
  pseudoXOF((unsigned long long)WEAVER_GBYTES * 8, in, (unsigned long long)inlen * 8, out);
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

void weaver_iccs_rkprf(uint8_t out[WEAVER_SSBYTES],
                       const uint8_t key[WEAVER_SYMBYTES],
                       const uint8_t input[WEAVER_CIPHERTEXTBYTES])
{
  uint8_t buf[WEAVER_SYMBYTES + WEAVER_CIPHERTEXTBYTES];

  memcpy(buf, key, WEAVER_SYMBYTES);
  memcpy(buf + WEAVER_SYMBYTES, input, WEAVER_CIPHERTEXTBYTES);
  pseudoXOF((unsigned long long)WEAVER_SSBYTES * 8, buf, (unsigned long long)sizeof(buf) * 8, out);
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
