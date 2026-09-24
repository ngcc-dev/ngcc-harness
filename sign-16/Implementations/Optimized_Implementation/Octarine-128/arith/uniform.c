#include "uniform.h"
#define RRLWR_HASH_DOMAIN_XOF
#define RRLWR_HASH_DOMAIN_XOF_PUBLIC_SINGLE
#define RRLWR_HASH_DOMAIN_XOF_SECRET_SINGLE
#include "hash_domain.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef RRLWR_SIGN_BREAKDOWN
#include "sign_test.h"
#include "test/cpucycles.h"
#endif

#define RRLWR_MAX_SAMPLING_BITLEN (24)  // Support sampling bit lengths up to 24 bits, only required to define buffer size
#define RRLWR_MAX_POLY_OUTLEN     ((RRLWR_MAX_SAMPLING_BITLEN * RRLWR_N) >> 3)
#define RRLWR_MAX_QUARTER_OUTLEN  ((RRLWR_MAX_SAMPLING_BITLEN * RRLWR_N) >> 5)

static void poly_uniform_public_x4(poly *r,
                                   int32_t bitlen,
                                   const unsigned char *seed,
                                   int32_t seed_len,
                                   unsigned char coeff)
{
  const size_t outlen = (size_t)bitlen * (RRLWR_N >> 5);
  uint8_t buf[4 * RRLWR_MAX_QUARTER_OUTLEN];

  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    memset(r, 0, sizeof(*r));
    return;
  }

  #ifdef RRLWR_SIGN_BREAKDOWN
  uint64_t ts = cpucycles();
  #endif
  RRLWR_XOF_PUBLIC_DOMAIN(buf + 0 * outlen, outlen, seed, seed_len, coeff, 0);
  RRLWR_XOF_PUBLIC_DOMAIN(buf + 1 * outlen, outlen, seed, seed_len, coeff, 1);
  RRLWR_XOF_PUBLIC_DOMAIN(buf + 2 * outlen, outlen, seed, seed_len, coeff, 2);
  RRLWR_XOF_PUBLIC_DOMAIN(buf + 3 * outlen, outlen, seed, seed_len, coeff, 3);
  #ifdef RRLWR_SIGN_BREAKDOWN
  if(sign_test_measure_awin_base) {
    sign_test_add_cycles(SIGN_TEST_KEYGEN_AWIN_BASE_XOF, ts);
  }
  ts = cpucycles();
  #endif

  poly_unpack(r, buf, bitlen);

  #ifdef RRLWR_SIGN_BREAKDOWN
  if(sign_test_measure_awin_base) {
    sign_test_add_cycles(SIGN_TEST_KEYGEN_AWIN_BASE_UNPACK, ts);
  }
  #endif
}

void ring_uniform_public_x4(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len)
{
  for(unsigned char i = 0; i < RRLWR_K; i++) {
    poly_uniform_public_x4(&r->x[i], bitlen, seed, seed_len, i);
  }
}

static void poly_uniform_secret_x4(poly *r,
                                   int32_t bitlen,
                                   const unsigned char *seed,
                                   int32_t seed_len,
                                   unsigned char coeff)
{
  const size_t outlen = (size_t)bitlen * (RRLWR_N >> 5);
  uint8_t buf[4 * RRLWR_MAX_QUARTER_OUTLEN];

  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    memset(r, 0, sizeof(*r));
    return;
  }

  RRLWR_XOF_SECRET_DOMAIN(buf + 0 * outlen, outlen, seed, seed_len, coeff, 0);
  RRLWR_XOF_SECRET_DOMAIN(buf + 1 * outlen, outlen, seed, seed_len, coeff, 1);
  RRLWR_XOF_SECRET_DOMAIN(buf + 2 * outlen, outlen, seed, seed_len, coeff, 2);
  RRLWR_XOF_SECRET_DOMAIN(buf + 3 * outlen, outlen, seed, seed_len, coeff, 3);

  poly_unpack(r, buf, bitlen);
}

void ring_uniform_secret_x4(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len)
{
  for(unsigned char i = 0; i < RRLWR_K; i++) {
    poly_uniform_secret_x4(&r->x[i], bitlen, seed, seed_len, i);
  }
}

static void poly_uniform_secret(poly *r,
                                int32_t bitlen,
                                const unsigned char *seed,
                                int32_t seed_len,
                                unsigned char coeff)
{
  unsigned char xof_bytes_buffer[RRLWR_MAX_POLY_OUTLEN];

  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    memset(r, 0, sizeof(*r));
    return;
  }

  RRLWR_XOF_SECRET_DOMAIN(xof_bytes_buffer, bitlen * (RRLWR_N >> 3), seed, seed_len, coeff, 0);
  poly_unpack(r, xof_bytes_buffer, bitlen);
}

void ring_uniform_secret(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len)
{
  for(unsigned char i = 0; i < RRLWR_K; i++) {
    poly_uniform_secret(&r->x[i], bitlen, seed, seed_len, i);
  }
}

void ring_uniform_Awin_base(ring_element_Awin *aw, int32_t bitlen, const unsigned char *seed, int32_t seed_len)
{
  for(unsigned char i = 0; i < RRLWR_K; i++) {
    poly_uniform_public_x4(&aw->x[RRLWR_K - 1 - i], bitlen, seed, seed_len, i);
  }
}
