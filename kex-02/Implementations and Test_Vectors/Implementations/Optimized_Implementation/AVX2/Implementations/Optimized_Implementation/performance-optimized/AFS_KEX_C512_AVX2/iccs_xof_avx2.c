#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "auxfunc.h"
#include "iccs_xof_avx2.h"
#include "sm3x8_avx2.h"
#include "symmetric.h"

static void store_counter(uint8_t *p, uint32_t counter)
{
  p[0] = (uint8_t)(counter >> 24);
  p[1] = (uint8_t)(counter >> 16);
  p[2] = (uint8_t)(counter >> 8);
  p[3] = (uint8_t)counter;
}

void iccs_pseudoxof8_bytes(uint8_t *out[8], size_t outlen, const uint8_t *msg[8], size_t msglen)
{
  uint32_t counter = 1;
  size_t off = 0;
  uint8_t digest[8][32];
  int lane;

  if(msglen + 4 > 119) {
    for(lane = 0; lane < 8; lane++)
      iccs_pseudoxof_bytes_scalar(out[lane], outlen, msg[lane], msglen);
    return;
  }

  while(off < outlen) {
    size_t inlen = msglen + 4;
    uint8_t in[8 * inlen];
    size_t take = outlen - off;
    if(take > 32)
      take = 32;

    for(lane = 0; lane < 8; lane++) {
      uint8_t *lane_in = in + (size_t)lane * inlen;
      memcpy(lane_in, msg[lane], msglen);
      store_counter(lane_in + msglen, counter);
    }
    sm3_x8_digest(in, inlen, digest);
    for(lane = 0; lane < 8; lane++)
      memcpy(out[lane] + off, digest[lane], take);

    off += take;
    counter++;
  }
}

void iccs_pseudoxof4_bytes(uint8_t *out[4], size_t outlen, const uint8_t *msg[4], size_t msglen)
{
  uint8_t *out8[8];
  const uint8_t *msg8[8];
  uint8_t dummy_out[4][outlen == 0 ? 1 : outlen];
  uint8_t dummy_msg[4][KYBER_SYMBYTES + 2];
  int lane;

  if(msglen + 4 > 119) {
    for(lane = 0; lane < 4; lane++)
      iccs_pseudoxof_bytes_scalar(out[lane], outlen, msg[lane], msglen);
    return;
  }

  for(lane = 0; lane < 4; lane++) {
    out8[lane] = out[lane];
    msg8[lane] = msg[lane];
  }
  for(; lane < 8; lane++) {
    out8[lane] = dummy_out[lane - 4];
    memset(dummy_msg[lane - 4], 0, sizeof(dummy_msg[lane - 4]));
    msg8[lane] = dummy_msg[lane - 4];
  }
  iccs_pseudoxof8_bytes(out8, outlen, msg8, msglen);
}

void kyber_iccs_prf4(uint8_t *out[4], size_t outlen, const uint8_t key[KYBER_SYMBYTES], uint8_t nonce0)
{
  uint8_t extkey[4][KYBER_SYMBYTES + 1];
  const uint8_t *msg[4];
  int lane;

  for(lane = 0; lane < 4; lane++) {
    memcpy(extkey[lane], key, KYBER_SYMBYTES);
    extkey[lane][KYBER_SYMBYTES] = (uint8_t)(nonce0 + lane);
    msg[lane] = extkey[lane];
  }
  iccs_pseudoxof4_bytes(out, outlen, msg, KYBER_SYMBYTES + 1);
}

void kyber_iccs_xof_squeezeblocks4(uint8_t *out[4], size_t outblocks, const uint8_t seed[KYBER_SYMBYTES], const uint8_t x[4], const uint8_t y[4])
{
  uint8_t extseed[4][KYBER_SYMBYTES + 2];
  const uint8_t *msg[4];
  int lane;

  for(lane = 0; lane < 4; lane++) {
    memcpy(extseed[lane], seed, KYBER_SYMBYTES);
    extseed[lane][KYBER_SYMBYTES] = x[lane];
    extseed[lane][KYBER_SYMBYTES + 1] = y[lane];
    msg[lane] = extseed[lane];
  }
  iccs_pseudoxof4_bytes(out, outblocks * XOF_BLOCKBYTES, msg, KYBER_SYMBYTES + 2);
}

void kyber_iccs_xof_squeezeblocks8(uint8_t *out[8], size_t outblocks, const uint8_t seed[KYBER_SYMBYTES], const uint8_t x[8], const uint8_t y[8])
{
  uint8_t extseed[8][KYBER_SYMBYTES + 2];
  const uint8_t *msg[8];
  int lane;

  for(lane = 0; lane < 8; lane++) {
    memcpy(extseed[lane], seed, KYBER_SYMBYTES);
    extseed[lane][KYBER_SYMBYTES] = x[lane];
    extseed[lane][KYBER_SYMBYTES + 1] = y[lane];
    msg[lane] = extseed[lane];
  }
  iccs_pseudoxof8_bytes(out, outblocks * XOF_BLOCKBYTES, msg, KYBER_SYMBYTES + 2);
}
