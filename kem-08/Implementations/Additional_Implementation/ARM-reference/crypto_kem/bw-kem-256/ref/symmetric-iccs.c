#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "auxfunc.h"
#include "params.h"
#include "symmetric.h"

#ifdef PROFILE_FUNCTIONS
#include "hal.h"
extern unsigned long long pseudoxof_cycles;
extern unsigned long long pseudohash_cycles;
#endif

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
#ifdef PROFILE_FUNCTIONS
  uint64_t t0 = hal_get_time();
#endif
  int rc = pseudoXOF((unsigned long long)outlen * 8ULL,
                     in,
                     (unsigned long long)inlen * 8ULL,
                     out);
#ifdef PROFILE_FUNCTIONS
  uint64_t t1 = hal_get_time();
  pseudoxof_cycles += (t1 - t0);
#endif
  return rc;
}

void kyber_iccs_hash_h(uint8_t out[32], const uint8_t *in, size_t inlen)
{
  int rc = sm3hash(256, in, (unsigned long long)inlen * 8ULL, out);
  if(rc != 0)
    kyber_zero_bytes(out, 32);
}

void kyber_iccs_hash_g(uint8_t out[64], const uint8_t *in, size_t inlen)
{
#ifdef PROFILE_FUNCTIONS
  uint64_t t0 = hal_get_time();
#endif
  int rc = pseudohash(512, in, (unsigned long long)inlen * 8ULL, out);
#ifdef PROFILE_FUNCTIONS
  uint64_t t1 = hal_get_time();
  pseudohash_cycles += (t1 - t0);
#endif
  if(rc != 0)
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
  state->squeezed_bytes = 0;
}

void kyber_iccs_xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *state)
{
  size_t offset;
  size_t chunklen;
  size_t total;

  if(outblocks == 0)
    return;

  offset = state->squeezed_bytes;
  chunklen = outblocks * XOF_BLOCKBYTES;
  total = offset + chunklen;

  {
    uint8_t stream[total];

    if(kyber_aux_pseudoxof(stream, total, state->extseed, sizeof(state->extseed)) != 0) {
      kyber_zero_bytes(out, chunklen);
      return;
    }

    memcpy(out, stream + offset, chunklen);
  }

  state->squeezed_bytes = total;
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
