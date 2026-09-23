#include "uniform.h"
#define RRLWR_HASH_DOMAIN_XOF
#define RRLWR_HASH_DOMAIN_XOF_PUBLIC_SINGLE
#define RRLWR_HASH_DOMAIN_XOF_SECRET_SINGLE
#include "hash_domain.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// @brief Generate a polynomial with coefficients pseudo-randomly generated in the uniform distribution [-bitlen/2, bitlen/2-1]
///        The seed, coefficient index, and lane are length-delimited before hashing.
#define RRLWR_MAX_SAMPLING_BITLEN (24)  // Support sampling bit lengths up to 24 bits, only required to define buffer size

// Uniform polynomial generation with 4 independent byte streams
static void poly_uniform_public_x4(poly *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len, unsigned char coeff)
{
  unsigned char xof_bytes_buffer0[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 3)]; // Allocate maximum length
  unsigned char xof_bytes_buffer1[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 3)]; // Allocate maximum length
  unsigned char xof_bytes_buffer2[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 3)]; // Allocate maximum length
  unsigned char xof_bytes_buffer3[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 3)]; // Allocate maximum length

  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    memset(r, 0, sizeof(*r));
    return;
  }

  // Generate the output byte stream
  RRLWR_XOF_PUBLIC_DOMAIN(xof_bytes_buffer0, bitlen*(RRLWR_N >> 5), seed, seed_len, coeff, 0);
  RRLWR_XOF_PUBLIC_DOMAIN(xof_bytes_buffer1, bitlen*(RRLWR_N >> 5), seed, seed_len, coeff, 1);
  RRLWR_XOF_PUBLIC_DOMAIN(xof_bytes_buffer2, bitlen*(RRLWR_N >> 5), seed, seed_len, coeff, 2);
  RRLWR_XOF_PUBLIC_DOMAIN(xof_bytes_buffer3, bitlen*(RRLWR_N >> 5), seed, seed_len, coeff, 3);

  // Unpack into polynomial coefficients
  subpoly_unpack(&r->coeffs[0*(RRLWR_N>>2)], RRLWR_N>>2, xof_bytes_buffer0, bitlen);
  subpoly_unpack(&r->coeffs[1*(RRLWR_N>>2)], RRLWR_N>>2, xof_bytes_buffer1, bitlen);
  subpoly_unpack(&r->coeffs[2*(RRLWR_N>>2)], RRLWR_N>>2, xof_bytes_buffer2, bitlen);
  subpoly_unpack(&r->coeffs[3*(RRLWR_N>>2)], RRLWR_N>>2, xof_bytes_buffer3, bitlen);
}

/// @brief Generate a ring element with coefficients pseudo-randomly generated in the uniform distribution [-bitlen/2, bitlen/2-1]
void ring_uniform_public_x4(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len) {
  for(unsigned char i = 0; i < RRLWR_K; i++) {
    poly_uniform_public_x4(&r->x[i], bitlen, seed, seed_len, i);
  }
}

// Uniform polynomial generation with 4 independent byte streams
static void poly_uniform_secret_x4(poly *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len, unsigned char coeff)
{
  unsigned char xof_bytes_buffer0[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 5)]; // Allocate maximum length
  unsigned char xof_bytes_buffer1[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 5)]; // Allocate maximum length
  unsigned char xof_bytes_buffer2[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 5)]; // Allocate maximum length
  unsigned char xof_bytes_buffer3[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 5)]; // Allocate maximum length

  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    memset(r, 0, sizeof(*r));
    return;
  }

  // Generate the output byte stream
  RRLWR_XOF_SECRET_DOMAIN(xof_bytes_buffer0, bitlen*(RRLWR_N >> 5), seed, seed_len, coeff, 0);
  RRLWR_XOF_SECRET_DOMAIN(xof_bytes_buffer1, bitlen*(RRLWR_N >> 5), seed, seed_len, coeff, 1);
  RRLWR_XOF_SECRET_DOMAIN(xof_bytes_buffer2, bitlen*(RRLWR_N >> 5), seed, seed_len, coeff, 2);
  RRLWR_XOF_SECRET_DOMAIN(xof_bytes_buffer3, bitlen*(RRLWR_N >> 5), seed, seed_len, coeff, 3);

  // Unpack into polynomial coefficients
  subpoly_unpack(&r->coeffs[0*(RRLWR_N>>2)], RRLWR_N>>2, xof_bytes_buffer0, bitlen);
  subpoly_unpack(&r->coeffs[1*(RRLWR_N>>2)], RRLWR_N>>2, xof_bytes_buffer1, bitlen);
  subpoly_unpack(&r->coeffs[2*(RRLWR_N>>2)], RRLWR_N>>2, xof_bytes_buffer2, bitlen);
  subpoly_unpack(&r->coeffs[3*(RRLWR_N>>2)], RRLWR_N>>2, xof_bytes_buffer3, bitlen);
}

/// @brief Generate a ring element with coefficients pseudo-randomly generated in the uniform distribution [-bitlen/2, bitlen/2-1]
void ring_uniform_secret_x4(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len) {
  for(unsigned char i = 0; i < RRLWR_K; i++) {
    poly_uniform_secret_x4(&r->x[i], bitlen, seed, seed_len, i);
  }
}

// Uniform polynomial generation with 4 independent byte streams
static void poly_uniform_secret(poly *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len, unsigned char coeff)
{
  unsigned char xof_bytes_buffer[RRLWR_MAX_SAMPLING_BITLEN*(RRLWR_N >> 3)]; // Allocate maximum length

  if(seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX) {
    memset(r, 0, sizeof(*r));
    return;
  }

  // Generate the output byte stream
  RRLWR_XOF_SECRET_DOMAIN(xof_bytes_buffer, bitlen*(RRLWR_N >> 3), seed, seed_len, coeff, 0);

  // Unpack into polynomial coefficients
  poly_unpack(r, xof_bytes_buffer, bitlen);
}

/// @brief Generate a ring element with coefficients pseudo-randomly generated in the uniform distribution [-bitlen/2, bitlen/2-1]
void ring_uniform_secret(ring_element *r, int32_t bitlen, const unsigned char *seed, int32_t seed_len) {
  for(unsigned char i = 0; i < RRLWR_K; i++) {
    poly_uniform_secret(&r->x[i], bitlen, seed, seed_len, i);
  }
}
