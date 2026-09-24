#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "cbd.h"

static unsigned int ref_get_bit(const uint8_t *buf, size_t bit_index)
{
  return (unsigned int)((buf[bit_index >> 3] >> (bit_index & 7u)) & 1u);
}

static void ref_cbd_eta(poly *r, const uint8_t *buf, unsigned int eta)
{
  unsigned int i;
  unsigned int j;
  size_t bit_offset = 0;

  for(i = 0; i < KYBER_N; i++) {
    int16_t a = 0;
    int16_t b = 0;

    for(j = 0; j < eta; j++) {
      a += (int16_t)ref_get_bit(buf, bit_offset + j);
      b += (int16_t)ref_get_bit(buf, bit_offset + eta + j);
    }

    r->coeffs[i] = a - b;
    bit_offset += (size_t)(2u * eta);
  }
}

static void ref_poly_cbd_eta1(poly *r, const uint8_t buf[KYBER_ETA1 * KYBER_N / 4])
{
  ref_cbd_eta(r, buf, KYBER_ETA1);
}

static void ref_poly_cbd_eta2(poly *r, const uint8_t buf[KYBER_ETA2 * KYBER_N / 4])
{
  ref_cbd_eta(r, buf, KYBER_ETA2);
}

static uint64_t rng_state = 0x123456789abcdef0ULL;

static uint32_t next_u32(void)
{
  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return (uint32_t)(rng_state >> 16);
}

static void fill_random(uint8_t *buf, size_t len)
{
  size_t i;
  for(i = 0; i < len; i++) {
    buf[i] = (uint8_t)next_u32();
  }
}

static int compare_poly(const poly *got, const poly *want, const char *name, unsigned int iter)
{
  unsigned int i;

  for(i = 0; i < KYBER_N; i++) {
    if(got->coeffs[i] != want->coeffs[i]) {
      fprintf(stderr,
              "%s mismatch at iter=%u coeff=%u: got=%d want=%d\n",
              name,
              iter,
              i,
              (int)got->coeffs[i],
              (int)want->coeffs[i]);
      return 0;
    }
  }

  return 1;
}

static int run_eta1_case(const uint8_t buf[KYBER_ETA1 * KYBER_N / 4], unsigned int iter)
{
  poly got;
  poly want;

  poly_cbd_eta1(&got, buf);
  ref_poly_cbd_eta1(&want, buf);
  return compare_poly(&got, &want, "eta1", iter);
}

static int run_eta2_case(const uint8_t buf[KYBER_ETA2 * KYBER_N / 4], unsigned int iter)
{
  poly got;
  poly want;

  poly_cbd_eta2(&got, buf);
  ref_poly_cbd_eta2(&want, buf);
  return compare_poly(&got, &want, "eta2", iter);
}

static int run_pattern_tests(void)
{
  static const uint8_t patterns[] = {0x00, 0xff, 0xaa, 0x55, 0x33, 0xcc};
  uint8_t eta1_buf[KYBER_ETA1 * KYBER_N / 4];
  uint8_t eta2_buf[KYBER_ETA2 * KYBER_N / 4];
  size_t i;

  for(i = 0; i < sizeof(patterns); i++) {
    memset(eta1_buf, patterns[i], sizeof(eta1_buf));
    memset(eta2_buf, patterns[i], sizeof(eta2_buf));
    if(!run_eta1_case(eta1_buf, (unsigned int)i) ||
       !run_eta2_case(eta2_buf, (unsigned int)i)) {
      return 0;
    }
  }

  for(i = 0; i < sizeof(eta1_buf); i++) {
    eta1_buf[i] = (uint8_t)i;
  }
  for(i = 0; i < sizeof(eta2_buf); i++) {
    eta2_buf[i] = (uint8_t)i;
  }

  return run_eta1_case(eta1_buf, 1000u) && run_eta2_case(eta2_buf, 1000u);
}

static int run_random_tests(unsigned int iterations)
{
  uint8_t eta1_buf[KYBER_ETA1 * KYBER_N / 4];
  uint8_t eta2_buf[KYBER_ETA2 * KYBER_N / 4];
  unsigned int i;

  for(i = 0; i < iterations; i++) {
    fill_random(eta1_buf, sizeof(eta1_buf));
    fill_random(eta2_buf, sizeof(eta2_buf));

    if(!run_eta1_case(eta1_buf, i) || !run_eta2_case(eta2_buf, i)) {
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

  puts("CBD eta=5/6 tests passed.");
  return 0;
}
