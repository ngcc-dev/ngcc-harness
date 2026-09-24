#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

#define decode_bw32 ref_decode_bw32
#define encode_bw32 ref_encode_bw32
#include "../../Reference_Implementation/BW_KEM_C256/BWcoding.c"
#undef decode_bw32
#undef encode_bw32

static uint64_t rng_state = 0xbb67ae8584caa73bULL;

static uint32_t next_u32(void)
{
  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return (uint32_t)(rng_state >> 16);
}

static void ref_poly_frommsg(poly *r, const uint8_t msg[KYBER_INDCPA_MSGBYTES])
{
  int i;
  int j;
  uint32_t m;
  uint64_t mh;
  int16_t mask1;
  int16_t mask2;

  for(i = 0; i < KYBER_N / 32; i++) {
    m = (uint32_t)msg[4 * i];
    m |= (uint32_t)msg[4 * i + 1] << 8;
    m |= (uint32_t)msg[4 * i + 2] << 16;
    m |= (uint32_t)msg[4 * i + 3] << 24;
    mh = ref_encode_bw32(m);

    for(j = 0; j < 32; j++) {
      mask1 = (int16_t)-((mh >> (2 * j)) & 1u);
      mask2 = (int16_t)-((mh >> (2 * j + 1)) & 1u);
      r->coeffs[32 * i + j] = (mask1 & 832) + (mask2 & 1664);
    }
  }
}

static void ref_poly_tomsg(uint8_t msg[KYBER_INDCPA_MSGBYTES], const poly *a)
{
  unsigned int i;
  unsigned int j;
  uint32_t t;
  int16_t vec[32];
  int16_t u;
  uint64_t d0;

  for(i = 0; i < KYBER_N / 32; i++) {
    for(j = 0; j < 32; j++) {
      u = a->coeffs[32 * i + j];
      u += (u >> 15) & KYBER_Q;
      d0 = (uint64_t)u << 12;
      d0 += 1664;
      d0 *= 2580335;
      d0 >>= 33;
      vec[31 - j] = (int16_t)(d0 & 0xfff);
    }

    t = ref_decode_bw32(vec);
    msg[4 * i + 0] = (uint8_t)((t >> 0) & 0xffu);
    msg[4 * i + 1] = (uint8_t)((t >> 8) & 0xffu);
    msg[4 * i + 2] = (uint8_t)((t >> 16) & 0xffu);
    msg[4 * i + 3] = (uint8_t)((t >> 24) & 0xffu);
  }
}

static void fill_random_msg(uint8_t msg[KYBER_INDCPA_MSGBYTES])
{
  unsigned int i;

  for(i = 0; i < KYBER_INDCPA_MSGBYTES; i++) {
    msg[i] = (uint8_t)next_u32();
  }
}

static void fill_random_poly(poly *p)
{
  unsigned int i;

  for(i = 0; i < KYBER_N; i++) {
    p->coeffs[i] = (int16_t)(next_u32() % (4u * KYBER_Q)) - (int16_t)(2u * KYBER_Q);
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

static int compare_msg(const uint8_t *got, const uint8_t *want, unsigned int iter)
{
  unsigned int i;

  for(i = 0; i < KYBER_INDCPA_MSGBYTES; i++) {
    if(got[i] != want[i]) {
      fprintf(stderr,
              "poly_tomsg mismatch iter=%u byte=%u got=%u want=%u\n",
              iter,
              i,
              (unsigned int)got[i],
              (unsigned int)want[i]);
      return 0;
    }
  }

  return 1;
}

static int check_frommsg(const uint8_t msg[KYBER_INDCPA_MSGBYTES], unsigned int iter)
{
  poly got;
  poly want;

  poly_frommsg(&got, msg);
  ref_poly_frommsg(&want, msg);
  return compare_poly(&got, &want, "poly_frommsg", iter);
}

static int check_tomsg(const poly *p, unsigned int iter)
{
  uint8_t got[KYBER_INDCPA_MSGBYTES];
  uint8_t want[KYBER_INDCPA_MSGBYTES];

  poly_tomsg(got, p);
  ref_poly_tomsg(want, p);
  return compare_msg(got, want, iter);
}

static int run_frommsg_tests(void)
{
  uint8_t msg[KYBER_INDCPA_MSGBYTES];
  unsigned int i;

  memset(msg, 0, sizeof(msg));
  if(!check_frommsg(msg, 0u)) return 0;

  memset(msg, 0xff, sizeof(msg));
  if(!check_frommsg(msg, 1u)) return 0;

  for(i = 0; i < sizeof(msg); i++) msg[i] = (uint8_t)i;
  if(!check_frommsg(msg, 2u)) return 0;

  for(i = 0; i < sizeof(msg); i++) msg[i] = (uint8_t)((i & 1u) ? 0x55u : 0xaau);
  if(!check_frommsg(msg, 3u)) return 0;

  for(i = 0; i < 10000u; i++) {
    fill_random_msg(msg);
    if(!check_frommsg(msg, i)) return 0;
  }

  return 1;
}

static int run_tomsg_tests(void)
{
  static const int16_t boundary_values[] = {
    -(int16_t)KYBER_Q, -1, 0, 1, (int16_t)(KYBER_Q / 2), (int16_t)(KYBER_Q - 1)
  };
  poly p;
  unsigned int i;
  unsigned int j;

  memset(&p, 0, sizeof(p));
  if(!check_tomsg(&p, 0u)) return 0;

  for(i = 0; i < KYBER_N; i++) p.coeffs[i] = KYBER_Q - 1;
  if(!check_tomsg(&p, 1u)) return 0;

  for(i = 0; i < KYBER_N; i++) p.coeffs[i] = boundary_values[i % (sizeof(boundary_values) / sizeof(boundary_values[0]))];
  if(!check_tomsg(&p, 2u)) return 0;

  for(i = 0; i < KYBER_N; i++) p.coeffs[i] = (int16_t)((i & 1u) ? -1 : KYBER_Q - 1);
  if(!check_tomsg(&p, 3u)) return 0;

  for(i = 0; i < 10000u; i++) {
    fill_random_poly(&p);
    for(j = 0; j < KYBER_N; j += 31) {
      p.coeffs[j] = boundary_values[(i + j) % (sizeof(boundary_values) / sizeof(boundary_values[0]))];
    }
    if(!check_tomsg(&p, i)) return 0;
  }

  return 1;
}

int main(void)
{
  if(!run_frommsg_tests()) return 1;
  if(!run_tomsg_tests()) return 1;

  puts("poly_frommsg/poly_tomsg tests passed.");
  return 0;
}
