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
  poly got;
  poly ref;
  unsigned int t;

  for(t = 0; t < 1000; t++) {
    fill_bytes(seed, sizeof(seed), (uint8_t)t);
    poly_getnoise_eta1(&got, seed, (uint8_t)t);
    poly_getnoise_eta1(&ref, seed, (uint8_t)t);
    assert_poly_equal("eta1", &got, &ref);
  }

  puts("test_noise: ok");
  return 0;
}
