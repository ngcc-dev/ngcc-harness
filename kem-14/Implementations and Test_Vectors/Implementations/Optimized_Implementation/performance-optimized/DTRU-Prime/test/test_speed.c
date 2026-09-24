#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../api.h"
#include "../params.h"
#include "cpucycles.h"
#include "speed.h"
#include "../avx2_ntt.h"
#include "../avx2_cbd.h"
#include "../cbd.h"
#include "../avx2_poly.h"

#define NTESTS 10000

uint64_t t[NTESTS];

static void benchmark_ntt(const nttpoly_n1087 *input) {
    unsigned int i;
    nttpoly_n1087 tmp;

    for (i = 0; i < NTESTS; i++) {
        memcpy(&tmp, input, sizeof(nttpoly_n1087));
        t[i] = cpucycles();
        poly_ntt(&tmp);
    }
    print_results("ntt (scalar):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        memcpy(&tmp, input, sizeof(nttpoly_n1087));
        t[i] = cpucycles();
        ntt_avx2_intrinsic(tmp.vec);
    }
    print_results("avx_intrinsic_ntt (AVX2):", t, NTESTS);
}

static void benchmark_invntt(const nttpoly_n1087 *input_ntt) {
    unsigned int i;
    nttpoly_n1087 tmp;

    for (i = 0; i < NTESTS; i++) {
        memcpy(&tmp, input_ntt, sizeof(nttpoly_n1087));
        t[i] = cpucycles();
        poly_invntt(&tmp);
    }
    print_results("invntt (scalar):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        memcpy(&tmp, input_ntt, sizeof(nttpoly_n1087));
        t[i] = cpucycles();
        invntt_avx2_intrinsic(tmp.vec);
    }
    print_results("avx_intrinsic_invntt (AVX2):", t, NTESTS);
}

static void benchmark_basemul(const nttpoly_n1087 *a_ntt, const nttpoly_n1087 *b_ntt) {
    unsigned int i;
    nttpoly_n1087 c;

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_basemul(&c, a_ntt, b_ntt);
    }
    print_results("basemul (scalar):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        basemul3x3_avx2_intrinsic(c.vec, a_ntt->vec, b_ntt->vec);
    }
    print_results("avx_intrinsic_basemul (AVX2):", t, NTESTS);
}

static void benchmark_polymul(const poly *a, const poly *b) {
    unsigned int i;
    poly c;

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_radix_ntt_n1087(&c, a, b);
    }
    print_results("polymul (scalar NTT):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_radix_ntt_n1087_q1_intrinsic(&c, a, b);
    }
    print_results("avx_intrinsic_polymul (AVX2 NTT):", t, NTESTS);
}

static void benchmark_poly_add(const poly *a, const poly *b) {
    unsigned int i;
    poly c;

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_add(&c, a, b);
    }
    print_results("poly_add (scalar):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_add_avx2(&c, a, b);
    }
    print_results("poly_add (AVX2):", t, NTESTS);
}

static void benchmark_poly_multi_p(const poly *a) {
    unsigned int i;
    poly b;

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_multi_p(&b, a);
    }
    print_results("poly_multi_p (scalar):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_multi_p_avx2(&b, a);
    }
    print_results("poly_multi_p (AVX2):", t, NTESTS);
}

static void benchmark_poly_fqcsubq(const poly *input) {
    unsigned int i;
    poly a;

    for (i = 0; i < NTESTS; i++) {
        memcpy(&a, input, sizeof(poly));
        t[i] = cpucycles();
        poly_fqcsubq(&a);
    }
    print_results("poly_fqcsubq (scalar):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        memcpy(&a, input, sizeof(poly));
        t[i] = cpucycles();
        poly_fqcsubq_avx2(&a);
    }
    print_results("poly_fqcsubq (AVX2):", t, NTESTS);
}

static void benchmark_cbd(const uint8_t *buf) {
    unsigned int i;
    poly r;

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        cbd1(&r, buf);
    }
    print_results("cbd1 (scalar):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        cbd1_avx2_intrinsic(&r, buf);
    }
    print_results("avx_intrinsic_cbd1 (AVX2):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        cbd1_avx2_fast(&r, buf);
    }
    print_results("avx_fast_cbd1 (AVX2):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        cbd2(&r, buf);
    }
    print_results("cbd2 (scalar):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        cbd2_avx2_intrinsic(&r, buf);
    }
    print_results("avx_intrinsic_cbd2 (AVX2):", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        cbd2_avx2_fast(&r, buf);
    }
    print_results("avx_fast_cbd2 (AVX2):", t, NTESTS);
}

void benchmark_inverse() {
    unsigned char seed[DTRU_SEEDBYTES] = {0};
    unsigned char coins[DTRU_COINBYTES_KEYGEN];

    rand_init(seed, DTRU_SEEDBYTES);
    rand_byts(DTRU_COINBYTES_KEYGEN, coins);
    poly f, finv;

    poly_sample_keygen_f(&f, coins);

    for (int i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_inverse(&finv, &f);
    }
    print_results("poly_inverse: ", t, NTESTS);

    for (int i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        poly_inverse_avx2(&finv, &f);
    }
    print_results("poly_inverse_avx2: ", t, NTESTS);
}

void benchmark_kem()
{
  printf("\n");

  printf("DTRU-%d-%d-KEM\n\n", DTRU_N, DTRU_Q);
  unsigned int i;
  unsigned char k1[DTRU_SHAREDKEYBYTES], k2[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES], sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
  }
  print_results("dtru_kem_keygen: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_keygen_avx2(pk, &pk_byts, sk, &sk_byts);
  }
  print_results("dtru_kem_keygen_avx2: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_enc(pk, pk_byts, k1, &ss_byts1, ct, &ct_byts);
  }
  print_results("dtru_kem_encaps: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_enc_avx2(pk, pk_byts, k1, &ss_byts1, ct, &ct_byts);
  }
  print_results("dtru_kem_encaps_avx2: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_dec(sk, sk_byts, ct, ct_byts, k2, &ss_byts2);
  }
  print_results("dtru_kem_decaps: ", t, NTESTS);

  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_dec_avx2(sk, sk_byts, ct, ct_byts, k2, &ss_byts2);
  }
  print_results("dtru_kem_decaps_avx2: ", t, NTESTS);
}

int main()
{
  //test_speed_kem();

  unsigned char seed[DTRU_SEEDBYTES] = {0};
  unsigned char coins[DTRU_COINBYTES_KEYGEN];

  rand_init(seed, DTRU_SEEDBYTES);
  rand_byts(DTRU_COINBYTES_KEYGEN, coins);

  poly a, b;
  poly_sample_keygen_f(&a, coins);
  poly_sample_keygen_g(&b, coins + DTRU_CBD1_BYTES);

  nttpoly_n1087 ntt_input;
  poly_extend(&ntt_input, &a);

  nttpoly_n1087 a_ntt, b_ntt;
  poly_extend(&a_ntt, &a);
  poly_extend(&b_ntt, &b);

  benchmark_ntt(&ntt_input);
  printf("\n");
  benchmark_invntt(&a_ntt);
  printf("\n");
  benchmark_basemul(&a_ntt, &b_ntt);
  printf("\n");
  benchmark_polymul(&a, &b);
  printf("\n");
  benchmark_cbd(coins);
  printf("\n");
  benchmark_poly_add(&a, &b);
  printf("\n");
  benchmark_poly_multi_p(&a);
  printf("\n");
  benchmark_poly_fqcsubq(&a);
  printf("\n");
  benchmark_inverse();
  printf("\n");
  benchmark_kem();
  return 0;
}
