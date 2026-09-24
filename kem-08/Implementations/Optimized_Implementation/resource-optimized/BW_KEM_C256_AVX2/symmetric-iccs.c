#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "auxfunc.h"
#include "params.h"
#include "symmetric.h"

static void kyber_zero_bytes(uint8_t *out, size_t outlen)
{
  if(outlen != 0)
    memset(out, 0, outlen);
}

static int kyber_aux_pseudoxof(uint8_t *out,
                               size_t outlen,
                               const uint8_t *in,
                               size_t inlen)
{
  return iccs_pseudoxof_bytes_scalar(out, outlen, in, inlen);
}

void kyber_iccs_hash_h(uint8_t out[32], const uint8_t *in, size_t inlen)
{
  if(sm3hash(256, in, (unsigned long long)inlen * 8ULL, out) != 0)
    kyber_zero_bytes(out, 32);
}

void kyber_iccs_hash_g(uint8_t out[64], const uint8_t *in, size_t inlen)
{
  if(pseudohash(512, in, (unsigned long long)inlen * 8ULL, out) != 0)
    kyber_zero_bytes(out, 64);
}

void kyber_iccs_xof_absorb(xof_state *state,
                           const uint8_t seed[KYBER_SYMBYTES],
                           uint8_t x,
                           uint8_t y)
{
  memcpy(state->extseed, seed, KYBER_SYMBYTES);
  state->extseed[KYBER_SYMBYTES + 0] = x;
  state->extseed[KYBER_SYMBYTES + 1] = y;
  state->next_counter = 1;
  state->block_pos = sizeof(state->block);
}

void kyber_iccs_xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *state)
{
  size_t chunklen;
  size_t pos = 0;

  if(outblocks == 0)
    return;

  chunklen = outblocks * XOF_BLOCKBYTES;

  while(pos < chunklen) {
    size_t take;
    if(state->block_pos == sizeof(state->block)) {
      if(iccs_sm3_counter_block(state->block,
                                state->extseed,
                                sizeof(state->extseed),
                                state->next_counter++) != 0) {
      kyber_zero_bytes(out, chunklen);
      return;
      }
      state->block_pos = 0;
    }

    take = sizeof(state->block) - state->block_pos;
    if(take > chunklen - pos)
      take = chunklen - pos;
    memcpy(out + pos, state->block + state->block_pos, take);
    state->block_pos += take;
    pos += take;
  }
}

void kyber_iccs_prf(uint8_t *out,
                    size_t outlen,
                    const uint8_t key[KYBER_SYMBYTES],
                    uint8_t nonce)
{
  uint8_t extkey[KYBER_SYMBYTES + 1];

  memcpy(extkey, key, KYBER_SYMBYTES);
  extkey[KYBER_SYMBYTES] = nonce;

  if(kyber_aux_pseudoxof(out, outlen, extkey, sizeof(extkey)) != 0)
    kyber_zero_bytes(out, outlen);
}

void kyber_iccs_rkprf(uint8_t out[KYBER_SSBYTES],
                      const uint8_t key[KYBER_SYMBYTES],
                      const uint8_t input[KYBER_CIPHERTEXTBYTES])
{
  uint8_t extinput[KYBER_SYMBYTES + KYBER_CIPHERTEXTBYTES];

  memcpy(extinput, key, KYBER_SYMBYTES);
  memcpy(extinput + KYBER_SYMBYTES, input, KYBER_CIPHERTEXTBYTES);

  if(kyber_aux_pseudoxof(out, KYBER_SSBYTES, extinput, sizeof(extinput)) != 0)
    kyber_zero_bytes(out, KYBER_SSBYTES);
}
