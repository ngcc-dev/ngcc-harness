#include "packing.h"

#ifndef RRLWR_PACKING_USE_AVX2
#define RRLWR_PACKING_USE_AVX2 1
#endif

#ifndef RRLWR_PACKING_DIRECT32
#define RRLWR_PACKING_DIRECT32 1
#endif

#if RRLWR_PACKING_USE_AVX2
void poly_pack_1_32_avx2(unsigned char *b, const int32_t r[RRLWR_N]);
void poly_pack_2_32_avx2(unsigned char *b, const int32_t r[RRLWR_N]);
void poly_pack_3_32_avx2(unsigned char *b, const int32_t r[RRLWR_N]);
void poly_pack_8_32_avx2(unsigned char *b, const int32_t r[RRLWR_N]);
void poly_pack_11_32_avx2(unsigned char *b, const int32_t r[RRLWR_N]);
#if !RRLWR_PACKING_DIRECT32
void poly_pack_2_16_avx2(unsigned char *b, const uint16_t r[RRLWR_N]);
void poly_pack_11_16_avx2(unsigned char *b, const uint16_t r[RRLWR_N]);
#endif
void poly_unpack_1_32_avx2(int32_t r[RRLWR_N], const unsigned char *b);
void poly_unpack_2_32_avx2(int32_t r[RRLWR_N], const unsigned char *b);
void poly_unpack_3_32_avx2(int32_t r[RRLWR_N], const unsigned char *b);
void poly_unpack_8_32_avx2(int32_t r[RRLWR_N], const unsigned char *b);
void poly_unpack_11_32_avx2(int32_t r[RRLWR_N], const unsigned char *b);
void poly_unpack_13_32_avx2(int32_t r[RRLWR_N], const unsigned char *b);
#if !RRLWR_PACKING_DIRECT32
void poly_unpack_2_16_avx2(uint16_t r[RRLWR_N], const unsigned char *b);
void poly_unpack_11_16_avx2(uint16_t r[RRLWR_N], const unsigned char *b);
void poly_unpack_13_16_avx2(uint16_t r[RRLWR_N], const unsigned char *b);
#endif
#endif

#if RRLWR_PACKING_USE_AVX2 && !RRLWR_PACKING_DIRECT32
static void poly_to_u16(uint16_t out[RRLWR_N], const poly *r)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    out[i] = (uint16_t)r->coeffs[i];
  }
}

static void poly_from_u16_signed(poly *r, const uint16_t in[RRLWR_N],
                                 int32_t bitlen)
{
  int32_t signed_max = ((int32_t)1 << (bitlen - 1)) - 1;
  int32_t modulus = (bitlen == 2) ? ((int32_t)1 << RRLWR_PKE_LOGQ) :
                                    ((int32_t)1 << bitlen);

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    int32_t v = in[i];

    if(v > signed_max) {
      v -= modulus;
    }

    r->coeffs[i] = v;
  }
}
#endif

static void poly_pack_1(unsigned char *b, const poly *r)
{
#if RRLWR_PACKING_USE_AVX2
  poly_pack_1_32_avx2(b, r->coeffs);
  return;
#endif

  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    uint32_t c0 = (uint32_t)(-r->coeffs[8 * i + 0]) & 1;
    uint32_t c1 = (uint32_t)(-r->coeffs[8 * i + 1]) & 1;
    uint32_t c2 = (uint32_t)(-r->coeffs[8 * i + 2]) & 1;
    uint32_t c3 = (uint32_t)(-r->coeffs[8 * i + 3]) & 1;
    uint32_t c4 = (uint32_t)(-r->coeffs[8 * i + 4]) & 1;
    uint32_t c5 = (uint32_t)(-r->coeffs[8 * i + 5]) & 1;
    uint32_t c6 = (uint32_t)(-r->coeffs[8 * i + 6]) & 1;
    uint32_t c7 = (uint32_t)(-r->coeffs[8 * i + 7]) & 1;

    b[i] = (unsigned char)(c0 | (c1 << 1) | (c2 << 2) | (c3 << 3) |
                           (c4 << 4) | (c5 << 5) | (c6 << 6) | (c7 << 7));
  }
}

static void poly_pack_2(unsigned char *b, const poly *r)
{
#if RRLWR_PACKING_USE_AVX2
#if RRLWR_PACKING_DIRECT32
  poly_pack_2_32_avx2(b, r->coeffs);
  return;
#else
  uint16_t tmp[RRLWR_N];

  poly_to_u16(tmp, r);
  poly_pack_2_16_avx2(b, tmp);
  return;
#endif
#endif

  for(unsigned int i = 0; i < RRLWR_N / 4; i++) {
    uint32_t c0 = (uint32_t)(1 - r->coeffs[4 * i + 0]) & 0x3;
    uint32_t c1 = (uint32_t)(1 - r->coeffs[4 * i + 1]) & 0x3;
    uint32_t c2 = (uint32_t)(1 - r->coeffs[4 * i + 2]) & 0x3;
    uint32_t c3 = (uint32_t)(1 - r->coeffs[4 * i + 3]) & 0x3;

    b[i] = (unsigned char)(c0 | (c1 << 2) | (c2 << 4) | (c3 << 6));
  }
}

static void poly_pack_3(unsigned char *b, const poly *r)
{
#if RRLWR_PACKING_USE_AVX2
  poly_pack_3_32_avx2(b, r->coeffs);
  return;
#endif

  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    uint32_t c0 = (uint32_t)(3 - r->coeffs[8 * i + 0]) & 0x7;
    uint32_t c1 = (uint32_t)(3 - r->coeffs[8 * i + 1]) & 0x7;
    uint32_t c2 = (uint32_t)(3 - r->coeffs[8 * i + 2]) & 0x7;
    uint32_t c3 = (uint32_t)(3 - r->coeffs[8 * i + 3]) & 0x7;
    uint32_t c4 = (uint32_t)(3 - r->coeffs[8 * i + 4]) & 0x7;
    uint32_t c5 = (uint32_t)(3 - r->coeffs[8 * i + 5]) & 0x7;
    uint32_t c6 = (uint32_t)(3 - r->coeffs[8 * i + 6]) & 0x7;
    uint32_t c7 = (uint32_t)(3 - r->coeffs[8 * i + 7]) & 0x7;
    unsigned int p = 3 * i;

    b[p + 0] = (unsigned char)(c0 | (c1 << 3) | (c2 << 6));
    b[p + 1] = (unsigned char)((c2 >> 2) | (c3 << 1) | (c4 << 4) | (c5 << 7));
    b[p + 2] = (unsigned char)((c5 >> 1) | (c6 << 2) | (c7 << 5));
  }
}

static void poly_pack_8(unsigned char *b, const poly *r)
{
#if RRLWR_PACKING_USE_AVX2
  poly_pack_8_32_avx2(b, r->coeffs);
  return;
#endif

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    b[i] = (unsigned char)((127 - r->coeffs[i]) & 0xff);
  }
}

static void poly_pack_11(unsigned char *b, const poly *r)
{
#if RRLWR_PACKING_USE_AVX2
#if RRLWR_PACKING_DIRECT32
  poly_pack_11_32_avx2(b, r->coeffs);
  return;
#else
  uint16_t tmp[RRLWR_N];

  poly_to_u16(tmp, r);
  poly_pack_11_16_avx2(b, tmp);
  return;
#endif
#endif

  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    uint32_t c0 = (uint32_t)(1023 - r->coeffs[8 * i + 0]) & 0x7ff;
    uint32_t c1 = (uint32_t)(1023 - r->coeffs[8 * i + 1]) & 0x7ff;
    uint32_t c2 = (uint32_t)(1023 - r->coeffs[8 * i + 2]) & 0x7ff;
    uint32_t c3 = (uint32_t)(1023 - r->coeffs[8 * i + 3]) & 0x7ff;
    uint32_t c4 = (uint32_t)(1023 - r->coeffs[8 * i + 4]) & 0x7ff;
    uint32_t c5 = (uint32_t)(1023 - r->coeffs[8 * i + 5]) & 0x7ff;
    uint32_t c6 = (uint32_t)(1023 - r->coeffs[8 * i + 6]) & 0x7ff;
    uint32_t c7 = (uint32_t)(1023 - r->coeffs[8 * i + 7]) & 0x7ff;
    unsigned int p = 11 * i;

    b[p + 0] = (unsigned char)c0;
    b[p + 1] = (unsigned char)((c0 >> 8) | (c1 << 3));
    b[p + 2] = (unsigned char)((c1 >> 5) | (c2 << 6));
    b[p + 3] = (unsigned char)(c2 >> 2);
    b[p + 4] = (unsigned char)((c2 >> 10) | (c3 << 1));
    b[p + 5] = (unsigned char)((c3 >> 7) | (c4 << 4));
    b[p + 6] = (unsigned char)((c4 >> 4) | (c5 << 7));
    b[p + 7] = (unsigned char)(c5 >> 1);
    b[p + 8] = (unsigned char)((c5 >> 9) | (c6 << 2));
    b[p + 9] = (unsigned char)((c6 >> 6) | (c7 << 5));
    b[p + 10] = (unsigned char)(c7 >> 3);
  }
}

/// @brief Pack a polynomial with coefficients in [-2^(bitlen-1), 2^(bitlen-1)-1] into a byte string of bitlen bits per coefficient
void poly_pack(unsigned char *b, poly *r, int32_t bitlen)
{
  if(bitlen == 1) {
    poly_pack_1(b, r);
    return;
  }

  if(bitlen == 2) {
    poly_pack_2(b, r);
    return;
  }

  if(bitlen == 3) {
    poly_pack_3(b, r);
    return;
  }

  if(bitlen == 8) {
    poly_pack_8(b, r);
    return;
  }

  if(bitlen == 11) {
    poly_pack_11(b, r);
    return;
  }

  unsigned int acc_shift = 0; 
  unsigned int bpos = 0;
  uint32_t acc = 0;
  int32_t offset = ((int32_t)1 << (bitlen - 1)) - 1;
  int32_t mask = ((int32_t)1 << bitlen) - 1;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    acc |= ((uint32_t)((offset - r->coeffs[i]) & mask)) << acc_shift;
    acc_shift += bitlen;
    while(acc_shift >= 8) {
      b[bpos++] = (unsigned char)(acc & 0xff);
      acc >>= 8;
      acc_shift -= 8;
    }
  }
}

void ring_pack(unsigned char *b, ring_element *r, int32_t bitlen)
{
  unsigned int offset = bitlen * (RRLWR_N >> 3);

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_pack(b + i * offset, &r->x[i], bitlen);
  }
}

static void poly_unpack_1(poly *r, const unsigned char *b)
{
#if RRLWR_PACKING_USE_AVX2
  poly_unpack_1_32_avx2(r->coeffs, b);
  return;
#endif

  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    uint32_t x = b[i];

    r->coeffs[8 * i + 0] = -(int32_t)(x & 1);
    r->coeffs[8 * i + 1] = -(int32_t)((x >> 1) & 1);
    r->coeffs[8 * i + 2] = -(int32_t)((x >> 2) & 1);
    r->coeffs[8 * i + 3] = -(int32_t)((x >> 3) & 1);
    r->coeffs[8 * i + 4] = -(int32_t)((x >> 4) & 1);
    r->coeffs[8 * i + 5] = -(int32_t)((x >> 5) & 1);
    r->coeffs[8 * i + 6] = -(int32_t)((x >> 6) & 1);
    r->coeffs[8 * i + 7] = -(int32_t)(x >> 7);
  }
}

static void poly_unpack_2(poly *r, const unsigned char *b)
{
#if RRLWR_PACKING_USE_AVX2
#if RRLWR_PACKING_DIRECT32
  poly_unpack_2_32_avx2(r->coeffs, b);
  return;
#else
  uint16_t tmp[RRLWR_N];

  poly_unpack_2_16_avx2(tmp, b);
  poly_from_u16_signed(r, tmp, 2);
  return;
#endif
#endif

  for(unsigned int i = 0; i < RRLWR_N / 4; i++) {
    uint32_t x = b[i];

    r->coeffs[4 * i + 0] = 1 - (int32_t)(x & 0x3);
    r->coeffs[4 * i + 1] = 1 - (int32_t)((x >> 2) & 0x3);
    r->coeffs[4 * i + 2] = 1 - (int32_t)((x >> 4) & 0x3);
    r->coeffs[4 * i + 3] = 1 - (int32_t)(x >> 6);
  }
}

static void poly_unpack_3(poly *r, const unsigned char *b)
{
#if RRLWR_PACKING_USE_AVX2
  poly_unpack_3_32_avx2(r->coeffs, b);
  return;
#endif

  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    const unsigned char *p = b + 3 * i;
    uint32_t c0 = p[0] & 0x7;
    uint32_t c1 = (p[0] >> 3) & 0x7;
    uint32_t c2 = ((p[0] >> 6) | ((uint32_t)p[1] << 2)) & 0x7;
    uint32_t c3 = (p[1] >> 1) & 0x7;
    uint32_t c4 = (p[1] >> 4) & 0x7;
    uint32_t c5 = ((p[1] >> 7) | ((uint32_t)p[2] << 1)) & 0x7;
    uint32_t c6 = (p[2] >> 2) & 0x7;
    uint32_t c7 = p[2] >> 5;

    r->coeffs[8 * i + 0] = 3 - (int32_t)c0;
    r->coeffs[8 * i + 1] = 3 - (int32_t)c1;
    r->coeffs[8 * i + 2] = 3 - (int32_t)c2;
    r->coeffs[8 * i + 3] = 3 - (int32_t)c3;
    r->coeffs[8 * i + 4] = 3 - (int32_t)c4;
    r->coeffs[8 * i + 5] = 3 - (int32_t)c5;
    r->coeffs[8 * i + 6] = 3 - (int32_t)c6;
    r->coeffs[8 * i + 7] = 3 - (int32_t)c7;
  }
}

static void poly_unpack_8(poly *r, const unsigned char *b)
{
#if RRLWR_PACKING_USE_AVX2
  poly_unpack_8_32_avx2(r->coeffs, b);
  return;
#endif

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = 127 - (int32_t)b[i];
  }
}

static void poly_unpack_11(poly *r, const unsigned char *b)
{
#if RRLWR_PACKING_USE_AVX2
#if RRLWR_PACKING_DIRECT32
  poly_unpack_11_32_avx2(r->coeffs, b);
  return;
#else
  uint16_t tmp[RRLWR_N];

  poly_unpack_11_16_avx2(tmp, b);
  poly_from_u16_signed(r, tmp, 11);
  return;
#endif
#endif

  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    const unsigned char *p = b + 11 * i;
    uint32_t c0 = (((uint32_t)p[0]) | ((uint32_t)p[1] << 8)) & 0x7ff;
    uint32_t c1 = (((uint32_t)p[1] >> 3) | ((uint32_t)p[2] << 5)) & 0x7ff;
    uint32_t c2 = (((uint32_t)p[2] >> 6) | ((uint32_t)p[3] << 2) |
                   ((uint32_t)p[4] << 10)) & 0x7ff;
    uint32_t c3 = (((uint32_t)p[4] >> 1) | ((uint32_t)p[5] << 7)) & 0x7ff;
    uint32_t c4 = (((uint32_t)p[5] >> 4) | ((uint32_t)p[6] << 4)) & 0x7ff;
    uint32_t c5 = (((uint32_t)p[6] >> 7) | ((uint32_t)p[7] << 1) |
                   ((uint32_t)p[8] << 9)) & 0x7ff;
    uint32_t c6 = (((uint32_t)p[8] >> 2) | ((uint32_t)p[9] << 6)) & 0x7ff;
    uint32_t c7 = (((uint32_t)p[9] >> 5) | ((uint32_t)p[10] << 3)) & 0x7ff;

    r->coeffs[8 * i + 0] = 1023 - (int32_t)c0;
    r->coeffs[8 * i + 1] = 1023 - (int32_t)c1;
    r->coeffs[8 * i + 2] = 1023 - (int32_t)c2;
    r->coeffs[8 * i + 3] = 1023 - (int32_t)c3;
    r->coeffs[8 * i + 4] = 1023 - (int32_t)c4;
    r->coeffs[8 * i + 5] = 1023 - (int32_t)c5;
    r->coeffs[8 * i + 6] = 1023 - (int32_t)c6;
    r->coeffs[8 * i + 7] = 1023 - (int32_t)c7;
  }
}

static void poly_unpack_13(poly *r, const unsigned char *b)
{
#if RRLWR_PACKING_USE_AVX2
#if RRLWR_PACKING_DIRECT32
  poly_unpack_13_32_avx2(r->coeffs, b);
  return;
#else
  uint16_t tmp[RRLWR_N];

  poly_unpack_13_16_avx2(tmp, b);
  poly_from_u16_signed(r, tmp, 13);
  return;
#endif
#endif

  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    const unsigned char *p = b + 13 * i;
    uint32_t c0 = (((uint32_t)p[0]) | ((uint32_t)p[1] << 8)) & 0x1fff;
    uint32_t c1 = (((uint32_t)p[1] >> 5) | ((uint32_t)p[2] << 3) |
                   ((uint32_t)p[3] << 11)) & 0x1fff;
    uint32_t c2 = (((uint32_t)p[3] >> 2) | ((uint32_t)p[4] << 6)) & 0x1fff;
    uint32_t c3 = (((uint32_t)p[4] >> 7) | ((uint32_t)p[5] << 1) |
                   ((uint32_t)p[6] << 9)) & 0x1fff;
    uint32_t c4 = (((uint32_t)p[6] >> 4) | ((uint32_t)p[7] << 4) |
                   ((uint32_t)p[8] << 12)) & 0x1fff;
    uint32_t c5 = (((uint32_t)p[8] >> 1) | ((uint32_t)p[9] << 7)) & 0x1fff;
    uint32_t c6 = (((uint32_t)p[9] >> 6) | ((uint32_t)p[10] << 2) |
                   ((uint32_t)p[11] << 10)) & 0x1fff;
    uint32_t c7 = (((uint32_t)p[11] >> 3) | ((uint32_t)p[12] << 5)) & 0x1fff;

    r->coeffs[8 * i + 0] = 4095 - (int32_t)c0;
    r->coeffs[8 * i + 1] = 4095 - (int32_t)c1;
    r->coeffs[8 * i + 2] = 4095 - (int32_t)c2;
    r->coeffs[8 * i + 3] = 4095 - (int32_t)c3;
    r->coeffs[8 * i + 4] = 4095 - (int32_t)c4;
    r->coeffs[8 * i + 5] = 4095 - (int32_t)c5;
    r->coeffs[8 * i + 6] = 4095 - (int32_t)c6;
    r->coeffs[8 * i + 7] = 4095 - (int32_t)c7;
  }
}

/// @brief Unpack a polynomial with coefficients in [-2^(bitlen-1), 2^(bitlen-1)-1] from a byte string of bitlen bits per coefficient
void poly_unpack(poly *r, const unsigned char *b, int32_t bitlen)
{
  if(bitlen == 1) {
    poly_unpack_1(r, b);
    return;
  }

  if(bitlen == 2) {
    poly_unpack_2(r, b);
    return;
  }

  if(bitlen == 3) {
    poly_unpack_3(r, b);
    return;
  }

  if(bitlen == 8) {
    poly_unpack_8(r, b);
    return;
  }

  if(bitlen == 11) {
    poly_unpack_11(r, b);
    return;
  }

  if(bitlen == 13) {
    poly_unpack_13(r, b);
    return;
  }

  unsigned int acc_shift = 0; 
  unsigned int bpos = 0;
  uint32_t acc = 0;
  int32_t offset = ((int32_t)1 << (bitlen - 1)) - 1;
  int32_t mask = ((int32_t)1 << bitlen) - 1;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    while(acc_shift < (unsigned int)bitlen) {
      acc |= ((uint32_t)b[bpos++]) << acc_shift;
      acc_shift += 8;
    }

    r->coeffs[i] = offset - (int32_t)(acc & (uint32_t)mask);
    acc >>= bitlen;
    acc_shift -= bitlen;
  }
}

void ring_unpack(ring_element *r, const unsigned char *b, int32_t bitlen)
{
  unsigned int offset = bitlen * (RRLWR_N >> 3);

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_unpack(&r->x[i], b + i * offset, bitlen);
  }
}

void poly_unpack_2_avx2(poly *r, const uint8_t *b)
{
  poly_unpack_2(r, b);
}

void poly_unpack_13_avx2(poly *r, const uint8_t *b)
{
  poly_unpack_13(r, b);
}

void ring_unpack_avx2(ring_element *r, const uint8_t *b, int32_t bitlen)
{
  ring_unpack(r, b, bitlen);
}
