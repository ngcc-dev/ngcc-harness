/* Phoenix-SHAKE-128f 4-way parallel SHAKE implementation
 *
 * This module provides AVX2 optimized 4-lane parallel SHAKE128/256
 * extendable-output functions for Phoenix signature scheme operations.
 *
 * Key features:
 * - 4-way parallel message absorption using KeccakP1600times4_PermuteAll
 * - Optimized for high-throughput signing operations
 * - Compatible with FIPS202 SHAKE interfaces
 */

#include <immintrin.h>
#include <stdint.h>
#include <assert.h>

#include "fips202.h"
#include "fips202x4.h"

#define PHOENIX_NROUNDS 24
#define ROL_M256(val, shift) ((val << shift) ^ (val >> (64-shift)))

/*************************************************
 * Name:        ull_from_bytes
 *
 * Description: Convert 8 consecutive bytes into uint64_t (little-endian)
 *
 * Arguments:   - const unsigned char *input: pointer to input bytes
 *
 * Returns the 64-bit unsigned integer value
 **************************************************/
static uint64_t ull_from_bytes(const unsigned char *input)
{
  uint64_t result = 0;
  for (size_t idx = 0; idx < 8; ++idx) {
    result |= (uint64_t)input[idx] << (8 * idx);
  }
  return result;
}

/*************************************************
 * Name:        ull_to_bytes
 *
 * Description: Convert uint64_t into 8 bytes (little-endian)
 *
 * Arguments:   - uint8_t *output: pointer to output buffer
 *              - uint64_t val: input value
 **************************************************/
static void ull_to_bytes(uint8_t *output, uint64_t val)
{
  for (size_t idx = 0; idx < 8; ++idx) {
    output[idx] = (uint8_t)val;
    val >>= 8;
  }
}

/* Use implementation from the Keccak Code Package */
extern void KeccakP1600times4_PermuteAll_24rounds(__m256i *s);
#define KeccakP1600_Permute4x KeccakP1600times4_PermuteAll_24rounds

/*************************************************
 * Name:        keccak_absorb_4x
 *
 * Description: Absorb 4 messages in parallel into Keccak state
 *
 * Arguments:   - __m256i *state: pointer to 4-lane Keccak state
 *              - unsigned int rate: absorption rate in bytes
 *              - const unsigned char *m0..3: pointers to 4 input messages
 *              - unsigned long long mlen: length of each message
 *              - unsigned char pad: domain separation padding byte
 **************************************************/
static void keccak_absorb_4x(__m256i *state,
                          unsigned int rate,
                          const unsigned char *m0,
                          const unsigned char *m1,
                          const unsigned char *m2,
                          const unsigned char *m3,
                          unsigned long long mlen,
                          unsigned char pad)
{
  size_t idx;
  unsigned char padbuf0[200];
  unsigned char padbuf1[200];
  unsigned char padbuf2[200];
  unsigned char padbuf3[200];

  uint64_t *state_ll = (uint64_t *)state;

  /* Absorb full rate-sized blocks */
  while (mlen >= rate)
  {
    for (idx = 0; idx < rate / 8; ++idx)
    {
      state_ll[4*idx+0] ^= ull_from_bytes(m0 + 8 * idx);
      state_ll[4*idx+1] ^= ull_from_bytes(m1 + 8 * idx);
      state_ll[4*idx+2] ^= ull_from_bytes(m2 + 8 * idx);
      state_ll[4*idx+3] ^= ull_from_bytes(m3 + 8 * idx);
    }

    KeccakP1600_Permute4x(state);
    mlen -= rate;
    m0 += rate;
    m1 += rate;
    m2 += rate;
    m3 += rate;
  }

  /* Handle remaining bytes with padding */
  for (idx = 0; idx < rate; ++idx)
  {
    padbuf0[idx] = 0;
    padbuf1[idx] = 0;
    padbuf2[idx] = 0;
    padbuf3[idx] = 0;
  }
  for (idx = 0; idx < mlen; ++idx)
  {
    padbuf0[idx] = m0[idx];
    padbuf1[idx] = m1[idx];
    padbuf2[idx] = m2[idx];
    padbuf3[idx] = m3[idx];
  }

  padbuf0[idx] = pad;
  padbuf1[idx] = pad;
  padbuf2[idx] = pad;
  padbuf3[idx] = pad;

  padbuf0[rate - 1] |= 128;
  padbuf1[rate - 1] |= 128;
  padbuf2[rate - 1] |= 128;
  padbuf3[rate - 1] |= 128;

  for (idx = 0; idx < rate / 8; ++idx)
  {
    state_ll[4*idx+0] ^= ull_from_bytes(padbuf0 + 8 * idx);
    state_ll[4*idx+1] ^= ull_from_bytes(padbuf1 + 8 * idx);
    state_ll[4*idx+2] ^= ull_from_bytes(padbuf2 + 8 * idx);
    state_ll[4*idx+3] ^= ull_from_bytes(padbuf3 + 8 * idx);
  }
}

/*************************************************
 * Name:        keccak_squeeze_4x
 *
 * Description: Squeeze 4 output blocks in parallel from Keccak state
 *
 * Arguments:   - unsigned char *h0..3: output buffer pointers
 *              - unsigned long long nblocks: number of blocks to squeeze
 *              - __m256i *state: pointer to 4-lane Keccak state
 *              - unsigned int rate: squeeze rate in bytes
 **************************************************/
static void keccak_squeeze_4x(unsigned char *h0,
                                   unsigned char *h1,
                                   unsigned char *h2,
                                   unsigned char *h3,
                                   unsigned long long nblocks,
                                   __m256i *state,
                                   unsigned int rate)
{
  size_t idx;
  uint64_t *state_ll = (uint64_t *)state;

  while(nblocks > 0)
  {
    KeccakP1600_Permute4x(state);
    for(idx = 0; idx < (rate >> 3); idx++)
    {
      ull_to_bytes(h0 + 8*idx, state_ll[4*idx+0]);
      ull_to_bytes(h1 + 8*idx, state_ll[4*idx+1]);
      ull_to_bytes(h2 + 8*idx, state_ll[4*idx+2]);
      ull_to_bytes(h3 + 8*idx, state_ll[4*idx+3]);
    }
    h0 += rate;
    h1 += rate;
    h2 += rate;
    h3 += rate;
    nblocks--;
  }
}

/*************************************************
 * Name:        shake128x4
 *
 * Description: SHAKE128 XOF with 4-way parallel processing
 *
 * Arguments:   - unsigned char *out0..3: output pointers
 *              - unsigned long long outlen: total output length per lane
 *              - unsigned char *in0..3: input message pointers
 *              - unsigned long long inlen: input length (identical for all)
 **************************************************/
void shake128x4(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3, unsigned long long outlen,
                unsigned char *in0,
                unsigned char *in1,
                unsigned char *in2,
                unsigned char *in3, unsigned long long inlen)
{
  __m256i state[25];
  unsigned char tmp0[SHAKE128_RATE];
  unsigned char tmp1[SHAKE128_RATE];
  unsigned char tmp2[SHAKE128_RATE];
  unsigned char tmp3[SHAKE128_RATE];
  size_t idx;

  /* Initialize state to zero */
  for(idx = 0; idx < 25; idx++)
    state[idx] = _mm256_xor_si256(state[idx], state[idx]);

  /* Absorb 4 messages of identical length in parallel */
  keccak_absorb_4x(state, SHAKE128_RATE, in0, in1, in2, in3, inlen, 0x1F);

  /* Squeeze full blocks */
  keccak_squeeze_4x(out0, out1, out2, out3, outlen/SHAKE128_RATE, state, SHAKE128_RATE);

  out0 += (outlen/SHAKE128_RATE)*SHAKE128_RATE;
  out1 += (outlen/SHAKE128_RATE)*SHAKE128_RATE;
  out2 += (outlen/SHAKE128_RATE)*SHAKE128_RATE;
  out3 += (outlen/SHAKE128_RATE)*SHAKE128_RATE;

  /* Handle remaining bytes */
  if(outlen % SHAKE128_RATE)
  {
    keccak_squeeze_4x(tmp0, tmp1, tmp2, tmp3, 1, state, SHAKE128_RATE);
    for(idx = 0; idx < outlen % SHAKE128_RATE; idx++)
    {
      out0[idx] = tmp0[idx];
      out1[idx] = tmp1[idx];
      out2[idx] = tmp2[idx];
      out3[idx] = tmp3[idx];
    }
  }
}

/*************************************************
 * Name:        shake256x4
 *
 * Description: SHAKE256 XOF with 4-way parallel processing
 *
 * Arguments:   - unsigned char *out0..3: output pointers
 *              - unsigned long long outlen: total output length per lane
 *              - unsigned char *in0..3: input message pointers
 *              - unsigned long long inlen: input length (identical for all)
 **************************************************/
void shake256x4(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3, unsigned long long outlen,
                unsigned char *in0,
                unsigned char *in1,
                unsigned char *in2,
                unsigned char *in3, unsigned long long inlen)
{
  __m256i state[25];
  unsigned char tmp0[SHAKE256_RATE];
  unsigned char tmp1[SHAKE256_RATE];
  unsigned char tmp2[SHAKE256_RATE];
  unsigned char tmp3[SHAKE256_RATE];
  size_t idx;

  /* Initialize state to zero */
  for(idx = 0; idx < 25; idx++)
    state[idx] = _mm256_xor_si256(state[idx], state[idx]);

  /* Absorb 4 messages of identical length in parallel */
  keccak_absorb_4x(state, SHAKE256_RATE, in0, in1, in2, in3, inlen, 0x1F);

  /* Squeeze full blocks */
  keccak_squeeze_4x(out0, out1, out2, out3, outlen/SHAKE256_RATE, state, SHAKE256_RATE);

  out0 += (outlen/SHAKE256_RATE)*SHAKE256_RATE;
  out1 += (outlen/SHAKE256_RATE)*SHAKE256_RATE;
  out2 += (outlen/SHAKE256_RATE)*SHAKE256_RATE;
  out3 += (outlen/SHAKE256_RATE)*SHAKE256_RATE;

  /* Handle remaining bytes */
  if(outlen % SHAKE256_RATE)
  {
    keccak_squeeze_4x(tmp0, tmp1, tmp2, tmp3, 1, state, SHAKE256_RATE);
    for(idx = 0; idx < outlen % SHAKE256_RATE; idx++)
    {
      out0[idx] = tmp0[idx];
      out1[idx] = tmp1[idx];
      out2[idx] = tmp2[idx];
      out3[idx] = tmp3[idx];
    }
  }
}
