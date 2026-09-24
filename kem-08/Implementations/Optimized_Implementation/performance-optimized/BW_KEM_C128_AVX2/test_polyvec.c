#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"

static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

static uint32_t next_u32(void)
{
  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return (uint32_t)(rng_state >> 16);
}

static void fill_polyvec(polyvec *v)
{
  unsigned int i;
  unsigned int j;

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_N; j++) {
      v->vec[i].coeffs[j] = (int16_t)(next_u32() % (4u * KYBER_Q)) - (int16_t)(2 * KYBER_Q);
    }
  }
}

static void fill_pattern_polyvec(polyvec *v, int16_t value)
{
  unsigned int i;
  unsigned int j;

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_N; j++) {
      v->vec[i].coeffs[j] = value;
    }
  }
}

static void fill_alternating_polyvec(polyvec *v)
{
  unsigned int i;
  unsigned int j;

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_N; j++) {
      v->vec[i].coeffs[j] = (j & 1u) ? (KYBER_Q - 1) : 0;
    }
  }
}

static void fill_boundary_polyvec(polyvec *v)
{
  static const int16_t values[] = {
    -(int16_t)KYBER_Q, -1, 0, 1, (int16_t)(KYBER_Q / 2), (int16_t)(KYBER_Q - 1)
  };
  unsigned int i;
  unsigned int j;

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_N; j++) {
      v->vec[i].coeffs[j] = values[j % (sizeof(values) / sizeof(values[0]))];
    }
  }
}

static void ref_polyvec_compress(uint8_t r[KYBER_POLYVECCOMPRESSEDBYTES], const polyvec *a)
{
  unsigned int i, j, k;
  uint16_t t[4];

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_N / 4; j++) {
      for(k = 0; k < 4; k++) {
        int16_t u = a->vec[i].coeffs[4 * j + k];
        u += (u >> 15) & KYBER_Q;
        t[k] = (uint16_t)((((u << 10) + KYBER_Q / 2) / KYBER_Q) & 0x3ff);
      }

      r[0] = (uint8_t)(t[0] >> 0);
      r[1] = (uint8_t)((t[0] >> 8) | (t[1] << 2));
      r[2] = (uint8_t)((t[1] >> 6) | (t[2] << 4));
      r[3] = (uint8_t)((t[2] >> 4) | (t[3] << 6));
      r[4] = (uint8_t)(t[3] >> 2);
      r += 5;
    }
  }
}

static void ref_polyvec_decompress(polyvec *r, const uint8_t a[KYBER_POLYVECCOMPRESSEDBYTES])
{
  unsigned int i, j, k;
  uint16_t t[4];

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_N / 4; j++) {
      t[0] = ((uint16_t)a[0] >> 0) | (((uint16_t)a[1] & 0x03u) << 8);
      t[1] = (((uint16_t)a[1] >> 2) | ((uint16_t)a[2] << 6)) & 0x3ff;
      t[2] = (((uint16_t)a[2] >> 4) | ((uint16_t)a[3] << 4)) & 0x3ff;
      t[3] = (((uint16_t)a[3] >> 6) | ((uint16_t)a[4] << 2)) & 0x3ff;
      a += 5;

      for(k = 0; k < 4; k++) {
        r->vec[i].coeffs[4 * j + k] = ((uint32_t)(t[k] & 0x3ff) * KYBER_Q + 512) >> 10;
      }
    }
  }
}

static int compare_bytes(const uint8_t *got, const uint8_t *want, unsigned int len, const char *name, unsigned int iter)
{
  unsigned int i;

  for(i = 0; i < len; i++) {
    if(got[i] != want[i]) {
      fprintf(stderr,
              "%s byte mismatch at iter=%u offset=%u: got=%u want=%u\n",
              name,
              iter,
              i,
              (unsigned int)got[i],
              (unsigned int)want[i]);
      return 0;
    }
  }

  return 1;
}

static int compare_polyvec(const polyvec *got, const polyvec *want, const char *name, unsigned int iter)
{
  unsigned int i;
  unsigned int j;

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_N; j++) {
      if(got->vec[i].coeffs[j] != want->vec[i].coeffs[j]) {
        fprintf(stderr,
                "%s coeff mismatch at iter=%u poly=%u coeff=%u: got=%d want=%d\n",
                name,
                iter,
                i,
                j,
                (int)got->vec[i].coeffs[j],
                (int)want->vec[i].coeffs[j]);
        return 0;
      }
    }
  }

  return 1;
}

static int run_case(const polyvec *input, unsigned int iter)
{
  uint8_t got_bytes[KYBER_POLYVECCOMPRESSEDBYTES];
  uint8_t ref_bytes[KYBER_POLYVECCOMPRESSEDBYTES];
  polyvec got_polyvec;
  polyvec ref_polyvec;

  polyvec_compress(got_bytes, input);
  ref_polyvec_compress(ref_bytes, input);
  if(!compare_bytes(got_bytes, ref_bytes, KYBER_POLYVECCOMPRESSEDBYTES, "compress", iter)) {
    return 0;
  }

  polyvec_decompress(&got_polyvec, got_bytes);
  ref_polyvec_decompress(&ref_polyvec, ref_bytes);
  if(!compare_polyvec(&got_polyvec, &ref_polyvec, "decompress", iter)) {
    return 0;
  }

  return 1;
}

static int run_pattern_tests(void)
{
  polyvec input;

  fill_pattern_polyvec(&input, 0);
  if(!run_case(&input, 0u)) return 0;

  fill_pattern_polyvec(&input, KYBER_Q - 1);
  if(!run_case(&input, 1u)) return 0;

  fill_pattern_polyvec(&input, -1);
  if(!run_case(&input, 2u)) return 0;

  fill_alternating_polyvec(&input);
  if(!run_case(&input, 3u)) return 0;

  fill_boundary_polyvec(&input);
  if(!run_case(&input, 4u)) return 0;

  return 1;
}

static int run_random_tests(unsigned int iterations)
{
  polyvec input;
  unsigned int i;

  for(i = 0; i < iterations; i++) {
    fill_polyvec(&input);
    if(!run_case(&input, i)) {
      return 0;
    }
  }

  return 1;
}

int main(void)
{
  if(!run_pattern_tests()) {
    return 1;
  }

  if(!run_random_tests(10000u)) {
    return 1;
  }

  puts("polyvec compress/decompress tests passed.");
  return 0;
}
