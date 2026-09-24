/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include <stdint.h>
#include <string.h>
#include "../SIG_AlgorithmInstance.h"
#include "../params.h"
#include "../poly.h"
#include "../polyvec.h"
#include "../drng.h"
#include "../sample.h"
#include "../symmetric.h"
#include "../endian.h"
#include "../align.h"
#include "cpucycles.h"
#include "speed_print.h"
#include "test_common.h"

#ifndef NTESTS
#define NTESTS 10000
#endif

uint64_t t[NTESTS];

int main(void)
{
  unsigned int i;
  unsigned long long pk_len;
  unsigned long long sk_len;
  unsigned long long sig_len;
  uint8_t pk[BIT_PUBLICKEYBYTES];
  uint8_t sk[BIT_SECRETKEYBYTES];
  uint8_t sm[BIT_SIGNBYTES + BIT_MESSAGEBYTES];
  uint8_t seed[55] = {0};
  uint8_t bit_seed[BIT_SEEDBYTES] = {0};
  uint8_t challenge[BIT_CHALLENGEBYTES] = {0};
  uint16_t nonce;
  poly_matrix_ntt mat_ntt;
  poly a = {0};
  poly b = {0};
  poly c = {0};
  poly_ntt a_ntt = {0};
  poly_ntt b_ntt = {0};
  poly_ntt c_ntt = {0};

  if (init_random_number(&test_rng, seed, sizeof(seed)) != 0) {
    return 1;
  }

  for (i = 0; i < BIT_SEEDBYTES; i++) {
    bit_seed[i] = (uint8_t)i;
  }
  for (i = 0; i < BIT_CHALLENGEBYTES; i++) {
    challenge[i] = (uint8_t)(i + 1);
  }
  nonce = 0;
  poly_sample_S1(&a, bit_seed, &nonce);
  {
    // Inline 10-bit triangular sampling (replaces deleted poly_sample_triangular)
    uint8_t ALIGNED_32 buf[640]; // BYTELEN
    bit_xof256_nonce(buf, sizeof(buf), bit_seed, BIT_SEEDBYTES, nonce);
    nonce = (uint8_t)(nonce + 1U);
    for (unsigned int j = 0; j < BIT_N / 4; j++) {
      const uint8_t *lo = buf + 5 * j;
      const uint8_t *hi = buf + 320 + 5 * j; // BYTELEN/2 = 320
      b.coeffs[4*j+0] = (int16_t)(load16_le(lo) & 0x03FF) - (int16_t)(load16_le(hi) & 0x03FF);
      b.coeffs[4*j+1] = (int16_t)(((lo[1]>>2)|(lo[2]<<6)) & 0x03FF) - (int16_t)(((hi[1]>>2)|(hi[2]<<6)) & 0x03FF);
      b.coeffs[4*j+2] = (int16_t)(((lo[2]>>4)|(lo[3]<<4)) & 0x03FF) - (int16_t)(((hi[2]>>4)|(hi[3]<<4)) & 0x03FF);
      b.coeffs[4*j+3] = (int16_t)(((lo[3]>>6)|(lo[4]<<2)) & 0x03FF) - (int16_t)(((hi[3]>>6)|(hi[4]<<2)) & 0x03FF);
    }
  }
  poly_to_ntt(&a_ntt, &a);
  poly_to_ntt(&b_ntt, &b);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    poly_matrix_expand_ntt(&mat_ntt, bit_seed);
  }
  print_results("polyvec_matrix_expand:", t, NTESTS);

  nonce = 0;
  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    poly_sample_S1(&a, bit_seed, &nonce);
  }
  print_results("poly_uniform_eta:", t, NTESTS);

  nonce = 0;
  for(i = 0; i < NTESTS; ++i) {
    uint8_t ALIGNED_32 buf[640];
    t[i] = cpucycles();
    bit_xof256_nonce(buf, sizeof(buf), bit_seed, BIT_SEEDBYTES, nonce);
    for (unsigned int j = 0; j < BIT_N / 4; j++) {
      const uint8_t *lo = buf + 5 * j;
      const uint8_t *hi = buf + 320 + 5 * j;
      a.coeffs[4*j+0] = (int16_t)(load16_le(lo) & 0x03FF) - (int16_t)(load16_le(hi) & 0x03FF);
      a.coeffs[4*j+1] = (int16_t)(((lo[1]>>2)|(lo[2]<<6)) & 0x03FF) - (int16_t)(((hi[1]>>2)|(hi[2]<<6)) & 0x03FF);
      a.coeffs[4*j+2] = (int16_t)(((lo[2]>>4)|(lo[3]<<4)) & 0x03FF) - (int16_t)(((hi[2]>>4)|(hi[3]<<4)) & 0x03FF);
      a.coeffs[4*j+3] = (int16_t)(((lo[3]>>6)|(lo[4]<<2)) & 0x03FF) - (int16_t)(((hi[3]>>6)|(hi[4]<<2)) & 0x03FF);
    }
    nonce = (uint8_t)(nonce + 1U);
  }
  print_results("poly_uniform_gamma1:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    poly_to_ntt(&a_ntt, &a);
  }
  print_results("poly_ntt:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    poly_from_ntt(&a, &a_ntt);
  }
  print_results("poly_invntt_tomont:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    poly_ntt_basemul_raw(&c_ntt, &a_ntt, &b_ntt);
  }
  print_results("poly_pointwise_raw:", t, NTESTS);

  poly_ntt_basemul_raw(&c_ntt, &a_ntt, &b_ntt);
  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    poly_ntt_montgomery_lift(&c_ntt);
  }
  print_results("poly_montgomery_lift:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    poly_ntt_basemul_raw(&c_ntt, &a_ntt, &b_ntt);
    poly_ntt_montgomery_lift(&c_ntt);
  }
  print_results("poly_pointwise_montgomery:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    poly_challenge(&c, challenge);
  }
  print_results("poly_challenge:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    sig_keygen(pk, &pk_len, sk, &sk_len);
  }
  print_results("Keypair:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    sig_sign(sk, sk_len, sm, BIT_MESSAGEBYTES, sm, &sig_len);
  }
  print_results("Sign:", t, NTESTS);

  for(i = 0; i < NTESTS; ++i) {
    t[i] = cpucycles();
    sig_verify(pk, pk_len, sm, sig_len, sm, BIT_MESSAGEBYTES);
  }
  print_results("Verify:", t, NTESTS);

  return 0;
}
