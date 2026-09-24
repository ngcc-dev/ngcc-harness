#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "kem.h"
#include "indcpa.h"
#include "poly.h"
#include "polyvec.h"
#include "drng.h"
#include "randombytes.h"
#include "../../../test/cpucycles.h"
#include "../../../test/speed_print.h"


#define NTESTS 1000
#define SEED_LEN_BYTES 64

static uint64_t t[NTESTS];
DRNG_ctx drng_algorithm;
static volatile uint64_t sink_u64;
static volatile uint32_t sink_u32;

static void fill_bytes(uint8_t *buf, size_t len, uint8_t seed)
{
  size_t i;

  for(i = 0; i < len; i++)
    buf[i] = (uint8_t)(seed + 17u * (uint8_t)i + (uint8_t)(i >> 1));
}

static void fill_poly(poly *p, uint16_t seed)
{
  unsigned int i;

  for(i = 0; i < KYBER_N; i++)
    p->coeffs[i] = (int16_t)((seed + 13u * i) % KYBER_Q);
}

static void fill_polyvec(polyvec *v, uint16_t seed)
{
  unsigned int i;

  for(i = 0; i < KYBER_K; i++)
    fill_poly(&v->vec[i], (uint16_t)(seed + 31u * i));
}

static void init_benchmark_rng(void)
{
  uint8_t seed[SEED_LEN_BYTES];

  fill_bytes(seed, sizeof(seed), 0x42);
  if(init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) {
    fprintf(stderr, "ERROR: init_random_number failed in test_speed.c\n");
    abort();
  }
}

static void benchmark_gen_matrix(void)
{
  unsigned int i;
  uint8_t seed[KYBER_SYMBYTES];
  polyvec matrix[KYBER_K];

  fill_bytes(seed, sizeof(seed), 0x11);
  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    gen_matrix(matrix, seed, 0);
  }
  print_results("gen_matrix", t, NTESTS);
}

static void benchmark_poly_noise(void)
{
  unsigned int i;
  uint8_t seed[KYBER_SYMBYTES];
  poly p;

  fill_bytes(seed, sizeof(seed), 0x22);
  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    poly_getnoise_eta1(&p, seed, (uint8_t)i);
  }
  print_results("poly_getnoise_eta1", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    poly_getnoise_eta2(&p, seed, (uint8_t)(i + 1));
  }
  print_results("poly_getnoise_eta2", t, NTESTS);
}

static void benchmark_poly_core(void)
{
  unsigned int i;
  poly src;
  poly tmp;
  poly out;
  polyvec va;
  polyvec vb;
  uint8_t msg[KYBER_INDCPA_MSGBYTES];
  uint8_t polybytes[KYBER_POLYBYTES];
  uint8_t vecbytes[KYBER_POLYVECCOMPRESSEDBYTES];

  fill_poly(&src, 5);
  fill_bytes(msg, sizeof(msg), 0x33);
  fill_polyvec(&va, 23);
  fill_polyvec(&vb, 71);
  polyvec_ntt(&va);
  polyvec_ntt(&vb);

  for(i = 0; i < NTESTS; i++) {
    tmp = src;
    t[i] = cpucycles();
    poly_ntt(&tmp);
  }
  print_results("poly_ntt", t, NTESTS);

  tmp = src;
  poly_ntt(&tmp);
  for(i = 0; i < NTESTS; i++) {
    poly out_poly = tmp;
    t[i] = cpucycles();
    poly_invntt_tomont(&out_poly);
    sink_u32 ^= (uint32_t)out_poly.coeffs[0];
  }
  print_results("poly_invntt_tomont", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    polyvec_basemul_acc_montgomery(&out, &va, &vb);
    sink_u32 ^= (uint32_t)out.coeffs[0];
  }
  print_results("polyvec_basemul_acc_montgomery", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    poly_tomsg(msg, &src);
  }
  print_results("poly_tomsg", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    poly_frommsg(&tmp, msg);
    sink_u32 ^= (uint32_t)tmp.coeffs[0];
  }
  print_results("poly_frommsg", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    poly_tobytes(polybytes, &src);
  }
  print_results("poly_tobytes", t, NTESTS);

  poly_tobytes(polybytes, &src);
  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    poly_frombytes(&tmp, polybytes);
    sink_u32 ^= (uint32_t)tmp.coeffs[1];
  }
  print_results("poly_frombytes", t, NTESTS);

  fill_polyvec(&va, 123);
  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    polyvec_compress(vecbytes, &va);
  }
  print_results("polyvec_compress", t, NTESTS);

  polyvec_compress(vecbytes, &va);
  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    polyvec_decompress(&vb, vecbytes);
    sink_u32 ^= (uint32_t)vb.vec[0].coeffs[0];
  }
  print_results("polyvec_decompress", t, NTESTS);
}


static void benchmark_indcpa(void)
{
  unsigned int i;
  uint8_t coins[KYBER_SYMBYTES];
  uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES];
  uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES];
  uint8_t ct[KYBER_INDCPA_BYTES];
  uint8_t msg[KYBER_INDCPA_MSGBYTES];

  fill_bytes(coins, sizeof(coins), 0x44);
  fill_bytes(msg, sizeof(msg), 0x55);
  indcpa_keypair_derand(pk, sk, coins);
  indcpa_enc(ct, msg, pk, coins);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    indcpa_keypair_derand(pk, sk, coins);
    sink_u32 ^= pk[0];
  }
  print_results("indcpa_keypair_derand", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    indcpa_enc(ct, msg, pk, coins);
    sink_u32 ^= ct[0];
  }
  print_results("indcpa_enc", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    indcpa_dec(msg, ct, sk);
    sink_u32 ^= msg[0];
  }
  print_results("indcpa_dec", t, NTESTS);
}

static void benchmark_kem(void)
{
  unsigned int i;
  uint8_t keypair_coins[2 * KYBER_SYMBYTES];
  uint8_t encaps_coins[KYBER_SYMBYTES];
  uint8_t pk[CRYPTO_PUBLICKEYBYTES];
  uint8_t sk[CRYPTO_SECRETKEYBYTES];
  uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
  uint8_t ss[CRYPTO_BYTES];

  fill_bytes(keypair_coins, sizeof(keypair_coins), 0x66);
  fill_bytes(encaps_coins, sizeof(encaps_coins), 0x77);
  init_benchmark_rng();
  crypto_kem_keypair_derand(pk, sk, keypair_coins);
  crypto_kem_enc_derand(ct, ss, pk, encaps_coins);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    crypto_kem_keypair(pk, sk);
    sink_u32 ^= pk[0];
  }
  print_results("crypto_kem_keypair", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    crypto_kem_enc(ct, ss, pk);
    sink_u32 ^= ct[0];
  }
  print_results("crypto_kem_enc", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    crypto_kem_dec(ss, ct, sk);
    sink_u32 ^= ss[0];
  }
  print_results("crypto_kem_dec", t, NTESTS);
}

int main(void)
{
  printf("instance: BW_KEM_C128\n");
  printf("implementation: optimized\n\n");

  benchmark_gen_matrix();
  benchmark_poly_noise();
  benchmark_poly_core();
  benchmark_indcpa();
  benchmark_kem();

  (void)sink_u64;
  (void)sink_u32;
  return 0;
}
