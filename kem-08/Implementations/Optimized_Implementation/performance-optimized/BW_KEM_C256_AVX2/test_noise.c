#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "drng.h"
#include "params.h"
#include "poly.h"

DRNG_ctx drng_algorithm;

static void fill_bytes(uint8_t *buf, size_t len, uint8_t seed)
{
  size_t i;

  for(i = 0; i < len; i++)
    buf[i] = (uint8_t)(seed + 19u * (uint8_t)i + (uint8_t)(i >> 1));
}

static void assert_poly_equal(const char *name, const poly *a, const poly *b)
{
  unsigned int i;

  for(i = 0; i < KYBER_N; i++) {
    if(a->coeffs[i] != b->coeffs[i]) {
      fprintf(stderr, "%s mismatch at %u: got %d want %d\n", name, i, a->coeffs[i], b->coeffs[i]);
      exit(1);
    }
  }
}

int main(void)
{
  uint8_t seed[KYBER_SYMBYTES];
  poly got[4];
  poly ref[4];
  unsigned int t;

  for(t = 0; t < 1000; t++) {
    fill_bytes(seed, sizeof(seed), (uint8_t)t);
    poly_getnoise_eta1_4x(&got[0], &got[1], &got[2], &got[3], seed, 0);
    poly_getnoise_eta1(&ref[0], seed, 0);
    poly_getnoise_eta1(&ref[1], seed, 1);
    poly_getnoise_eta1(&ref[2], seed, 2);
    poly_getnoise_eta1(&ref[3], seed, 3);
    assert_poly_equal("eta1_4x[0]", &got[0], &ref[0]);
    assert_poly_equal("eta1_4x[1]", &got[1], &ref[1]);
    assert_poly_equal("eta1_4x[2]", &got[2], &ref[2]);
    assert_poly_equal("eta1_4x[3]", &got[3], &ref[3]);

    poly_getnoise_eta2_4x(&got[0], &got[1], &got[2], &got[3], seed, 4);
    poly_getnoise_eta2(&ref[0], seed, 4);
    poly_getnoise_eta2(&ref[1], seed, 5);
    poly_getnoise_eta2(&ref[2], seed, 6);
    poly_getnoise_eta2(&ref[3], seed, 7);
    assert_poly_equal("eta2_4x[0]", &got[0], &ref[0]);
    assert_poly_equal("eta2_4x[1]", &got[1], &ref[1]);
    assert_poly_equal("eta2_4x[2]", &got[2], &ref[2]);
    assert_poly_equal("eta2_4x[3]", &got[3], &ref[3]);
  }

  puts("test_noise: ok");
  return 0;
}
