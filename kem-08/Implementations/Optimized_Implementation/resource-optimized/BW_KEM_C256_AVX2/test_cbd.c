#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

#define poly_cbd_eta1 KYBER_NAMESPACE(poly_cbd_eta1)
#define poly_cbd_eta2 KYBER_NAMESPACE(poly_cbd_eta2)
void poly_cbd_eta1(poly *r, const uint8_t buf[KYBER_ETA1 * KYBER_N / 4]);
void poly_cbd_eta2(poly *r, const uint8_t buf[KYBER_ETA2 * KYBER_N / 4]);

static uint32_t load24_littleendian(const uint8_t x[3])
{
  uint32_t r;

  r = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;

  return r;
}

static void ref_cbd3(poly *r, const uint8_t buf[3 * KYBER_N / 4])
{
  unsigned int i;
  unsigned int j;
  uint32_t t;
  uint32_t d;
  int16_t a;
  int16_t b;

  for(i = 0; i < KYBER_N / 4; i++) {
    t = load24_littleendian(buf + 3 * i);
    d = t & 0x00249249;
    d += (t >> 1) & 0x00249249;
    d += (t >> 2) & 0x00249249;

    for(j = 0; j < 4; j++) {
      a = (int16_t)((d >> (6 * j + 0)) & 0x7);
      b = (int16_t)((d >> (6 * j + 3)) & 0x7);
      r->coeffs[4 * i + j] = a - b;
    }
  }
}

static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

static uint32_t next_u32(void)
{
  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return (uint32_t)(rng_state >> 16);
}

static void fill_random(uint8_t *buf, unsigned int len)
{
  unsigned int i;

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
              "%s mismatch iter=%u coeff=%u got=%d want=%d\n",
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

static int check_eta1(const uint8_t buf[KYBER_ETA1 * KYBER_N / 4], unsigned int iter)
{
  poly got;
  poly want;

  poly_cbd_eta1(&got, buf);
  ref_cbd3(&want, buf);

  return compare_poly(&got, &want, "poly_cbd_eta1", iter);
}

static int check_eta2(const uint8_t buf[KYBER_ETA2 * KYBER_N / 4], unsigned int iter)
{
  poly got;
  poly want;

  poly_cbd_eta2(&got, buf);
  ref_cbd3(&want, buf);

  return compare_poly(&got, &want, "poly_cbd_eta2", iter);
}

int main(void)
{
  uint8_t buf1[KYBER_ETA1 * KYBER_N / 4];
  uint8_t buf2[KYBER_ETA2 * KYBER_N / 4];
  unsigned int i;

  memset(buf1, 0, sizeof(buf1));
  memset(buf2, 0, sizeof(buf2));
  if(!check_eta1(buf1, 0u)) return 1;
  if(!check_eta2(buf2, 0u)) return 1;

  memset(buf1, 0xff, sizeof(buf1));
  memset(buf2, 0xff, sizeof(buf2));
  if(!check_eta1(buf1, 1u)) return 1;
  if(!check_eta2(buf2, 1u)) return 1;

  for(i = 0; i < sizeof(buf1); i++) buf1[i] = (uint8_t)i;
  for(i = 0; i < sizeof(buf2); i++) buf2[i] = (uint8_t)(255u - i);
  if(!check_eta1(buf1, 2u)) return 1;
  if(!check_eta2(buf2, 2u)) return 1;

  for(i = 0; i < sizeof(buf1); i++) buf1[i] = (uint8_t)((i & 1u) ? 0x55u : 0xaau);
  for(i = 0; i < sizeof(buf2); i++) buf2[i] = (uint8_t)((i & 1u) ? 0xaau : 0x55u);
  if(!check_eta1(buf1, 3u)) return 1;
  if(!check_eta2(buf2, 3u)) return 1;

  for(i = 0; i < 10000u; i++) {
    fill_random(buf1, sizeof(buf1));
    fill_random(buf2, sizeof(buf2));
    if(!check_eta1(buf1, i)) return 1;
    if(!check_eta2(buf2, i)) return 1;
  }

  puts("CBD eta=3 tests passed.");
  return 0;
}
