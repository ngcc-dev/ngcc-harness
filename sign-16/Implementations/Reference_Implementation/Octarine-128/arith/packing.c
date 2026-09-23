#include "packing.h"

#define POLY_PACK_CASE(BITS) case BITS: poly_pack##BITS(b, r); break
#define SUBPOLY_UNPACK_CASE(BITS, ncoeffs) case BITS: subpoly_unpack##BITS(r, ncoeffs, b); break

static inline uint32_t pack_coeff(int32_t c, unsigned int bitlen) {
  return (((uint32_t)1 << (bitlen-1))-1u-(uint32_t)c) & (((uint32_t)1 << bitlen)-1u);
}

static inline int32_t unpack_coeff(uint32_t c, unsigned int bitlen) {
  return (int32_t)(((uint32_t)1 << (bitlen-1))-1u-c);
}

static inline uint64_t load64le(const unsigned char *b) {
  return (uint64_t)b[0] | ((uint64_t)b[1] << 8) | ((uint64_t)b[2] << 16) | ((uint64_t)b[3] << 24)
       | ((uint64_t)b[4] << 32) | ((uint64_t)b[5] << 40) | ((uint64_t)b[6] << 48) | ((uint64_t)b[7] << 56);
}

static inline uint32_t load24le(const unsigned char *b) {
  return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16);
}

static inline uint64_t load40le(const unsigned char *b) {
  return (uint64_t)b[0] | ((uint64_t)b[1] << 8) | ((uint64_t)b[2] << 16)
       | ((uint64_t)b[3] << 24) | ((uint64_t)b[4] << 32);
}

static inline void store64le(unsigned char *b, uint64_t x) {
  b[0] = (unsigned char)x;
  b[1] = (unsigned char)(x >> 8);
  b[2] = (unsigned char)(x >> 16);
  b[3] = (unsigned char)(x >> 24);
  b[4] = (unsigned char)(x >> 32);
  b[5] = (unsigned char)(x >> 40);
  b[6] = (unsigned char)(x >> 48);
  b[7] = (unsigned char)(x >> 56);
}

static inline void store24le(unsigned char *b, uint32_t x) {
  b[0] = (unsigned char)x;
  b[1] = (unsigned char)(x >> 8);
  b[2] = (unsigned char)(x >> 16);
}

static inline void store40le(unsigned char *b, uint64_t x) {
  b[0] = (unsigned char)x;
  b[1] = (unsigned char)(x >> 8);
  b[2] = (unsigned char)(x >> 16);
  b[3] = (unsigned char)(x >> 24);
  b[4] = (unsigned char)(x >> 32);
}

static void poly_pack_bits(unsigned char *b, const poly *r, unsigned int bitlen) {
  const int32_t *restrict rc = r->coeffs;
  const uint32_t mask = ((uint32_t)1 << bitlen)-1u;
  const uint32_t offset = ((uint32_t)1 << (bitlen-1))-1u;
  uint64_t acc = 0;
  unsigned int acc_shift = 0;
  unsigned int bpos = 0;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    uint32_t t = (offset-(uint32_t)rc[i]) & mask;
    acc |= (uint64_t)t << acc_shift;
    acc_shift += bitlen;
    while(acc_shift >= 8) {
      b[bpos++] = (unsigned char)acc;
      acc >>= 8;
      acc_shift -= 8;
    }
  }
}

#define DEFINE_POLY_PACK_CONST(BITS) \
static void poly_pack##BITS(unsigned char *b, const poly *r) { \
  const int32_t *restrict rc = r->coeffs; \
  const uint32_t mask = ((uint32_t)1 << (BITS))-1u; \
  const uint32_t offset = ((uint32_t)1 << ((BITS)-1))-1u; \
  uint64_t acc = 0; \
  unsigned int acc_shift = 0; \
  unsigned int bpos = 0; \
  for(unsigned int i = 0; i < RRLWR_N; i++) { \
    uint32_t t = (offset-(uint32_t)rc[i]) & mask; \
    acc |= (uint64_t)t << acc_shift; \
    acc_shift += (BITS); \
    while(acc_shift >= 8) { \
      b[bpos++] = (unsigned char)acc; \
      acc >>= 8; \
      acc_shift -= 8; \
    } \
  } \
}

#define DEFINE_POLY_UNPACK_CONST(BITS) \
static void poly_unpack##BITS(poly *r, const unsigned char *b) { \
  int32_t *restrict rc = r->coeffs; \
  const uint32_t mask = ((uint32_t)1 << (BITS))-1u; \
  const uint32_t offset = ((uint32_t)1 << ((BITS)-1))-1u; \
  uint64_t acc = 0; \
  unsigned int acc_shift = 0; \
  unsigned int bpos = 0; \
  for(unsigned int i = 0; i < RRLWR_N; i++) { \
    while(acc_shift < (BITS)) { \
      acc |= (uint64_t)b[bpos++] << acc_shift; \
      acc_shift += 8; \
    } \
    rc[i] = (int32_t)(offset-(uint32_t)(acc & mask)); \
    acc >>= (BITS); \
    acc_shift -= (BITS); \
  } \
}

static void poly_pack2(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 4, j++) {
    uint32_t x0 = pack_coeff(rc[i+0], 2);
    uint32_t x1 = pack_coeff(rc[i+1], 2);
    uint32_t x2 = pack_coeff(rc[i+2], 2);
    uint32_t x3 = pack_coeff(rc[i+3], 2);
    b[j] = (unsigned char)(x0 | (x1 << 2) | (x2 << 4) | (x3 << 6));
  }
}

static void subpoly_unpack2(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 4, j++) {
    uint32_t x = b[j];
    rc[i+0] = unpack_coeff((x >> 0) & 0x03u, 2);
    rc[i+1] = unpack_coeff((x >> 2) & 0x03u, 2);
    rc[i+2] = unpack_coeff((x >> 4) & 0x03u, 2);
    rc[i+3] = unpack_coeff((x >> 6) & 0x03u, 2);
  }
}

static void poly_pack3(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 8, j += 3) {
    uint32_t x =  pack_coeff(rc[i+0], 3);
    x |= pack_coeff(rc[i+1], 3) << 3;
    x |= pack_coeff(rc[i+2], 3) << 6;
    x |= pack_coeff(rc[i+3], 3) << 9;
    x |= pack_coeff(rc[i+4], 3) << 12;
    x |= pack_coeff(rc[i+5], 3) << 15;
    x |= pack_coeff(rc[i+6], 3) << 18;
    x |= pack_coeff(rc[i+7], 3) << 21;
    b[j+0] = (unsigned char)x;
    b[j+1] = (unsigned char)(x >> 8);
    b[j+2] = (unsigned char)(x >> 16);
  }
}

static void subpoly_unpack3(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 8, j += 3) {
    uint32_t x = (uint32_t)b[j+0] | ((uint32_t)b[j+1] << 8) | ((uint32_t)b[j+2] << 16);
    rc[i+0] = unpack_coeff((x >> 0) & 0x07u, 3);
    rc[i+1] = unpack_coeff((x >> 3) & 0x07u, 3);
    rc[i+2] = unpack_coeff((x >> 6) & 0x07u, 3);
    rc[i+3] = unpack_coeff((x >> 9) & 0x07u, 3);
    rc[i+4] = unpack_coeff((x >> 12) & 0x07u, 3);
    rc[i+5] = unpack_coeff((x >> 15) & 0x07u, 3);
    rc[i+6] = unpack_coeff((x >> 18) & 0x07u, 3);
    rc[i+7] = unpack_coeff((x >> 21) & 0x07u, 3);
  }
}

static void poly_pack5(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 8, j += 5) {
    uint64_t x =  (uint64_t)pack_coeff(rc[i+0], 5);
    x |= (uint64_t)pack_coeff(rc[i+1], 5) << 5;
    x |= (uint64_t)pack_coeff(rc[i+2], 5) << 10;
    x |= (uint64_t)pack_coeff(rc[i+3], 5) << 15;
    x |= (uint64_t)pack_coeff(rc[i+4], 5) << 20;
    x |= (uint64_t)pack_coeff(rc[i+5], 5) << 25;
    x |= (uint64_t)pack_coeff(rc[i+6], 5) << 30;
    x |= (uint64_t)pack_coeff(rc[i+7], 5) << 35;
    b[j+0] = (unsigned char)x;
    b[j+1] = (unsigned char)(x >> 8);
    b[j+2] = (unsigned char)(x >> 16);
    b[j+3] = (unsigned char)(x >> 24);
    b[j+4] = (unsigned char)(x >> 32);
  }
}

static void subpoly_unpack5(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 8, j += 5) {
    uint64_t x = (uint64_t)b[j+0] | ((uint64_t)b[j+1] << 8) | ((uint64_t)b[j+2] << 16)
               | ((uint64_t)b[j+3] << 24) | ((uint64_t)b[j+4] << 32);
    rc[i+0] = unpack_coeff((uint32_t)(x >> 0) & 0x1fu, 5);
    rc[i+1] = unpack_coeff((uint32_t)(x >> 5) & 0x1fu, 5);
    rc[i+2] = unpack_coeff((uint32_t)(x >> 10) & 0x1fu, 5);
    rc[i+3] = unpack_coeff((uint32_t)(x >> 15) & 0x1fu, 5);
    rc[i+4] = unpack_coeff((uint32_t)(x >> 20) & 0x1fu, 5);
    rc[i+5] = unpack_coeff((uint32_t)(x >> 25) & 0x1fu, 5);
    rc[i+6] = unpack_coeff((uint32_t)(x >> 30) & 0x1fu, 5);
    rc[i+7] = unpack_coeff((uint32_t)(x >> 35) & 0x1fu, 5);
  }
}

static void poly_pack6(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 4, j += 3) {
    uint32_t x =  pack_coeff(rc[i+0], 6);
    x |= pack_coeff(rc[i+1], 6) << 6;
    x |= pack_coeff(rc[i+2], 6) << 12;
    x |= pack_coeff(rc[i+3], 6) << 18;
    b[j+0] = (unsigned char)x;
    b[j+1] = (unsigned char)(x >> 8);
    b[j+2] = (unsigned char)(x >> 16);
  }
}

static void subpoly_unpack6(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 4, j += 3) {
    uint32_t x = (uint32_t)b[j+0] | ((uint32_t)b[j+1] << 8) | ((uint32_t)b[j+2] << 16);
    rc[i+0] = unpack_coeff((x >> 0) & 0x3fu, 6);
    rc[i+1] = unpack_coeff((x >> 6) & 0x3fu, 6);
    rc[i+2] = unpack_coeff((x >> 12) & 0x3fu, 6);
    rc[i+3] = unpack_coeff((x >> 18) & 0x3fu, 6);
  }
}

static void poly_pack8(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    b[i] = (unsigned char)pack_coeff(rc[i], 8);
  }
}

static void subpoly_unpack8(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0; i < ncoeffs; i++) {
    rc[i] = unpack_coeff(b[i], 8);
  }
}

static void poly_pack9(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 8, j += 9) {
    uint32_t x0 = pack_coeff(rc[i+0], 9);
    uint32_t x1 = pack_coeff(rc[i+1], 9);
    uint32_t x2 = pack_coeff(rc[i+2], 9);
    uint32_t x3 = pack_coeff(rc[i+3], 9);
    uint32_t x4 = pack_coeff(rc[i+4], 9);
    uint32_t x5 = pack_coeff(rc[i+5], 9);
    uint32_t x6 = pack_coeff(rc[i+6], 9);
    uint32_t x7 = pack_coeff(rc[i+7], 9);
    uint64_t lo = (uint64_t)x0 | ((uint64_t)x1 << 9) | ((uint64_t)x2 << 18) | ((uint64_t)x3 << 27)
                | ((uint64_t)x4 << 36) | ((uint64_t)x5 << 45) | ((uint64_t)x6 << 54) | ((uint64_t)(x7 & 1u) << 63);
    store64le(b+j, lo);
    b[j+8] = (unsigned char)(x7 >> 1);
  }
}

static void subpoly_unpack9(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 8, j += 9) {
    uint64_t lo = load64le(b+j);
    uint32_t hi = b[j+8];
    rc[i+0] = unpack_coeff((uint32_t)(lo >> 0) & 0x1ffu, 9);
    rc[i+1] = unpack_coeff((uint32_t)(lo >> 9) & 0x1ffu, 9);
    rc[i+2] = unpack_coeff((uint32_t)(lo >> 18) & 0x1ffu, 9);
    rc[i+3] = unpack_coeff((uint32_t)(lo >> 27) & 0x1ffu, 9);
    rc[i+4] = unpack_coeff((uint32_t)(lo >> 36) & 0x1ffu, 9);
    rc[i+5] = unpack_coeff((uint32_t)(lo >> 45) & 0x1ffu, 9);
    rc[i+6] = unpack_coeff((uint32_t)(lo >> 54) & 0x1ffu, 9);
    rc[i+7] = unpack_coeff((uint32_t)((lo >> 63) | ((uint64_t)hi << 1)) & 0x1ffu, 9);
  }
}

static void poly_pack10(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 4, j += 5) {
    uint64_t x =  (uint64_t)pack_coeff(rc[i+0], 10);
    x |= (uint64_t)pack_coeff(rc[i+1], 10) << 10;
    x |= (uint64_t)pack_coeff(rc[i+2], 10) << 20;
    x |= (uint64_t)pack_coeff(rc[i+3], 10) << 30;
    b[j+0] = (unsigned char)x;
    b[j+1] = (unsigned char)(x >> 8);
    b[j+2] = (unsigned char)(x >> 16);
    b[j+3] = (unsigned char)(x >> 24);
    b[j+4] = (unsigned char)(x >> 32);
  }
}

static void subpoly_unpack10(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 4, j += 5) {
    uint64_t x = (uint64_t)b[j+0] | ((uint64_t)b[j+1] << 8) | ((uint64_t)b[j+2] << 16)
               | ((uint64_t)b[j+3] << 24) | ((uint64_t)b[j+4] << 32);
    rc[i+0] = unpack_coeff((uint32_t)(x >> 0) & 0x3ffu, 10);
    rc[i+1] = unpack_coeff((uint32_t)(x >> 10) & 0x3ffu, 10);
    rc[i+2] = unpack_coeff((uint32_t)(x >> 20) & 0x3ffu, 10);
    rc[i+3] = unpack_coeff((uint32_t)(x >> 30) & 0x3ffu, 10);
  }
}

static void poly_pack11(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 8, j += 11) {
    uint32_t x0 = pack_coeff(rc[i+0], 11);
    uint32_t x1 = pack_coeff(rc[i+1], 11);
    uint32_t x2 = pack_coeff(rc[i+2], 11);
    uint32_t x3 = pack_coeff(rc[i+3], 11);
    uint32_t x4 = pack_coeff(rc[i+4], 11);
    uint32_t x5 = pack_coeff(rc[i+5], 11);
    uint32_t x6 = pack_coeff(rc[i+6], 11);
    uint32_t x7 = pack_coeff(rc[i+7], 11);
    uint64_t lo = (uint64_t)x0 | ((uint64_t)x1 << 11) | ((uint64_t)x2 << 22) | ((uint64_t)x3 << 33)
                | ((uint64_t)x4 << 44) | ((uint64_t)(x5 & 0x1ffu) << 55);
    uint32_t hi = (x5 >> 9) | (x6 << 2) | (x7 << 13);
    store64le(b+j, lo);
    store24le(b+j+8, hi);
  }
}

static void subpoly_unpack11(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 8, j += 11) {
    uint64_t lo = load64le(b+j);
    uint32_t hi = load24le(b+j+8);
    rc[i+0] = unpack_coeff((uint32_t)(lo >> 0) & 0x7ffu, 11);
    rc[i+1] = unpack_coeff((uint32_t)(lo >> 11) & 0x7ffu, 11);
    rc[i+2] = unpack_coeff((uint32_t)(lo >> 22) & 0x7ffu, 11);
    rc[i+3] = unpack_coeff((uint32_t)(lo >> 33) & 0x7ffu, 11);
    rc[i+4] = unpack_coeff((uint32_t)(lo >> 44) & 0x7ffu, 11);
    rc[i+5] = unpack_coeff((uint32_t)((lo >> 55) | ((uint64_t)(hi & 0x03u) << 9)) & 0x7ffu, 11);
    rc[i+6] = unpack_coeff((hi >> 2) & 0x7ffu, 11);
    rc[i+7] = unpack_coeff((hi >> 13) & 0x7ffu, 11);
  }
}

static void poly_pack13(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 8, j += 13) {
    uint32_t x0 = pack_coeff(rc[i+0], 13);
    uint32_t x1 = pack_coeff(rc[i+1], 13);
    uint32_t x2 = pack_coeff(rc[i+2], 13);
    uint32_t x3 = pack_coeff(rc[i+3], 13);
    uint32_t x4 = pack_coeff(rc[i+4], 13);
    uint32_t x5 = pack_coeff(rc[i+5], 13);
    uint32_t x6 = pack_coeff(rc[i+6], 13);
    uint32_t x7 = pack_coeff(rc[i+7], 13);
    uint64_t lo = (uint64_t)x0 | ((uint64_t)x1 << 13) | ((uint64_t)x2 << 26) | ((uint64_t)x3 << 39)
                | ((uint64_t)(x4 & 0xfffu) << 52);
    uint64_t hi = (x4 >> 12) | ((uint64_t)x5 << 1) | ((uint64_t)x6 << 14) | ((uint64_t)x7 << 27);
    store64le(b+j, lo);
    store40le(b+j+8, hi);
  }
}

static void subpoly_unpack13(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 8, j += 13) {
    uint64_t lo = load64le(b+j);
    uint64_t hi = load40le(b+j+8);
    rc[i+0] = unpack_coeff((uint32_t)(lo >> 0) & 0x1fffu, 13);
    rc[i+1] = unpack_coeff((uint32_t)(lo >> 13) & 0x1fffu, 13);
    rc[i+2] = unpack_coeff((uint32_t)(lo >> 26) & 0x1fffu, 13);
    rc[i+3] = unpack_coeff((uint32_t)(lo >> 39) & 0x1fffu, 13);
    rc[i+4] = unpack_coeff((uint32_t)((lo >> 52) | ((hi & 1u) << 12)) & 0x1fffu, 13);
    rc[i+5] = unpack_coeff((uint32_t)(hi >> 1) & 0x1fffu, 13);
    rc[i+6] = unpack_coeff((uint32_t)(hi >> 14) & 0x1fffu, 13);
    rc[i+7] = unpack_coeff((uint32_t)(hi >> 27) & 0x1fffu, 13);
  }
}

static void poly_pack14(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 4, j += 7) {
    uint64_t x =  (uint64_t)pack_coeff(rc[i+0], 14);
    x |= (uint64_t)pack_coeff(rc[i+1], 14) << 14;
    x |= (uint64_t)pack_coeff(rc[i+2], 14) << 28;
    x |= (uint64_t)pack_coeff(rc[i+3], 14) << 42;
    b[j+0] = (unsigned char)x;
    b[j+1] = (unsigned char)(x >> 8);
    b[j+2] = (unsigned char)(x >> 16);
    b[j+3] = (unsigned char)(x >> 24);
    b[j+4] = (unsigned char)(x >> 32);
    b[j+5] = (unsigned char)(x >> 40);
    b[j+6] = (unsigned char)(x >> 48);
  }
}

static void subpoly_unpack14(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 4, j += 7) {
    uint64_t x = (uint64_t)b[j+0] | ((uint64_t)b[j+1] << 8) | ((uint64_t)b[j+2] << 16)
               | ((uint64_t)b[j+3] << 24) | ((uint64_t)b[j+4] << 32) | ((uint64_t)b[j+5] << 40)
               | ((uint64_t)b[j+6] << 48);
    rc[i+0] = unpack_coeff((uint32_t)(x >> 0) & 0x3fffu, 14);
    rc[i+1] = unpack_coeff((uint32_t)(x >> 14) & 0x3fffu, 14);
    rc[i+2] = unpack_coeff((uint32_t)(x >> 28) & 0x3fffu, 14);
    rc[i+3] = unpack_coeff((uint32_t)(x >> 42) & 0x3fffu, 14);
  }
}

static void poly_pack19(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 8, j += 19) {
    uint32_t x0 = pack_coeff(rc[i+0], 19);
    uint32_t x1 = pack_coeff(rc[i+1], 19);
    uint32_t x2 = pack_coeff(rc[i+2], 19);
    uint32_t x3 = pack_coeff(rc[i+3], 19);
    uint32_t x4 = pack_coeff(rc[i+4], 19);
    uint32_t x5 = pack_coeff(rc[i+5], 19);
    uint32_t x6 = pack_coeff(rc[i+6], 19);
    uint32_t x7 = pack_coeff(rc[i+7], 19);
    uint64_t lo = (uint64_t)x0 | ((uint64_t)x1 << 19) | ((uint64_t)x2 << 38) | ((uint64_t)(x3 & 0x7fu) << 57);
    uint64_t mid = (x3 >> 7) | ((uint64_t)x4 << 12) | ((uint64_t)x5 << 31) | ((uint64_t)(x6 & 0x3fffu) << 50);
    uint32_t hi = (x6 >> 14) | (x7 << 5);
    store64le(b+j, lo);
    store64le(b+j+8, mid);
    store24le(b+j+16, hi);
  }
}

static void subpoly_unpack19(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 8, j += 19) {
    uint64_t lo = load64le(b+j);
    uint64_t mid = load64le(b+j+8);
    uint32_t hi = load24le(b+j+16);
    rc[i+0] = unpack_coeff((uint32_t)(lo >> 0) & 0x7ffffu, 19);
    rc[i+1] = unpack_coeff((uint32_t)(lo >> 19) & 0x7ffffu, 19);
    rc[i+2] = unpack_coeff((uint32_t)(lo >> 38) & 0x7ffffu, 19);
    rc[i+3] = unpack_coeff((uint32_t)((lo >> 57) | ((mid & 0xfffu) << 7)) & 0x7ffffu, 19);
    rc[i+4] = unpack_coeff((uint32_t)(mid >> 12) & 0x7ffffu, 19);
    rc[i+5] = unpack_coeff((uint32_t)(mid >> 31) & 0x7ffffu, 19);
    rc[i+6] = unpack_coeff((uint32_t)((mid >> 50) | ((uint64_t)(hi & 0x1fu) << 14)) & 0x7ffffu, 19);
    rc[i+7] = unpack_coeff((hi >> 5) & 0x7ffffu, 19);
  }
}

static void poly_pack20(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 2, j += 5) {
    uint64_t x =  (uint64_t)pack_coeff(rc[i+0], 20);
    x |= (uint64_t)pack_coeff(rc[i+1], 20) << 20;
    b[j+0] = (unsigned char)x;
    b[j+1] = (unsigned char)(x >> 8);
    b[j+2] = (unsigned char)(x >> 16);
    b[j+3] = (unsigned char)(x >> 24);
    b[j+4] = (unsigned char)(x >> 32);
  }
}

static void subpoly_unpack20(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 2, j += 5) {
    uint64_t x = (uint64_t)b[j+0] | ((uint64_t)b[j+1] << 8) | ((uint64_t)b[j+2] << 16)
               | ((uint64_t)b[j+3] << 24) | ((uint64_t)b[j+4] << 32);
    rc[i+0] = unpack_coeff((uint32_t)(x >> 0) & 0xfffffu, 20);
    rc[i+1] = unpack_coeff((uint32_t)(x >> 20) & 0xfffffu, 20);
  }
}

static void poly_pack22(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 4, j += 11) {
    uint32_t x0 = pack_coeff(rc[i+0], 22);
    uint32_t x1 = pack_coeff(rc[i+1], 22);
    uint32_t x2 = pack_coeff(rc[i+2], 22);
    uint32_t x3 = pack_coeff(rc[i+3], 22);
    uint64_t lo = (uint64_t)x0 | ((uint64_t)x1 << 22) | ((uint64_t)(x2 & 0xfffffu) << 44);
    uint32_t hi = (x2 >> 20) | (x3 << 2);
    store64le(b+j, lo);
    store24le(b+j+8, hi);
  }
}

static void subpoly_unpack22(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i += 4, j += 11) {
    uint64_t lo = load64le(b+j);
    uint32_t hi = load24le(b+j+8);
    rc[i+0] = unpack_coeff((uint32_t)(lo >> 0) & 0x3fffffu, 22);
    rc[i+1] = unpack_coeff((uint32_t)(lo >> 22) & 0x3fffffu, 22);
    rc[i+2] = unpack_coeff((uint32_t)((lo >> 44) | ((uint64_t)(hi & 0x03u) << 20)) & 0x3fffffu, 22);
    rc[i+3] = unpack_coeff((hi >> 2) & 0x3fffffu, 22);
  }
}

static void poly_pack24(unsigned char *b, const poly *r) {
  const int32_t *restrict rc = r->coeffs;

  for(unsigned int i = 0, j = 0; i < RRLWR_N; i++, j += 3) {
    uint32_t x = pack_coeff(rc[i], 24);
    b[j+0] = (unsigned char)x;
    b[j+1] = (unsigned char)(x >> 8);
    b[j+2] = (unsigned char)(x >> 16);
  }
}

static void subpoly_unpack24(int32_t *r, unsigned int ncoeffs, const unsigned char *b) {
  int32_t *restrict rc = r;

  for(unsigned int i = 0, j = 0; i < ncoeffs; i++, j += 3) {
    uint32_t x = (uint32_t)b[j+0] | ((uint32_t)b[j+1] << 8) | ((uint32_t)b[j+2] << 16);
    rc[i] = 0x7fffff-(int32_t)x;
  }
}

/// @brief Pack a polynomial with coefficients in [-bitlen/2, bitlen/2-1] into a byte string of bitlen bits per coefficient
void poly_pack(unsigned char *b, poly *r, int32_t bitlen) {
  switch(bitlen) {
    POLY_PACK_CASE(2);
    POLY_PACK_CASE(3);
    POLY_PACK_CASE(5);
    POLY_PACK_CASE(6);
    POLY_PACK_CASE(8);
    POLY_PACK_CASE(9);
    POLY_PACK_CASE(10);
    POLY_PACK_CASE(11);
    POLY_PACK_CASE(13);
    POLY_PACK_CASE(14);
    POLY_PACK_CASE(19);
    POLY_PACK_CASE(20);
    POLY_PACK_CASE(22);
    POLY_PACK_CASE(24);
    default: poly_pack_bits(b, r, (unsigned int)bitlen); break;
  }
}

void ring_pack(unsigned char *b, ring_element *r, int32_t bitlen) {
  unsigned int offset = bitlen*(RRLWR_N>>3);
  for (unsigned int i = 0; i < RRLWR_K; i++) {
    poly_pack(b+i*offset, &r->x[i], bitlen);
  }
}

/// @brief Unpack ncoeffs coefficients into r. ncoeffs must be divisible by 32
static void subpoly_unpack_bits(int32_t *r, unsigned int ncoeffs, const unsigned char *b, unsigned int bitlen) {
  int32_t *restrict rc = r;
  const uint32_t mask = ((uint32_t)1 << bitlen)-1u;
  const uint32_t offset = ((uint32_t)1 << (bitlen-1))-1u;
  uint64_t acc = 0;
  unsigned int acc_shift = 0;
  unsigned int bpos = 0;

  for(unsigned int i = 0; i < ncoeffs; i++) {
    while(acc_shift < bitlen) {
      acc |= (uint64_t)b[bpos++] << acc_shift;
      acc_shift += 8;
    }
    rc[i] = (int32_t)(offset-(uint32_t)(acc & mask));
    acc >>= bitlen;
    acc_shift -= bitlen;
  }
}

/// @brief Unpack ncoeffs coefficients into r. ncoeffs must be divisible by 32
void subpoly_unpack(int32_t *r, unsigned int ncoeffs, const unsigned char *b, unsigned int bitlen) {
  switch(bitlen) {
    SUBPOLY_UNPACK_CASE(2, ncoeffs);
    SUBPOLY_UNPACK_CASE(3, ncoeffs);
    SUBPOLY_UNPACK_CASE(5, ncoeffs);
    SUBPOLY_UNPACK_CASE(6, ncoeffs);
    SUBPOLY_UNPACK_CASE(8, ncoeffs);
    SUBPOLY_UNPACK_CASE(9, ncoeffs);
    SUBPOLY_UNPACK_CASE(10, ncoeffs);
    SUBPOLY_UNPACK_CASE(11, ncoeffs);
    SUBPOLY_UNPACK_CASE(13, ncoeffs);
    SUBPOLY_UNPACK_CASE(14, ncoeffs);
    SUBPOLY_UNPACK_CASE(19, ncoeffs);
    SUBPOLY_UNPACK_CASE(20, ncoeffs);
    SUBPOLY_UNPACK_CASE(22, ncoeffs);
    SUBPOLY_UNPACK_CASE(24, ncoeffs);
    default: subpoly_unpack_bits(r, ncoeffs, b, (unsigned int)bitlen); break;
  }
}

/// @brief Unpack a polynomial with coefficients in [-bitlen/2, bitlen/2-1] from a byte string of bitlen bits per coefficient
void poly_unpack(poly *r, const unsigned char *b, int32_t bitlen) {
  subpoly_unpack(r->coeffs, RRLWR_N, b, bitlen);
}

void ring_unpack(ring_element *r, const unsigned char *b, int32_t bitlen) {
  unsigned int offset = bitlen*(RRLWR_N>>3);
  for (unsigned int i = 0; i < RRLWR_K; i++) {
    poly_unpack(&r->x[i], b+i*offset, bitlen);
  }
}
