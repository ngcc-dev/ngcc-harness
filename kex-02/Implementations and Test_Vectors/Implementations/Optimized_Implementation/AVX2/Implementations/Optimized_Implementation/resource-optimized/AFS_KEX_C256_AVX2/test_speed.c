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
#include "KEX_AlgorithmInstance.h"
#include "../../../test/cpucycles.h"
#include "../../../test/speed_print.h"
#include "BWcoding.h"


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

static void benchmark_bwcoding(void)
{
  unsigned int i;
  uint32_t value = 0x12345678u;
  int16_t vec[32];

  for(i = 0; i < 32; i++)
    vec[i] = (int16_t)((97u * i) & 0x0FFFu);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    sink_u64 ^= encode_bw32(value + i);
  }
  print_results("encode_bw32", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    sink_u32 ^= decode_bw32(vec);
  }
  print_results("decode_bw32", t, NTESTS);
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

static void benchmark_kex(void)
{
  unsigned int i;
  unsigned long long pk_len  = kex_get_pk_len_bytes();
  unsigned long long sk_len  = kex_get_sk_len_bytes();
  unsigned long long sta_len = kex_get_sta_len_bytes();
  unsigned long long stb_len = kex_get_stb_len_bytes();
  unsigned long long ss_len  = kex_get_ss_len_bytes();
  unsigned long long msg_len = kex_get_total_msg_len_bytes();
  unsigned long long l;

  /* canonical (valid) state produced by a full handshake */
  uint8_t *pka = malloc(pk_len),  *ska = malloc(sk_len),  *sta = malloc(sta_len);
  uint8_t *pkb = malloc(pk_len),  *skb = malloc(sk_len),  *stb = malloc(stb_len);
  uint8_t *m1 = malloc(msg_len),  *m2 = malloc(msg_len),  *m3 = malloc(msg_len), *m4 = malloc(msg_len);
  /* scratch buffers for timed calls (keep canonical state intact) */
  uint8_t *pks = malloc(pk_len),  *sks = malloc(sk_len),  *sts = malloc(sta_len > stb_len ? sta_len : stb_len);
  uint8_t *ms  = malloc(msg_len), *sss = malloc(ss_len);
  unsigned long long m1_len = 0, m2_len = 0, m3_len = 0, m4_len = 0;

  if(!pka||!ska||!sta||!pkb||!skb||!stb||!m1||!m2||!m3||!m4||!pks||!sks||!sts||!ms||!sss) {
    fprintf(stderr, "ERROR: malloc failed in benchmark_kex\n");
    abort();
  }

  init_benchmark_rng();

  /* one full handshake to obtain valid messages/states */
  kex_init_a(pka, &l, ska, &l, sta, &l);
  kex_init_b(pkb, &l, skb, &l, stb, &l);
  kex_generate_pass1_msg_a(ska, sk_len, pkb, pk_len, sta, &l, m1, &m1_len);
  kex_generate_pass2_msg_b(skb, sk_len, pka, pk_len, m1, m1_len, stb, &l, m2, &m2_len);
  kex_generate_pass3_msg_a(ska, sk_len, pkb, pk_len, m2, m2_len, sta, &l, m3, &m3_len);
  kex_generate_pass4_msg_b(skb, sk_len, pka, pk_len, m3, m3_len, stb, &l, m4, &m4_len);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    kex_init_a(pks, &l, sks, &l, sts, &l);
    sink_u32 ^= pks[0];
  }
  print_results("kex_init_a", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    kex_init_b(pks, &l, sks, &l, sts, &l);
    sink_u32 ^= pks[0];
  }
  print_results("kex_init_b", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    memcpy(sts, sta, sta_len);
    t[i] = cpucycles();
    kex_generate_pass1_msg_a(ska, sk_len, pkb, pk_len, sts, &l, ms, &l);
    sink_u32 ^= ms[0];
  }
  print_results("kex_generate_pass1_msg_a", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    memcpy(sts, stb, stb_len);
    t[i] = cpucycles();
    kex_generate_pass2_msg_b(skb, sk_len, pka, pk_len, m1, m1_len, sts, &l, ms, &l);
    sink_u32 ^= ms[0];
  }
  print_results("kex_generate_pass2_msg_b", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    memcpy(sts, sta, sta_len);
    t[i] = cpucycles();
    kex_generate_pass3_msg_a(ska, sk_len, pkb, pk_len, m2, m2_len, sts, &l, ms, &l);
    sink_u32 ^= ms[0];
  }
  print_results("kex_generate_pass3_msg_a", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    memcpy(sts, stb, stb_len);
    t[i] = cpucycles();
    kex_generate_pass4_msg_b(skb, sk_len, pka, pk_len, m3, m3_len, sts, &l, ms, &l);
    sink_u32 ^= sts[0];
  }
  print_results("kex_generate_pass4_msg_b", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    kex_derive_ss_a(ska, sk_len, pkb, pk_len, m2, m2_len, sta, sta_len, sss, &l);
    sink_u32 ^= sss[0];
  }
  print_results("kex_derive_ss_a", t, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    t[i] = cpucycles();
    kex_derive_ss_b(skb, sk_len, pka, pk_len, m1, m1_len, stb, stb_len, sss, &l);
    sink_u32 ^= sss[0];
  }
  print_results("kex_derive_ss_b", t, NTESTS);

  free(pka); free(ska); free(sta);
  free(pkb); free(skb); free(stb);
  free(m1); free(m2); free(m3); free(m4);
  free(pks); free(sks); free(sts); free(ms); free(sss);
}

int main(void)
{
  printf("instance: AFS_KEX_C256\n");
  printf("implementation: optimized\n\n");

  benchmark_gen_matrix();
  benchmark_poly_noise();
  benchmark_poly_core();
  benchmark_bwcoding();
  benchmark_indcpa();
  benchmark_kem();
  benchmark_kex();

  (void)sink_u64;
  (void)sink_u32;
  return 0;
}
