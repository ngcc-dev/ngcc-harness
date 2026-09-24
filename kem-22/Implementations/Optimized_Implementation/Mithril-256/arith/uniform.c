#include "uniform.h"
#include "hash_domain.h"

#include <stddef.h>
#include <string.h>

#define RRLWR_MAX_SAMPLING_BITLEN (13)  // Support sampling bit lengths up to 13 bits, only required to define buffer size
#define RRLWR_MAX_QUARTER_OUTLEN  ((RRLWR_MAX_SAMPLING_BITLEN * RRLWR_N) >> 3)
static void poly_uniform_public_x4(poly **r,
                                   int32_t bitlen,
                                   const unsigned char *seed,
                                   int32_t seed_len,
                                   unsigned char coeff,
                                   unsigned int ncoeffs)
{
  const size_t outlen = (size_t)bitlen * (RRLWR_N >> 3);
  uint8_t buf[4 * RRLWR_MAX_QUARTER_OUTLEN];

  for(unsigned int i = 0; i < ncoeffs; i++) {
    RRLWR_XOF_PUBLIC_DOMAIN(buf + i*outlen, outlen, seed, seed_len, coeff + i, 0);
    poly_unpack(r[i], buf + i*outlen, bitlen);
  }
}

static void ring_uniform_public_x4(poly **r, int32_t bitlen, const unsigned char *seed, int32_t seed_len)
{
  unsigned char i;
  for(i = 0; (i + 4) < RRLWR_K; i += 4) { // Sample 4 at a time until only fewer are left
    poly_uniform_public_x4(r + i, bitlen, seed, seed_len, i, 4);
  }
  poly_uniform_public_x4(r + i, bitlen, seed, seed_len, i, RRLWR_K - i);
}

void ring_uniform_Awin_base(ring_element_Awin *aw, int32_t bitlen, const unsigned char *seed, int32_t seed_len) {
  poly *polys[RRLWR_K];

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    polys[i] = &aw->x[RRLWR_K - 1 - i];
  }

  ring_uniform_public_x4(polys, bitlen, seed, seed_len);
}

#define RRLWR_MAX_SAMPLING_SECRET_BITLEN (RRLWR_PKE_LOG_ETA+1)
#define RRLWR_MAX_SECRET_OUTLEN  (((RRLWR_MAX_SAMPLING_SECRET_BITLEN * RRLWR_N) >> 3) * RRLWR_K)
void ring_uniform(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len) {
  const size_t outlen = (size_t)bitlen * (RRLWR_N >> 3);
  uint8_t buf[RRLWR_MAX_SECRET_OUTLEN];

  RRLWR_XOF_SECRET_DOMAIN(buf, RRLWR_K*outlen, seed, seed_len, 0, 0);

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_unpack(&r->x[i], buf + i*outlen, bitlen);
  }
}
