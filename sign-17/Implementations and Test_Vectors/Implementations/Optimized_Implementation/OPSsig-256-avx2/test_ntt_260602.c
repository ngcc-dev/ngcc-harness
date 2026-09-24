#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "poly.h"
#include "reduce.h"

#define NTESTS 10000

static int32_t center_mod_q(int64_t x) {
  int64_t r = x % Q;
  if(r > (Q/2)) r -= Q;
  if(r < -(Q/2)) r += Q;
  return (int32_t)r;
}

static int equal_mod_q(int32_t a, int32_t b) {
  return center_mod_q((int64_t)a - b) == 0;
}

static void fill_seed(uint8_t seed[SEEDBYTES]) {
  for(unsigned i = 0; i < SEEDBYTES; ++i)
    seed[i] = (uint8_t)(31u * i + 7u);
}

static void poly_from_montgomery(poly *a) {
  for(unsigned i = 0; i < N; ++i)
    a->coeffs[i] = center_mod_q(montgomery_reduce((int64_t)a->coeffs[i]));
}

static void poly_naivemul(poly *c, const poly *a, const poly *b) {
  int64_t r[2 * N] = {0};

  for(unsigned i = 0; i < N; ++i) {
    for(unsigned j = 0; j < N; ++j)
      r[i + j] += (int64_t)a->coeffs[i] * b->coeffs[j];
  }

  for(unsigned i = N; i < 2 * N; ++i)
    r[i - N] -= r[i];

  for(unsigned i = 0; i < N; ++i)
    c->coeffs[i] = center_mod_q(r[i]);
}

static int test_roundtrip(const uint8_t seed[SEEDBYTES]) {
  uint16_t nonce = 0;
  poly a, b;

  for(unsigned i = 0; i < NTESTS; ++i) {
    poly_uniform(&a, seed, nonce++);
    b = a;

    poly_ntt(&b);
    poly_invntt_tomont(&b);
    poly_from_montgomery(&b);

    for(unsigned j = 0; j < N; ++j) {
      if(!equal_mod_q(b.coeffs[j], a.coeffs[j])) {
        fprintf(stderr,
                "ntt/invntt mismatch at test %u coeff %u: got %d expected %d\n",
                i, j, b.coeffs[j], a.coeffs[j]);
        return 1;
      }
    }
  }

  printf("OK: ntt/invntt roundtrip passed (%d tests)\n", NTESTS);
  return 0;
}

static int test_multiplication(const uint8_t seed[SEEDBYTES]) {
  uint16_t nonce = 10000;
  poly a, b, c_ref, c_ntt;

  for(unsigned i = 0; i < NTESTS; ++i) {
    poly_uniform(&a, seed, nonce++);
    poly_uniform(&b, seed, nonce++);

    poly_naivemul(&c_ref, &a, &b);

    c_ntt = a;
    poly_ntt(&c_ntt);
    poly_ntt(&b);
    poly_pointwise_montgomery(&c_ntt, &c_ntt, &b);
    poly_invntt_tomont(&c_ntt);

    for(unsigned j = 0; j < N; ++j) {
      if(!equal_mod_q(c_ntt.coeffs[j], c_ref.coeffs[j])) {
        fprintf(stderr,
                "ntt multiplication mismatch at test %u coeff %u: got %d expected %d\n",
                i, j, c_ntt.coeffs[j], c_ref.coeffs[j]);
        return 1;
      }
    }
  }

  printf("OK: NTT multiplication passed (%d tests)\n", NTESTS);
  return 0;
}

int main(void) {
  uint8_t seed[SEEDBYTES];

  fill_seed(seed);

  if(test_roundtrip(seed))
    return 1;
  if(test_multiplication(seed))
    return 1;

  printf("OK: test_ntt_260602 passed\n");
  return 0;
}
