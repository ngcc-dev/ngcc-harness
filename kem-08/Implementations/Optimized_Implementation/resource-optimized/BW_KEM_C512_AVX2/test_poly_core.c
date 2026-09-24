#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "drng.h"
#include "params.h"
#include "poly.h"
#include "reduce.h"

DRNG_ctx drng_algorithm;

static uint64_t rng_state = 0x3c6ef372fe94f82bULL;

static uint32_t next_u32(void)
{
  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return (uint32_t)(rng_state >> 16);
}

static void fill_poly(poly *p)
{
  unsigned int i;

  for(i = 0; i < KYBER_N; i++) {
    p->coeffs[i] = (int16_t)(next_u32() & 0xffffU);
  }
}

static void fill_poly_small(poly *p)
{
  unsigned int i;

  for(i = 0; i < KYBER_N; i++) {
    p->coeffs[i] = (int16_t)(next_u32() % (4U * KYBER_Q)) - (int16_t)(2U * KYBER_Q);
  }
}

static void fill_bytes(uint8_t *buf, unsigned int len)
{
  unsigned int i;

  for(i = 0; i < len; i++) {
    buf[i] = (uint8_t)next_u32();
  }
}

static int16_t ref_montgomery_reduce(int32_t a)
{
  int16_t t;

  t = (int16_t)a * QINV;
  t = (a - (int32_t)t * KYBER_Q) >> 16;
  return t;
}

static int16_t ref_barrett_reduce(int16_t a)
{
  int16_t t;
  const int16_t v = ((1 << 26) + KYBER_Q / 2) / KYBER_Q;

  t = ((int32_t)v * a + (1 << 25)) >> 26;
  t *= KYBER_Q;
  return a - t;
}

static void ref_poly_tobytes(uint8_t r[KYBER_POLYBYTES], const poly *a)
{
  unsigned int i;
  uint16_t t0;
  uint16_t t1;

  for(i = 0; i < KYBER_N / 2; i++) {
    t0 = (uint16_t)a->coeffs[2 * i + 0];
    t0 += ((int16_t)t0 >> 15) & KYBER_Q;
    t1 = (uint16_t)a->coeffs[2 * i + 1];
    t1 += ((int16_t)t1 >> 15) & KYBER_Q;
    r[3 * i + 0] = (uint8_t)(t0 >> 0);
    r[3 * i + 1] = (uint8_t)((t0 >> 8) | (t1 << 4));
    r[3 * i + 2] = (uint8_t)(t1 >> 4);
  }
}

static void ref_poly_frombytes(poly *r, const uint8_t a[KYBER_POLYBYTES])
{
  unsigned int i;

  for(i = 0; i < KYBER_N / 2; i++) {
    r->coeffs[2 * i + 0] = ((a[3 * i + 0] >> 0) | ((uint16_t)a[3 * i + 1] << 8)) & 0xFFF;
    r->coeffs[2 * i + 1] = ((a[3 * i + 1] >> 4) | ((uint16_t)a[3 * i + 2] << 4)) & 0xFFF;
  }
}

static void ref_poly_compress(uint8_t r[KYBER_POLYCOMPRESSEDBYTES], const poly *a)
{
  unsigned int i;
  unsigned int j;
  int16_t u;
  uint8_t t[4];

  for(i = 0; i < KYBER_N / 4; i++) {
    for(j = 0; j < 4; j++) {
      u = a->coeffs[4 * i + j];
      u += (u >> 15) & KYBER_Q;
      t[j] = (uint8_t)(((((uint16_t)u << 6) + KYBER_Q / 2) / KYBER_Q) & 63);
    }

    r[0] = (uint8_t)((t[0] >> 0) | (t[1] << 6));
    r[1] = (uint8_t)((t[1] >> 2) | (t[2] << 4));
    r[2] = (uint8_t)((t[2] >> 4) | (t[3] << 2));
    r += 3;
  }
}

static void ref_poly_decompress(poly *r, const uint8_t a[KYBER_POLYCOMPRESSEDBYTES])
{
  unsigned int i;
  unsigned int j;
  uint8_t t[4];

  for(i = 0; i < KYBER_N / 4; i++) {
    t[0] = (a[0] >> 0);
    t[1] = (a[0] >> 6) | (a[1] << 2);
    t[2] = (a[1] >> 4) | (a[2] << 4);
    t[3] = (a[2] >> 2);
    a += 3;

    for(j = 0; j < 4; j++) {
      r->coeffs[4 * i + j] = ((uint32_t)(t[j] & 0x3f) * KYBER_Q + 32) >> 6;
    }
  }
}

static void ref_poly_reduce(poly *r)
{
  unsigned int i;

  for(i = 0; i < KYBER_N; i++) {
    r->coeffs[i] = ref_barrett_reduce(r->coeffs[i]);
  }
}

static void ref_poly_tomont(poly *r)
{
  unsigned int i;
  const int16_t f = (1ULL << 32) % KYBER_Q;

  for(i = 0; i < KYBER_N; i++) {
    r->coeffs[i] = ref_montgomery_reduce((int32_t)r->coeffs[i] * f);
  }
}

static int compare_bytes(const uint8_t *got, const uint8_t *want, unsigned int len, const char *name, unsigned int iter)
{
  unsigned int i;

  for(i = 0; i < len; i++) {
    if(got[i] != want[i]) {
      fprintf(stderr, "%s byte mismatch iter=%u offset=%u got=%u want=%u\n",
              name, iter, i, (unsigned int)got[i], (unsigned int)want[i]);
      return 0;
    }
  }

  return 1;
}

static int compare_poly(const poly *got, const poly *want, const char *name, unsigned int iter)
{
  unsigned int i;

  for(i = 0; i < KYBER_N; i++) {
    if(got->coeffs[i] != want->coeffs[i]) {
      fprintf(stderr, "%s coeff mismatch iter=%u coeff=%u got=%d want=%d\n",
              name, iter, i, (int)got->coeffs[i], (int)want->coeffs[i]);
      return 0;
    }
  }

  return 1;
}

static int run_polybytes_case(const poly *input, unsigned int iter)
{
  uint8_t got_bytes[KYBER_POLYBYTES];
  uint8_t want_bytes[KYBER_POLYBYTES];
  poly got_poly;
  poly want_poly;

  poly_tobytes(got_bytes, input);
  ref_poly_tobytes(want_bytes, input);
  if(!compare_bytes(got_bytes, want_bytes, sizeof(got_bytes), "poly_tobytes", iter))
    return 0;

  poly_frombytes(&got_poly, got_bytes);
  ref_poly_frombytes(&want_poly, want_bytes);
  return compare_poly(&got_poly, &want_poly, "poly_frombytes", iter);
}

static int run_compress_case(const poly *input, unsigned int iter)
{
  uint8_t got_bytes[KYBER_POLYCOMPRESSEDBYTES];
  uint8_t want_bytes[KYBER_POLYCOMPRESSEDBYTES];
  poly got_poly;
  poly want_poly;

  poly_compress(got_bytes, input);
  ref_poly_compress(want_bytes, input);
  if(!compare_bytes(got_bytes, want_bytes, sizeof(got_bytes), "poly_compress", iter))
    return 0;

  poly_decompress(&got_poly, got_bytes);
  ref_poly_decompress(&want_poly, want_bytes);
  return compare_poly(&got_poly, &want_poly, "poly_decompress", iter);
}

static int run_reduce_case(const poly *input, unsigned int iter)
{
  poly got;
  poly want;

  got = *input;
  want = *input;
  poly_reduce(&got);
  ref_poly_reduce(&want);
  if(!compare_poly(&got, &want, "poly_reduce", iter))
    return 0;

  got = *input;
  want = *input;
  poly_tomont(&got);
  ref_poly_tomont(&want);
  return compare_poly(&got, &want, "poly_tomont", iter);
}

int main(void)
{
  poly p;
  uint8_t bytes[KYBER_POLYBYTES];
  unsigned int i;

  memset(&p, 0, sizeof(p));
  if(!run_polybytes_case(&p, 0u)) return 1;
  if(!run_compress_case(&p, 0u)) return 1;
  if(!run_reduce_case(&p, 0u)) return 1;

  for(i = 0; i < KYBER_N; i++) p.coeffs[i] = KYBER_Q - 1;
  if(!run_polybytes_case(&p, 1u)) return 1;
  if(!run_compress_case(&p, 1u)) return 1;
  if(!run_reduce_case(&p, 1u)) return 1;

  for(i = 0; i < KYBER_N; i++) p.coeffs[i] = (int16_t)((i & 1u) ? -1 : KYBER_Q - 1);
  if(!run_polybytes_case(&p, 2u)) return 1;
  if(!run_compress_case(&p, 2u)) return 1;
  if(!run_reduce_case(&p, 2u)) return 1;

  for(i = 0; i < 10000u; i++) {
    fill_poly_small(&p);
    if(!run_polybytes_case(&p, i)) return 1;
    if(!run_compress_case(&p, i)) return 1;

    fill_poly(&p);
    if(!run_reduce_case(&p, i)) return 1;

    fill_bytes(bytes, sizeof(bytes));
    poly_frombytes(&p, bytes);
    {
      poly want;
      ref_poly_frombytes(&want, bytes);
      if(!compare_poly(&p, &want, "poly_frombytes-random", i)) return 1;
    }
  }

  puts("poly core serialization/compression/reduction tests passed.");
  return 0;
}
