/*
 *  SPDX-License-Identifier: MIT
 *
 *  owf.c - one-way function and witness extension.
 *
 *  OWF circuit:
 *    v_1 = S_1(k  XOR  c_0)
 *    v_2 = S_2(L_0(k)  XOR  c_1)
 *    y   = (L_1(v_1) * L_2(v_2)) XOR L_3(k) XOR L_3(v_1) XOR L_3(v_2)
 *
 *  Witness:  w = k || v_1 || v_2   [3n bits]
 */

#include <string.h>

#include "owf.h"
#include "fields.h"
#include "instances.h"
#include "lynx_matrices.h"

/* ================================================================
   Helper: bit parity of a 64-bit word
   ================================================================ */
static inline int parity_u64(uint64_t x) {
  x ^= x >> 32;
  x ^= x >> 16;
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return (int)(x & 1);
}

/* ================================================================
   Accessor macro for the 6 uint64_t words of bf384_t
   (handles both HAVE_ATTR_VECTOR_SIZE and plain struct layouts)
   ================================================================ */
#if defined(HAVE_ATTR_VECTOR_SIZE)
#define BF384_WORD(x, i) BF_VALUE((x).inner[(i) / 2], (i) % 2)
#else
#define BF384_WORD(x, i) BF_VALUE(x, i)
#endif

/* ================================================================
   Squaring: sq(x) = x * x in the respective field
   ================================================================ */
static inline bf128_t bf128_sq(bf128_t x) {
  return bf128_mul(x, x);
}
static inline bf160_t bf160_sq(bf160_t x) {
  return bf160_mul(x, x);
}
static inline bf192_t bf192_sq(bf192_t x) {
  return bf192_mul(x, x);
}
static inline bf256_t bf256_sq(bf256_t x) {
  return bf256_mul(x, x);
}
static inline bf384_t bf384_sq(bf384_t x) {
  return bf384_mul(x, x);
}

static inline bf512_t bf512_sq(bf512_t x) {
  return bf512_mul(x, x);
}

static inline bf256_t bf256_inv_allones_exp(bf256_t x) {
  bf256_t result = bf256_one();
  for (int i = 255; i >= 0; --i) {
    result = bf256_sq(result);
    if (i != 0) {
      result = bf256_mul(result, x);
    }
  }
  return result;
}

static inline bf384_t bf384_inv_allones_exp(bf384_t x) {
  bf384_t result = bf384_one();
  for (int i = 383; i >= 0; --i) {
    result = bf384_sq(result);
    if (i != 0) {
      result = bf384_mul(result, x);
    }
  }
  return result;
}

static inline bf512_t bf512_inv_allones_exp(bf512_t x) {
  bf512_t result = bf512_one();
  for (int i = 511; i >= 0; --i) {
    result = bf512_sq(result);
    if (i != 0) {
      result = bf512_mul(result, x);
    }
  }
  return result;
}

/* ================================================================
   GF(2) matrix-vector multiplication
   M is stored row-major: M[i][k] = k-th 64-bit word of row i
   output bit i = XOR of bits of (M[i][k] AND x[k]) across all k
   ================================================================ */

static bf128_t lynx_L128(const uint64_t M[128][2], bf128_t x) {
  const uint64_t x0 = BF_VALUE(x, 0);
  const uint64_t x1 = BF_VALUE(x, 1);
  uint64_t y0 = 0;
  uint64_t y1 = 0;
  bf128_t res = bf128_zero();

  for (unsigned int i = 0; i < 64; ++i) {
    const uint64_t dot = (M[i][0] & x0) ^ (M[i][1] & x1);
    y0 |= ((uint64_t)parity_u64(dot)) << i;
  }
  for (unsigned int i = 64; i < 128; ++i) {
    const uint64_t dot = (M[i][0] & x0) ^ (M[i][1] & x1);
    y1 |= ((uint64_t)parity_u64(dot)) << (i - 64);
  }
  BF_VALUE(res, 0) = y0;
  BF_VALUE(res, 1) = y1;
  return res;
}

static bf160_t lynx_L160(const uint64_t M[160][3], bf160_t x) {
  bf160_t res = bf160_zero();
  uint64_t out = 0;
  unsigned int out_word = 0;
  for (unsigned int i = 0; i < 160; i++) {
    const uint64_t dot =
        (M[i][0] & BF_VALUE(x, 0)) ^ (M[i][1] & BF_VALUE(x, 1)) ^ (M[i][2] & BF_VALUE(x, 2));
    out |= ((uint64_t)parity_u64(dot)) << (i % 64);
    if ((i % 64) == 63 || i == 159) {
      BF_VALUE(res, out_word++) = out;
      out = 0;
    }
  }
  return res;
}

static bf192_t lynx_L192(const uint64_t M[192][3], bf192_t x) {
  bf192_t res = bf192_zero();
  uint64_t out = 0;
  unsigned int out_word = 0;
  for (unsigned int i = 0; i < 192; i++) {
    const uint64_t dot =
        (M[i][0] & BF_VALUE(x, 0)) ^ (M[i][1] & BF_VALUE(x, 1)) ^ (M[i][2] & BF_VALUE(x, 2));
    out |= ((uint64_t)parity_u64(dot)) << (i % 64);
    if ((i % 64) == 63) {
      BF_VALUE(res, out_word++) = out;
      out = 0;
    }
  }
  return res;
}

static bf256_t lynx_L256(const uint64_t M[256][4], bf256_t x) {
  bf256_t res = bf256_zero();
  uint64_t out = 0;
  unsigned int out_word = 0;
  for (unsigned int i = 0; i < 256; i++) {
    const uint64_t dot = (M[i][0] & BF_VALUE(x, 0)) ^ (M[i][1] & BF_VALUE(x, 1)) ^
                         (M[i][2] & BF_VALUE(x, 2)) ^ (M[i][3] & BF_VALUE(x, 3));
    out |= ((uint64_t)parity_u64(dot)) << (i % 64);
    if ((i % 64) == 63) {
      BF_VALUE(res, out_word++) = out;
      out = 0;
    }
  }
  return res;
}

static bf384_t lynx_L384(const uint64_t M[384][6], bf384_t x) {
  bf384_t res = bf384_zero();
  uint64_t out = 0;
  for (unsigned int i = 0; i < 384; i++) {
    const uint64_t dot = (M[i][0] & BF384_WORD(x, 0)) ^ (M[i][1] & BF384_WORD(x, 1)) ^
                         (M[i][2] & BF384_WORD(x, 2)) ^ (M[i][3] & BF384_WORD(x, 3)) ^
                         (M[i][4] & BF384_WORD(x, 4)) ^ (M[i][5] & BF384_WORD(x, 5));
    out |= ((uint64_t)parity_u64(dot)) << (i % 64);
    if ((i % 64) == 63) {
      BF384_WORD(res, i / 64) = out;
      out = 0;
    }
  }
  return res;
}

static bf512_t lynx_L512(const uint64_t M[512][8], bf512_t x) {
  bf512_t res = bf512_zero();
  uint64_t out = 0;
  for (unsigned int i = 0; i < 512; i++) {
    uint64_t dot = 0;
    for (unsigned int j = 0; j < 8; ++j) {
      dot ^= (M[i][j] & BF_VALUE(x, j));
    }
    out |= ((uint64_t)parity_u64(dot)) << (i % 64);
    if ((i % 64) == 63) {
      BF_VALUE(res, i / 64) = out;
      out = 0;
    }
  }
  return res;
}

/* ================================================================
   S-box power maps
   ================================================================ */

/*
 * n=128: S_1 = S_2 = x^{-1} = x^{2^128 - 2}
 *
 * Itoh-Tsujii algorithm:
 *   r_k = x^{2^k - 1} built via tower doubling
 *   x^{-1} = (x^{2^127 - 1})^2
 */
static bf128_t bf128_inv_itoh(bf128_t x) {
  bf128_t t;

  /* Tower: r_k = x^{2^k - 1} for k = 2, 4, 8, 16, 32, 64 */
  bf128_t r1  = x;
  bf128_t r2  = bf128_mul(bf128_sq(r1), r1);       /* x^{2^2 - 1} */
  t           = bf128_sq(bf128_sq(r2));
  bf128_t r4  = bf128_mul(t, r2);                   /* x^{2^4 - 1} */
  t = r4; for (int i = 0; i < 4; i++) t = bf128_sq(t);
  bf128_t r8  = bf128_mul(t, r4);                   /* x^{2^8 - 1} */
  t = r8; for (int i = 0; i < 8; i++) t = bf128_sq(t);
  bf128_t r16 = bf128_mul(t, r8);                   /* x^{2^16 - 1} */
  t = r16; for (int i = 0; i < 16; i++) t = bf128_sq(t);
  bf128_t r32 = bf128_mul(t, r16);                  /* x^{2^32 - 1} */
  t = r32; for (int i = 0; i < 32; i++) t = bf128_sq(t);
  bf128_t r64 = bf128_mul(t, r32);                  /* x^{2^64 - 1} */

  /* Build up r127 = x^{2^127 - 1}
   * 127 = 64+63, 63 = 32+31, 31 = 16+15, 15 = 8+7, 7 = 4+3, 3 = 2+1 */
  bf128_t r3  = bf128_mul(bf128_sq(r2), r1);        /* x^{2^3 - 1} */
  t = r4; for (int i = 0; i < 3; i++) t = bf128_sq(t);
  bf128_t r7  = bf128_mul(t, r3);                   /* x^{2^7 - 1} */
  t = r8; for (int i = 0; i < 7; i++) t = bf128_sq(t);
  bf128_t r15 = bf128_mul(t, r7);                   /* x^{2^15 - 1} */
  t = r16; for (int i = 0; i < 15; i++) t = bf128_sq(t);
  bf128_t r31 = bf128_mul(t, r15);                  /* x^{2^31 - 1} */
  t = r32; for (int i = 0; i < 31; i++) t = bf128_sq(t);
  bf128_t r63 = bf128_mul(t, r31);                  /* x^{2^63 - 1} */
  t = r64; for (int i = 0; i < 63; i++) t = bf128_sq(t);
  bf128_t r127 = bf128_mul(t, r63);                 /* x^{2^127 - 1} */

  return bf128_sq(r127);                            /* x^{2^128 - 2} = x^{-1} */
}

static bf160_t bf160_inv_allones_exp(bf160_t x) {
  bf160_t result = bf160_one();
  for (int i = 159; i >= 0; --i) {
    result = bf160_sq(result);
    if (i != 0) {
      result = bf160_mul(result, x);
    }
  }
  return result;
}

static bf160_t bf160_pow_67_minus_1(bf160_t x) {
  bf160_t t;
  bf160_t r1  = x;
  bf160_t r2  = bf160_mul(bf160_sq(r1), r1);
  t           = bf160_sq(bf160_sq(r2));
  bf160_t r4  = bf160_mul(t, r2);
  t           = r4;
  for (int i = 0; i < 4; i++) {
    t = bf160_sq(t);
  }
  bf160_t r8  = bf160_mul(t, r4);
  t           = r8;
  for (int i = 0; i < 8; i++) {
    t = bf160_sq(t);
  }
  bf160_t r16 = bf160_mul(t, r8);
  t           = r16;
  for (int i = 0; i < 16; i++) {
    t = bf160_sq(t);
  }
  bf160_t r32 = bf160_mul(t, r16);
  t           = r32;
  for (int i = 0; i < 32; i++) {
    t = bf160_sq(t);
  }
  bf160_t r64 = bf160_mul(t, r32);
  bf160_t r3  = bf160_mul(bf160_sq(r2), r1);
  t           = r64;
  for (int i = 0; i < 3; i++) {
    t = bf160_sq(t);
  }
  return bf160_mul(t, r3);
}

static inline bf160_t bf160_s1(bf160_t x) {
  return bf160_inv_allones_exp(x);
}

static inline bf160_t bf160_s2(bf160_t x) {
  return bf160_pow_67_minus_1(x);
}

/*
 * n=192 helper: x^{2^59 - 1}
 *
 * Build using: x^{2^59-1} = r32^{2^27} * r27
 *              r27 = r16^{2^11} * r11
 *              r11 = r8^{2^3}   * r3
 *              r3  = r2^{2^1}   * r1
 */
static bf192_t bf192_pow_59_minus_1(bf192_t x) {
  bf192_t t;

  bf192_t r1  = x;
  bf192_t r2  = bf192_mul(bf192_sq(r1), r1);        /* x^{2^2 - 1} */
  t           = bf192_sq(bf192_sq(r2));
  bf192_t r4  = bf192_mul(t, r2);                   /* x^{2^4 - 1} */
  t = r4; for (int i = 0; i < 4; i++) t = bf192_sq(t);
  bf192_t r8  = bf192_mul(t, r4);                   /* x^{2^8 - 1} */
  t = r8; for (int i = 0; i < 8; i++) t = bf192_sq(t);
  bf192_t r16 = bf192_mul(t, r8);                   /* x^{2^16 - 1} */
  t = r16; for (int i = 0; i < 16; i++) t = bf192_sq(t);
  bf192_t r32 = bf192_mul(t, r16);                  /* x^{2^32 - 1} */

  bf192_t r3  = bf192_mul(bf192_sq(r2), r1);        /* x^{2^3 - 1} */
  t = r8; for (int i = 0; i < 3; i++) t = bf192_sq(t);
  bf192_t r11 = bf192_mul(t, r3);                   /* x^{2^11 - 1} */
  t = r16; for (int i = 0; i < 11; i++) t = bf192_sq(t);
  bf192_t r27 = bf192_mul(t, r11);                  /* x^{2^27 - 1} */
  t = r32; for (int i = 0; i < 27; i++) t = bf192_sq(t);
  return bf192_mul(t, r27);                         /* x^{2^59 - 1} */
}

/*
 * n=192 helper: x^e where e = (2^13 - 1)^{-1} mod (2^192 - 1)
 * e (little-endian 64-bit words):
 *   {0xb6ddb6edb76dbb6d, 0x6dbb6ddb6edb76db, 0xdb76dbb6ddb6edb7}
 * Left-to-right binary exponentiation.
 */
static bf192_t bf192_inv_13_minus_1(bf192_t x) {
  static const uint64_t e[3] = {
      UINT64_C(0xb6ddb6edb76dbb6d),
      UINT64_C(0x6dbb6ddb6edb76db),
      UINT64_C(0xdb76dbb6ddb6edb7),
  };
  bf192_t result = bf192_one();
  for (int i = 191; i >= 0; i--) {
    result = bf192_sq(result);
    if ((e[i / 64] >> (i % 64)) & 1) {
      result = bf192_mul(result, x);
    }
  }
  return result;
}

static inline bf192_t bf192_s1(bf192_t x) {
  return bf192_inv_13_minus_1(x);
}

static inline bf192_t bf192_s2(bf192_t x) {
  return bf192_pow_59_minus_1(x);
}

/*
 * n=256 helper: x^{2^53 - 1}
 */
static bf256_t bf256_pow_53_minus_1(bf256_t x) {
  bf256_t t;

  bf256_t r1  = x;
  bf256_t r2  = bf256_mul(bf256_sq(r1), r1);        /* x^{2^2 - 1} */
  t           = bf256_sq(bf256_sq(r2));
  bf256_t r4  = bf256_mul(t, r2);                   /* x^{2^4 - 1} */
  t = r4; for (int i = 0; i < 4; i++) t = bf256_sq(t);
  bf256_t r8  = bf256_mul(t, r4);                   /* x^{2^8 - 1} */
  t = r8; for (int i = 0; i < 8; i++) t = bf256_sq(t);
  bf256_t r16 = bf256_mul(t, r8);                   /* x^{2^16 - 1} */
  t = r16; for (int i = 0; i < 16; i++) t = bf256_sq(t);
  bf256_t r32 = bf256_mul(t, r16);                  /* x^{2^32 - 1} */

  t          = bf256_sq(r4);
  bf256_t r5 = bf256_mul(t, r1);                    /* x^{2^5 - 1} */
  t = r16; for (int i = 0; i < 5; i++) t = bf256_sq(t);
  bf256_t r21 = bf256_mul(t, r5);                   /* x^{2^21 - 1} */

  t = r32; for (int i = 0; i < 21; i++) t = bf256_sq(t);
  return bf256_mul(t, r21);                         /* x^{2^53 - 1} */
}

/*
 * n=256 helper: x^e where e = (2^5 - 1)^{-1} mod (2^256 - 1)
 * e (little-endian 64-bit words):
 *   {0xdef7bdef7bdef7bd, 0xbdef7bdef7bdef7b, 0x7bdef7bdef7bdef7, 0xf7bdef7bdef7bdef}
 */
static bf256_t bf256_inv_5_minus_1(bf256_t x) {
  static const uint64_t e[4] = {
      UINT64_C(0xdef7bdef7bdef7bd),
      UINT64_C(0xbdef7bdef7bdef7b),
      UINT64_C(0x7bdef7bdef7bdef7),
      UINT64_C(0xf7bdef7bdef7bdef),
  };
  bf256_t result = bf256_one();
  for (int i = 255; i >= 0; i--) {
    result = bf256_sq(result);
    if ((e[i / 64] >> (i % 64)) & 1) {
      result = bf256_mul(result, x);
    }
  }
  return result;
}

static inline bf256_t bf256_s1(bf256_t x) {
  return bf256_inv_5_minus_1(x);
}

static inline bf256_t bf256_s2(bf256_t x) {
  return bf256_pow_53_minus_1(x);
}

/*
 * n=384 helper: x^{2^60 - 2^30 + 1}
 *
 * Factored as: x * (x^{2^30 - 1})^{2^30}
 * r30 = x^{2^30-1}: 30 = 16 + 14, 14 = 8 + 6, 6 = 4 + 2
 */
static bf384_t bf384_pow_60_minus_30_plus_1(bf384_t x) {
  bf384_t t;

  bf384_t r1  = x;
  bf384_t r2  = bf384_mul(bf384_sq(r1), r1);
  t           = bf384_sq(bf384_sq(r2));
  bf384_t r4  = bf384_mul(t, r2);
  t = r4; for (int i = 0; i < 4; i++) t = bf384_sq(t);
  bf384_t r8  = bf384_mul(t, r4);
  t = r8; for (int i = 0; i < 8; i++) t = bf384_sq(t);
  bf384_t r16 = bf384_mul(t, r8);

  bf384_t r6  = bf384_mul(bf384_sq(bf384_sq(r4)), r2); /* x^{2^6 - 1} */
  t = r8; for (int i = 0; i < 6; i++) t = bf384_sq(t);
  bf384_t r14 = bf384_mul(t, r6);                      /* x^{2^14 - 1} */
  t = r16; for (int i = 0; i < 14; i++) t = bf384_sq(t);
  bf384_t r30 = bf384_mul(t, r14);                     /* x^{2^30 - 1} */

  t = r30; for (int i = 0; i < 30; i++) t = bf384_sq(t);
  return bf384_mul(t, x);                              /* x^{2^60 - 2^30 + 1} */
}

/*
 * n=384 helper: x^e where e = (2^13 - 1)^{-1} mod (2^384 - 1)
 * e (little-endian 64-bit words):
 *   {0xf7dfbefdf7efbf7d, 0xefbf7dfbefdf7efb, 0xdf7efbf7dfbefdf7,
 *    0xbefdf7efbf7dfbef, 0x7dfbefdf7efbf7df, 0xfbf7dfbefdf7efbf}
 */
static bf384_t bf384_inv_13_minus_1(bf384_t x) {
  static const uint64_t e[6] = {
      UINT64_C(0xf7dfbefdf7efbf7d),
      UINT64_C(0xefbf7dfbefdf7efb),
      UINT64_C(0xdf7efbf7dfbefdf7),
      UINT64_C(0xbefdf7efbf7dfbef),
      UINT64_C(0x7dfbefdf7efbf7df),
      UINT64_C(0xfbf7dfbefdf7efbf),
  };
  bf384_t result = bf384_one();
  for (int i = 383; i >= 0; i--) {
    result = bf384_sq(result);
    if ((e[i / 64] >> (i % 64)) & 1) {
      result = bf384_mul(result, x);
    }
  }
  return result;
}

static inline bf384_t bf384_s1(bf384_t x) {
  return bf384_inv_13_minus_1(x);
}

static inline bf384_t bf384_s2(bf384_t x) {
  return bf384_pow_60_minus_30_plus_1(x);
}

/*
 * n=512 helper: x^{2^180 - 2^90 + 1}
 *
 * Factored as: x * (x^{2^90 - 1})^{2^90}
 * r90 = x^{2^90-1}: 90 = 64 + 26, 26 = 16 + 10, 10 = 8 + 2
 */
static bf512_t bf512_pow_180_minus_90_plus_1(bf512_t x) {
  bf512_t t;

  bf512_t r1  = x;
  bf512_t r2  = bf512_mul(bf512_sq(r1), r1);
  t           = bf512_sq(bf512_sq(r2));
  bf512_t r4  = bf512_mul(t, r2);
  t = r4; for (int i = 0; i < 4; i++) t = bf512_sq(t);
  bf512_t r8  = bf512_mul(t, r4);
  t = r8; for (int i = 0; i < 8; i++) t = bf512_sq(t);
  bf512_t r16 = bf512_mul(t, r8);
  t = r16; for (int i = 0; i < 16; i++) t = bf512_sq(t);
  bf512_t r32 = bf512_mul(t, r16);
  t = r32; for (int i = 0; i < 32; i++) t = bf512_sq(t);
  bf512_t r64 = bf512_mul(t, r32);

  bf512_t r10 = bf512_mul(bf512_sq(bf512_sq(r8)), r2); /* x^{2^10 - 1} */
  t = r16; for (int i = 0; i < 10; i++) t = bf512_sq(t);
  bf512_t r26 = bf512_mul(t, r10);                     /* x^{2^26 - 1} */
  t = r64; for (int i = 0; i < 26; i++) t = bf512_sq(t);
  bf512_t r90 = bf512_mul(t, r26);                     /* x^{2^90 - 1} */

  t = r90; for (int i = 0; i < 90; i++) t = bf512_sq(t);
  return bf512_mul(t, x);                              /* x^{2^180 - 2^90 + 1} */
}

/*
 * n=512 helper: x^e where e = (2^12 - 2^6 + 1)^(-1) mod (2^512 - 1)
 * e (little-endian 64-bit words):
 *   {0xc9898d9d9c9898d9, 0x898d9d9c9898d9d9, 0x8d9d9c9898d9d9c9, 0x9d9c9898d9d9c989,
 *    0x9c9898d9d9c9898d, 0x9898d9d9c9898d9d, 0x98d9d9c9898d9d9c, 0xd9d9c9898d9d9c98}
 */
static bf512_t bf512_inv_12_minus_6_plus_1(bf512_t x) {
  static const uint64_t e[8] = {
      UINT64_C(0xc9898d9d9c9898d9),
      UINT64_C(0x898d9d9c9898d9d9),
      UINT64_C(0x8d9d9c9898d9d9c9),
      UINT64_C(0x9d9c9898d9d9c989),
      UINT64_C(0x9c9898d9d9c9898d),
      UINT64_C(0x9898d9d9c9898d9d),
      UINT64_C(0x98d9d9c9898d9d9c),
      UINT64_C(0xd9d9c9898d9d9c98),
  };
  bf512_t result = bf512_one();
  for (int i = 511; i >= 0; i--) {
    result = bf512_sq(result);
    if ((e[i / 64] >> (i % 64)) & UINT64_C(1)) {
      result = bf512_mul(result, x);
    }
  }
  return result;
}

static inline bf512_t bf512_s1(bf512_t x) {
  return bf512_inv_12_minus_6_plus_1(x);
}

static inline bf512_t bf512_s2(bf512_t x) {
  return bf512_pow_180_minus_90_plus_1(x);
}

/* ================================================================
   LYNX OWF evaluation

   Signature: owf_lynx_N(key, input, output)
     key    = n-bit secret key (n/8 bytes)
     input  = ignored (OWF input size is 0 for all LYNX variants)
     output = n-bit OWF output (n/8 bytes)
   ================================================================ */

void owf_lynx_128(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  lynx_t mats;
  if (lynx_init(&mats, 128, input, 16) != 0) {
    memset(output, 0, 16);
    return;
  }
  const uint64_t (*L0)[2] = (const uint64_t (*)[2])mats.L0;
  const uint64_t (*L1)[2] = (const uint64_t (*)[2])mats.L1;
  const uint64_t (*L2)[2] = (const uint64_t (*)[2])mats.L2;
  const uint64_t (*L3)[2] = (const uint64_t (*)[2])mats.L3;

  bf128_t k  = bf128_load(key);
  bf128_t c0 = bf128_load(mats.C0);
  bf128_t c1 = bf128_load(mats.C1);

  bf128_t v1 = bf128_inv_itoh(bf128_add(k, c0));

  bf128_t L0k = lynx_L128(L0, k);
  bf128_t v2  = bf128_inv_itoh(bf128_add(L0k, c1));

  {
    const bf128_t L1v1 = lynx_L128(L1, v1);
    const bf128_t L2v2 = lynx_L128(L2, v2);
    const bf128_t L3sum = lynx_L128(L3, bf128_add(k, bf128_add(v1, v2)));
    const bf128_t y = bf128_add(bf128_mul(L1v1, L2v2), L3sum);

    bf128_store(output, y);
  }
}

void owf_lynx_160(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  lynx_t mats;
  if (lynx_init(&mats, 160, input, 20) != 0) {
    memset(output, 0, 20);
    return;
  }
  const uint64_t (*L0)[3] = (const uint64_t (*)[3])mats.L0;
  const uint64_t (*L1)[3] = (const uint64_t (*)[3])mats.L1;
  const uint64_t (*L2)[3] = (const uint64_t (*)[3])mats.L2;
  const uint64_t (*L3)[3] = (const uint64_t (*)[3])mats.L3;

  bf160_t k  = bf160_load(key);
  bf160_t c0 = bf160_load(mats.C0);

  bf160_t v1 = bf160_s1(bf160_add(k, c0));
  bf160_t L0k = lynx_L160(L0, k);
  bf160_t v2  = bf160_s2(bf160_add(L0k, c0));
  bf160_t L1v1 = lynx_L160(L1, v1);
  bf160_t L2v2 = lynx_L160(L2, v2);
  bf160_t L3sum = lynx_L160(L3, bf160_add(k, bf160_add(v1, v2)));
  bf160_t y = bf160_add(bf160_mul(L1v1, L2v2), L3sum);

  bf160_store(output, y);
}

void owf_lynx_192(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  lynx_t mats;
  if (lynx_init(&mats, 192, input, 24) != 0) {
    memset(output, 0, 24);
    return;
  }
  const uint64_t (*L0)[3] = (const uint64_t (*)[3])mats.L0;
  const uint64_t (*L1)[3] = (const uint64_t (*)[3])mats.L1;
  const uint64_t (*L2)[3] = (const uint64_t (*)[3])mats.L2;
  const uint64_t (*L3)[3] = (const uint64_t (*)[3])mats.L3;

  bf192_t k  = bf192_load(key);
  bf192_t c0 = bf192_load(mats.C0);
  bf192_t c1 = bf192_load(mats.C1);

  bf192_t v1 = bf192_s1(bf192_add(k, c0));

  bf192_t L0k = lynx_L192(L0, k);
  bf192_t v2  = bf192_s2(bf192_add(L0k, c1));

  bf192_t L1v1 = lynx_L192(L1, v1);
  bf192_t L2v2 = lynx_L192(L2, v2);
  bf192_t L3sum = lynx_L192(L3, bf192_add(k, bf192_add(v1, v2)));
  bf192_t y = bf192_add(bf192_mul(L1v1, L2v2), L3sum);

  bf192_store(output, y);
}

void owf_lynx_256(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  lynx_t mats;
  if (lynx_init(&mats, 256, input, 32) != 0) {
    memset(output, 0, 32);
    return;
  }
  const uint64_t (*L0)[4] = (const uint64_t (*)[4])mats.L0;
  const uint64_t (*L1)[4] = (const uint64_t (*)[4])mats.L1;
  const uint64_t (*L2)[4] = (const uint64_t (*)[4])mats.L2;
  const uint64_t (*L3)[4] = (const uint64_t (*)[4])mats.L3;

  bf256_t k  = bf256_load(key);
  bf256_t c0 = bf256_load(mats.C0);
  bf256_t c1 = bf256_load(mats.C1);
  bf256_t c2 = bf256_load(mats.C2);

  bf256_t v1 = bf256_s1(bf256_add(k, c0));

  bf256_t L0k = lynx_L256(L0, k);
  bf256_t v2  = bf256_s2(bf256_add(L0k, c1));

  bf256_t L1v1 = lynx_L256(L1, v1);
  bf256_t L2v2 = lynx_L256(L2, v2);
  bf256_t L3sum = lynx_L256(L3, bf256_add(k, bf256_add(v1, v2)));
  bf256_t invkc2 = bf256_inv_allones_exp(bf256_add(k, c2));
  bf256_t y = bf256_add(bf256_mul(invkc2, bf256_mul(L1v1, L2v2)), L3sum);

  bf256_store(output, y);
}

void owf_lynx_384(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  lynx_t mats;
  if (lynx_init(&mats, 384, input, 48) != 0) {
    memset(output, 0, 48);
    return;
  }
  const uint64_t (*L0)[6] = (const uint64_t (*)[6])mats.L0;
  const uint64_t (*L1)[6] = (const uint64_t (*)[6])mats.L1;
  const uint64_t (*L2)[6] = (const uint64_t (*)[6])mats.L2;
  const uint64_t (*L3)[6] = (const uint64_t (*)[6])mats.L3;

  bf384_t k  = bf384_load(key);
  bf384_t c0 = bf384_load(mats.C0);
  bf384_t c1 = bf384_load(mats.C1);
  bf384_t c2 = bf384_load(mats.C2);

  bf384_t v1 = bf384_s1(bf384_add(k, c0));

  bf384_t L0k = lynx_L384(L0, k);
  bf384_t v2  = bf384_s2(bf384_add(L0k, c1));

  bf384_t L1v1 = lynx_L384(L1, v1);
  bf384_t L2v2 = lynx_L384(L2, v2);
  bf384_t L3sum = lynx_L384(L3, bf384_add(k, bf384_add(v1, v2)));
  bf384_t invkc2 = bf384_inv_allones_exp(bf384_add(k, c2));
  bf384_t y = bf384_add(bf384_mul(invkc2, bf384_mul(L1v1, L2v2)), L3sum);

  bf384_store(output, y);
}

void owf_lynx_512(const uint8_t* key, const uint8_t* input, uint8_t* output) {
  lynx_t mats;
  if (lynx_init(&mats, 512, input, 64) != 0) {
    memset(output, 0, 64);
    return;
  }
  const uint64_t (*L0)[8] = (const uint64_t (*)[8])mats.L0;
  const uint64_t (*L1)[8] = (const uint64_t (*)[8])mats.L1;
  const uint64_t (*L2)[8] = (const uint64_t (*)[8])mats.L2;
  const uint64_t (*L3)[8] = (const uint64_t (*)[8])mats.L3;

  bf512_t k  = bf512_load(key);
  bf512_t c0 = bf512_load(mats.C0);
  bf512_t c1 = bf512_load(mats.C1);

  bf512_t v1 = bf512_s1(bf512_add(k, c0));

  bf512_t L0k = lynx_L512(L0, k);
  bf512_t v2  = bf512_s2(bf512_add(L0k, c1));

  bf512_t L1v1 = lynx_L512(L1, v1);
  bf512_t L2v2 = lynx_L512(L2, v2);
  bf512_t L3sum = lynx_L512(L3, bf512_add(k, bf512_add(v1, v2)));
  bf512_t c2     = bf512_load(mats.C2);
  bf512_t invkc2 = bf512_inv_allones_exp(bf512_add(k, c2));
  bf512_t y      = bf512_add(bf512_mul(invkc2, bf512_mul(L1v1, L2v2)),
                        L3sum);

  bf512_store(output, y);
}

/* ================================================================
   Witness extension
   w = k || v_1 || v_2     [3n bits]
   ================================================================ */

void lynx_extend_witness(uint8_t* w, const uint8_t* key, const uint8_t* in,
                         const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, params->csp, in, params->owf_input_size) != 0) {
    memset(w, 0, (3 * params->csp) / 8);
    return;
  }

  switch (params->csp) {
  case 128: {
    const uint64_t (*L0)[2] = (const uint64_t (*)[2])mats.L0;
    bf128_t k  = bf128_load(key);
    bf128_t c0 = bf128_load(mats.C0);
    bf128_t c1 = bf128_load(mats.C1);

    bf128_t v1  = bf128_inv_itoh(bf128_add(k, c0));
    bf128_t v2  = bf128_inv_itoh(bf128_add(lynx_L128(L0, k), c1));
    bf128_store(w, k);
    bf128_store(w + 16, v1);
    bf128_store(w + 32, v2);
    break;
  }
  case 160: {
    const uint64_t (*L0)[3] = (const uint64_t (*)[3])mats.L0;
    bf160_t k  = bf160_load(key);
    bf160_t c0 = bf160_load(mats.C0);

    bf160_t v1  = bf160_s1(bf160_add(k, c0));
    bf160_t v2  = bf160_s2(bf160_add(lynx_L160(L0, k), c0));
    bf160_store(w, k);
    bf160_store(w + 20, v1);
    bf160_store(w + 40, v2);
    break;
  }
  case 192: {
    const uint64_t (*L0)[3] = (const uint64_t (*)[3])mats.L0;
    bf192_t k  = bf192_load(key);
    bf192_t c0 = bf192_load(mats.C0);
    bf192_t c1 = bf192_load(mats.C1);

    bf192_t v1  = bf192_s1(bf192_add(k, c0));
    bf192_t v2  = bf192_s2(bf192_add(lynx_L192(L0, k), c1));
    bf192_store(w, k);
    bf192_store(w + 24, v1);
    bf192_store(w + 48, v2);
    break;
  }
  case 256: {
    const uint64_t (*L0)[4] = (const uint64_t (*)[4])mats.L0;
    bf256_t k  = bf256_load(key);
    bf256_t c0 = bf256_load(mats.C0);
    bf256_t c1 = bf256_load(mats.C1);

    bf256_t v1  = bf256_s1(bf256_add(k, c0));
    bf256_t v2  = bf256_s2(bf256_add(lynx_L256(L0, k), c1));
    bf256_store(w, k);
    bf256_store(w + 32, v1);
    bf256_store(w + 64, v2);
    break;
  }
  case 384: {
    const uint64_t (*L0)[6] = (const uint64_t (*)[6])mats.L0;
    bf384_t k  = bf384_load(key);
    bf384_t c0 = bf384_load(mats.C0);
    bf384_t c1 = bf384_load(mats.C1);

    bf384_t v1  = bf384_s1(bf384_add(k, c0));
    bf384_t v2  = bf384_s2(bf384_add(lynx_L384(L0, k), c1));
    bf384_store(w, k);
    bf384_store(w + 48, v1);
    bf384_store(w + 96, v2);
    break;
  }
  case 512: {
    const uint64_t (*L0)[8] = (const uint64_t (*)[8])mats.L0;
    bf512_t k  = bf512_load(key);
    bf512_t c0 = bf512_load(mats.C0);
    bf512_t c1 = bf512_load(mats.C1);

    bf512_t v1  = bf512_s1(bf512_add(k, c0));
    bf512_t v2  = bf512_s2(bf512_add(lynx_L512(L0, k), c1));
    bf512_store(w, k);
    bf512_store(w + 64, v1);
    bf512_store(w + 128, v2);
    break;
  }
  default:
    memset(w, 0, (3 * params->csp) / 8);
    break;
  }
}
