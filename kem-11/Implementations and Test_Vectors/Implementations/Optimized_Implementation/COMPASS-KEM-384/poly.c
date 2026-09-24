#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "cbd.h"
#include "symmetric.h"
#include "verify.h"
#include "consts.h"
#include <immintrin.h>
#include "align.h"
#include "consts.h"

#if (COMPASS_KEM_Q == 7681)
  #include "zetas_7681_avx.h"
#endif

#if (COMPASS_KEM_Q == 3329)
#define tomont_avx COMPASS_KEM_NAMESPACE(tomont_avx)
extern void tomont_avx(int16_t *r, const int16_t *qdata);

#define reduce_avx COMPASS_KEM_NAMESPACE(reduce_avx)
extern void reduce_avx(int16_t *r, const int16_t *qdata);
#endif

/*************************************************
* Name:        poly_compress_arith_avx
*
* Description: AVX2-accelerated arithmetic rounding compression
* Description: Generic AVX2 arithmetic rounding compression operator
**************************************************/
/*************************************************
* Name:        poly_compress_arith_avx
*
* Description: AVX2-accelerated arithmetic rounding compression_fast
* Description: Pure integer AVX2 operator, mathematically equivalent to MLWR truncation logic
**************************************************/
static inline void poly_compress_arith_avx_fast(uint16_t *t_out, const poly *a, int8_t d) {
    __m256i q_vec = _mm256_set1_epi16(COMPASS_KEM_Q);
    __m256i zero = _mm256_setzero_si256();
    // Convert scalar shift amount to __m128i for _mm256_srl_epi16
    __m128i d_vec = _mm_cvtsi32_si128(d);

    for (int i = 0; i < COMPASS_KEM_N; i += 16) {
        __m256i v = _mm256_load_si256((__m256i *)&a->coeffs[i]);

        // 1. Normalize negatives: u += (u >> 15) & COMPASS_KEM_Q
        __m256i sign = _mm256_srai_epi16(v, 15);
        __m256i add_q = _mm256_and_si256(sign, q_vec);
        v = _mm256_add_epi16(v, add_q);

        // 2. Decrement-if-nonzero: d_val -= (d_val != 0)
        // Since v >= 0 after step 1, cmpgt returns 0xFFFF (-1) if v > 0.
        // Adding -1 is equivalent to subtracting 1. Perfect match!
        __m256i is_gt_zero = _mm256_cmpgt_epi16(v, zero);
        v = _mm256_add_epi16(v, is_gt_zero);

        // 3. Core compression shift: t[j] = d_val >> d
        v = _mm256_srl_epi16(v, d_vec);

        // 4. Store 16 processed coefficients into temporary array
        _mm256_storeu_si256((__m256i *)&t_out[i], v);
    }
}


void poly_compress(uint8_t *r, const poly *a, int8_t d) {
  unsigned int i;
  // Temporary array to hold AVX2-computed COMPASS_KEM_N coefficients
  uint16_t t_full[COMPASS_KEM_N] __attribute__((aligned(32)));

  // [Performance]: After this call, all arithmetic, shifts, and comparisons are complete
  poly_compress_arith_avx_fast(t_full, a, d);

#if (COMPASS_KEM_Q == 3329)
  // --- 128/256-bit security level (q=3329) ---
  if (d == 2) {
    for(i=0; i<COMPASS_KEM_N/4; i++) {
      // Read from t_full[block] instead of original t[j]
      r[0] = t_full[4*i+0] & 0xFF;
      r[1] = (t_full[4*i+0] >> 8) | ((t_full[4*i+1] & 0x3F) << 2);
      r[2] = (t_full[4*i+1] >> 6) | ((t_full[4*i+2] & 0x0F) << 4);
      r[3] = (t_full[4*i+2] >> 4) | ((t_full[4*i+3] & 0x03) << 6);
      r[4] = (t_full[4*i+3] >> 2);
      r += 5;
    }
  } 
  else if (d == 6) {
    for(i=0; i<COMPASS_KEM_N/4; i++) {
      r[0] = t_full[4*i+0] | (t_full[4*i+1] << 6);
      r[1] = (t_full[4*i+1] >> 2) | (t_full[4*i+2] << 4);
      r[2] = (t_full[4*i+2] >> 4) | (t_full[4*i+3] << 2);
      r += 3;
    }
  }
  else if (d == 8) {
    for(i=0; i<COMPASS_KEM_N/2; i++) {
      r[i] = t_full[2*i+0] | (t_full[2*i+1] << 4); 
    }
  }

#elif (COMPASS_KEM_Q == 7681)
  // --- 384/512-bit security level (q=7681) ---
  if (d == 2) {
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      r[0] = t_full[8*i+0] & 0xFF;
      r[1] = (t_full[8*i+0] >> 8) | ((t_full[8*i+1] & 0x1F) << 3);
      r[2] = (t_full[8*i+1] >> 5) | ((t_full[8*i+2] & 0x03) << 6);
      r[3] = (t_full[8*i+2] >> 2) & 0xFF;
      r[4] = (t_full[8*i+2] >> 10) | ((t_full[8*i+3] & 0x7F) << 1);
      r[5] = (t_full[8*i+3] >> 7) | ((t_full[8*i+4] & 0x0F) << 4);
      r[6] = (t_full[8*i+4] >> 4) | ((t_full[8*i+5] & 0x01) << 7);
      r[7] = (t_full[8*i+5] >> 1) & 0xFF;
      r[8] = (t_full[8*i+5] >> 9) | ((t_full[8*i+6] & 0x3F) << 2);
      r[9] = (t_full[8*i+6] >> 6) | ((t_full[8*i+7] & 0x07) << 5);
      r[10] = (t_full[8*i+7] >> 3);
      r += 11;
    }
  } 
  else if (d == 6) {
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      r[0] = t_full[8*i+0] | (t_full[8*i+1] << 7);
      r[1] = (t_full[8*i+1] >> 1) | (t_full[8*i+2] << 6);
      r[2] = (t_full[8*i+2] >> 2) | (t_full[8*i+3] << 5);
      r[3] = (t_full[8*i+3] >> 3) | (t_full[8*i+4] << 4);
      r[4] = (t_full[8*i+4] >> 4) | (t_full[8*i+5] << 3);
      r[5] = (t_full[8*i+5] >> 5) | (t_full[8*i+6] << 2);
      r[6] = (t_full[8*i+6] >> 6) | (t_full[8*i+7] << 1);
      r += 7;
    }
  }
  else if (d == 8) {
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      r[0] = t_full[8*i+0] | (t_full[8*i+1] << 5);
      r[1] = (t_full[8*i+1] >> 3) | (t_full[8*i+2] << 2) | (t_full[8*i+3] << 7);
      r[2] = (t_full[8*i+3] >> 1) | (t_full[8*i+4] << 4);
      r[3] = (t_full[8*i+4] >> 4) | (t_full[8*i+5] << 1) | (t_full[8*i+6] << 6);
      r[4] = (t_full[8*i+6] >> 2) | (t_full[8*i+7] << 3);
      r += 5;
    }
  }
#endif
}

// void poly_compress(uint8_t *r, const poly *a, int8_t d) {
//   unsigned int i, j;
//   uint16_t u;

// #if (COMPASS_KEM_Q == 3329)
//   // --- 128/256-bit security level (q=3329) ---
//   if (d == 2) {
//     // 压缩为 10 bits (对应公钥 d 和密文 d1)
//     uint16_t t[4];
//     for(i=0; i<COMPASS_KEM_N/4; i++) {
//       for(j=0; j<4; j++) {
//         u = a->coeffs[4*i+j];
//         u += (u >> 15) & COMPASS_KEM_Q;
//         if (u > 0) u--;
//         t[j] = u >> 2; 
//       }
//       r[0] = t[0] & 0xFF;
//       r[1] = (t[0] >> 8) | ((t[1] & 0x3F) << 2);
//       r[2] = (t[1] >> 6) | ((t[2] & 0x0F) << 4);
//       r[3] = (t[2] >> 4) | ((t[3] & 0x03) << 6);
//       r[4] = (t[3] >> 2);
//       r += 5;
//     }
//   } 
//   else if (d == 6) {
//     // 压缩为 6 bits (对应密文 d2)
//     uint8_t t[4];
//     for(i=0; i<COMPASS_KEM_N/4; i++) {
//       for(j=0; j<4; j++) {
//         u = a->coeffs[4*i+j];
//         u += (u >> 15) & COMPASS_KEM_Q;
//         if (u > 0) u--;
//         t[j] = u >> 6;
//       }
//       r[0] = t[0] | (t[1] << 6);
//       r[1] = (t[1] >> 2) | (t[2] << 4);
//       r[2] = (t[2] >> 4) | (t[3] << 2);
//       r += 3;
//     }
//   }

// #elif (COMPASS_KEM_Q == 7681)
//   // --- 384/512-bit security level (q=7681) ---
//   if (d == 2) {
//     // 压缩为 11 bits (对应公钥 d 和密文 d1)
//     uint16_t t[8];
//     for(i=0; i<COMPASS_KEM_N/8; i++) {
//       for(j=0; j<8; j++) {
//         u = a->coeffs[8*i+j];
//         u += (u >> 15) & COMPASS_KEM_Q;
//         if (u > 0) u--;
//         t[j] = u >> 2;
//       }
//       r[0] = t[0] & 0xFF;
//       r[1] = (t[0] >> 8) | ((t[1] & 0x1F) << 3);
//       r[2] = (t[1] >> 5) | ((t[2] & 0x03) << 6);
//       r[3] = (t[2] >> 2) & 0xFF;
//       r[4] = (t[2] >> 10) | ((t[3] & 0x7F) << 1);
//       r[5] = (t[3] >> 7) | ((t[4] & 0x0F) << 4);
//       r[6] = (t[4] >> 4) | ((t[5] & 0x01) << 7);
//       r[7] = (t[5] >> 1) & 0xFF;
//       r[8] = (t[5] >> 9) | ((t[6] & 0x3F) << 2);
//       r[9] = (t[6] >> 6) | ((t[7] & 0x07) << 5);
//       r[10] = (t[7] >> 3);
//       r += 11;
//     }
//   } 
//   else if (d == 6) {
//     // 压缩为 7 bits (对应密文 d2)
//     uint8_t t[8];
//     for(i=0; i<COMPASS_KEM_N/8; i++) {
//       for(j=0; j<8; j++) {
//         u = a->coeffs[8*i+j];
//         u += (u >> 15) & COMPASS_KEM_Q;
//         if (u > 0) u--;
//         t[j] = u >> 6;
//       }
//       r[0] = t[0] | (t[1] << 7);
//       r[1] = (t[1] >> 1) | (t[2] << 6);
//       r[2] = (t[2] >> 2) | (t[3] << 5);
//       r[3] = (t[3] >> 3) | (t[4] << 4);
//       r[4] = (t[4] >> 4) | (t[5] << 3);
//       r[5] = (t[5] >> 5) | (t[6] << 2);
//       r[6] = (t[6] >> 6) | (t[7] << 1);
//       r += 7;
//     }
//   }
// #endif
// }

/*************************************************
* Name:        poly_decompress
*
* Description: De-serialization and subsequent decompression of a polynomial;
*              approximate inverse of poly_compress
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of length COMPASS_KEM_POLYCOMPRESSEDBYTES bytes)
**************************************************/
static inline void poly_decompress_arith_avx_fast(poly *r, const uint16_t *t_in, int8_t d) {
    // Construct shift vector for d
    __m128i d_vec = _mm_cvtsi32_si128(d);
    // Construct rounding constant 1 << (d-1)
    __m256i half_vec = _mm256_set1_epi16(1 << (d - 1));

    for (int i = 0; i < COMPASS_KEM_N; i += 16) {
        // Load unpacked clean coefficients
        __m256i v = _mm256_load_si256((__m256i *)&t_in[i]);

        // 1. Core scaling shift: v = v << d
        v = _mm256_sll_epi16(v, d_vec);

        // 2. Add half-step rounding: v = v | (1 << (d-1))
        // After left shift by d bits, low d bits are 0; OR is safer and faster than ADD
        v = _mm256_or_si256(v, half_vec);

        // Store final result directly into polynomial r
        _mm256_store_si256((__m256i *)&r->coeffs[i], v);
    }
}

void poly_decompress(poly *r, const uint8_t *a, int8_t d) {
  unsigned int i;
  // Temporary array to hold unpacked COMPASS_KEM_N coefficients
  uint16_t t_full[COMPASS_KEM_N] __attribute__((aligned(32)));

#if (COMPASS_KEM_Q == 3329)
  if (d == 2) {
    // Extract 10 bits
    for(i=0; i<COMPASS_KEM_N/4; i++) {
      t_full[4*i+0] = ((a[0] >> 0) | ((uint16_t)a[1] << 8)) & 0x3FF;
      t_full[4*i+1] = ((a[1] >> 2) | ((uint16_t)a[2] << 6)) & 0x3FF;
      t_full[4*i+2] = ((a[2] >> 4) | ((uint16_t)a[3] << 4)) & 0x3FF;
      t_full[4*i+3] = ((a[3] >> 6) | ((uint16_t)a[4] << 2)) & 0x3FF;
      a += 5;
    }
  } 
  else if (d == 6) {
    // Extract 6 bits
    for(i=0; i<COMPASS_KEM_N/4; i++) {
      t_full[4*i+0] = ((a[0]      )                   ) & 0x3F;
      t_full[4*i+1] = ((a[0] >> 6) | ((uint16_t)a[1] << 2)) & 0x3F;
      t_full[4*i+2] = ((a[1] >> 4) | ((uint16_t)a[2] << 4)) & 0x3F;
      t_full[4*i+3] = ((a[2] >> 2)                        ) & 0x3F;
      a += 3;
    }
  }
  else if (d == 8) {
    // Extract 4 bits
    for(i=0; i<COMPASS_KEM_N/2; i++) {
      t_full[2*i+0] = a[i] & 0x0F;
      t_full[2*i+1] = a[i] >> 4;
    }
  }

#elif (COMPASS_KEM_Q == 7681)
  if (d == 2) {
    // Extract 11 bits
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      t_full[8*i+0] = ((a[0] >> 0) | ((uint16_t)a[1] << 8)) & 0x7FF;
      t_full[8*i+1] = ((a[1] >> 3) | ((uint16_t)a[2] << 5)) & 0x7FF;
      t_full[8*i+2] = ((a[2] >> 6) | ((uint16_t)a[3] << 2) | ((uint16_t)a[4] << 10)) & 0x7FF;
      t_full[8*i+3] = ((a[4] >> 1) | ((uint16_t)a[5] << 7)) & 0x7FF;
      t_full[8*i+4] = ((a[5] >> 4) | ((uint16_t)a[6] << 4)) & 0x7FF;
      t_full[8*i+5] = ((a[6] >> 7) | ((uint16_t)a[7] << 1) | ((uint16_t)a[8] << 9)) & 0x7FF;
      t_full[8*i+6] = ((a[8] >> 2) | ((uint16_t)a[9] << 6)) & 0x7FF;
      t_full[8*i+7] = ((a[9] >> 5) | ((uint16_t)a[10] << 3)) & 0x7FF;
      a += 11;
    }
  } 
  else if (d == 6) {
    // Extract 7 bits
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      t_full[8*i+0] = ((a[0]      )                   ) & 0x7F;
      t_full[8*i+1] = ((a[0] >> 7) | ((uint16_t)a[1] << 1)) & 0x7F;
      t_full[8*i+2] = ((a[1] >> 6) | ((uint16_t)a[2] << 2)) & 0x7F;
      t_full[8*i+3] = ((a[2] >> 5) | ((uint16_t)a[3] << 3)) & 0x7F;
      t_full[8*i+4] = ((a[3] >> 4) | ((uint16_t)a[4] << 4)) & 0x7F;
      t_full[8*i+5] = ((a[4] >> 3) | ((uint16_t)a[5] << 5)) & 0x7F;
      t_full[8*i+6] = ((a[5] >> 2) | ((uint16_t)a[6] << 6)) & 0x7F;
      t_full[8*i+7] = ((a[6] >> 1)                    ) & 0x7F;
      a += 7;
    }
  }
  else if (d == 8) {
    // Extract 5 bits
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      t_full[8*i+0] = ((a[0]      )       ) & 0x1F;
      t_full[8*i+1] = ((a[0] >> 5) | ((uint16_t)a[1] << 3)) & 0x1F;
      t_full[8*i+2] = ((a[1] >> 2)        ) & 0x1F;
      t_full[8*i+3] = ((a[1] >> 7) | ((uint16_t)a[2] << 1)) & 0x1F;
      t_full[8*i+4] = ((a[2] >> 4) | ((uint16_t)a[3] << 4)) & 0x1F;
      t_full[8*i+5] = ((a[3] >> 1)        ) & 0x1F;
      t_full[8*i+6] = ((a[3] >> 6) | ((uint16_t)a[4] << 2)) & 0x1F;
      t_full[8*i+7] = ((a[4] >> 3)        ) & 0x1F;
      a += 5;
    }
  }
#endif

  // [Performance]: After unpacking, use AVX2 for instant scaling and precision compensation
  poly_decompress_arith_avx_fast(r, t_full, d);
}

/*************************************************
* Name:        poly_tobytes
*
* Description: Serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for COMPASS_KEM_POLYBYTES bytes)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_tobytes(uint8_t r[COMPASS_KEM_POLYBYTES], const poly *a)
{
  unsigned int i;
  
#if (COMPASS_KEM_Q == 3329)
  uint16_t t0, t1;
  for(i=0;i<COMPASS_KEM_N/2;i++) {
    // map to positive standard representatives
    t0  = a->coeffs[2*i];
    t0 += ((int16_t)t0 >> 15) & COMPASS_KEM_Q;
    t1 = a->coeffs[2*i+1];
    t1 += ((int16_t)t1 >> 15) & COMPASS_KEM_Q;
    r[3*i+0] = (t0 >> 0);
    r[3*i+1] = (t0 >> 8) | (t1 << 4);
    r[3*i+2] = (t1 >> 4);
  }
#elif (COMPASS_KEM_Q == 7681)
  // 新增的 13-bit 逻辑：8个系数转13字节
  uint16_t t[8];
  for(i=0; i<COMPASS_KEM_N/8; i++) {
    for(int j=0; j<8; j++) {
      t[j] = a->coeffs[8*i+j];
      t[j] += ((int16_t)t[j] >> 15) & COMPASS_KEM_Q;
    }
    r[0]  = (t[0] >> 0);
    r[1]  = (t[0] >> 8) | (t[1] << 5);
    r[2]  = (t[1] >> 3);
    r[3]  = (t[1] >> 11) | (t[2] << 2);
    r[4]  = (t[2] >> 6) | (t[3] << 7);
    r[5]  = (t[3] >> 1);
    r[6]  = (t[3] >> 9) | (t[4] << 4);
    r[7]  = (t[4] >> 4);
    r[8]  = (t[4] >> 12) | (t[5] << 1);
    r[9]  = (t[5] >> 7) | (t[6] << 6);
    r[10] = (t[6] >> 2);
    r[11] = (t[6] >> 10) | (t[7] << 3);
    r[12] = (t[7] >> 5);
    r += 13;
  }
#endif
}

/*************************************************
* Name:        poly_frombytes
*
* Description: De-serialization of a polynomial;
*              inverse of poly_tobytes
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of COMPASS_KEM_POLYBYTES bytes)
**************************************************/
void poly_frombytes(poly *r, const uint8_t a[COMPASS_KEM_POLYBYTES])
{
  unsigned int i;
#if (COMPASS_KEM_Q == 3329)
  for(i=0;i<COMPASS_KEM_N/2;i++) {
    r->coeffs[2*i]   = ((a[3*i+0] >> 0) | ((uint16_t)a[3*i+1] << 8)) & 0xFFF;
    r->coeffs[2*i+1] = ((a[3*i+1] >> 4) | ((uint16_t)a[3*i+2] << 4)) & 0xFFF;
  }
#elif (COMPASS_KEM_Q == 7681)
  // --- 13-bit 逻辑 (每 13 字节解包 8 个系数) ---
  // 掩码为 0x1FFF (即 2^13 - 1)
  for(i=0; i<COMPASS_KEM_N/8; i++) {
    r->coeffs[8*i+0] = ( (uint16_t)a[13*i+0]       | ((uint16_t)a[13*i+1] << 8)) & 0x1FFF;
    r->coeffs[8*i+1] = ( (uint16_t)a[13*i+1] >> 5  | ((uint16_t)a[13*i+2] << 3) | ((uint16_t)a[13*i+3] << 11)) & 0x1FFF;
    r->coeffs[8*i+2] = ( (uint16_t)a[13*i+3] >> 2  | ((uint16_t)a[13*i+4] << 6)) & 0x1FFF;
    r->coeffs[8*i+3] = ( (uint16_t)a[13*i+4] >> 7  | ((uint16_t)a[13*i+5] << 1) | ((uint16_t)a[13*i+6] << 9)) & 0x1FFF;
    r->coeffs[8*i+4] = ( (uint16_t)a[13*i+6] >> 4  | ((uint16_t)a[13*i+7] << 4) | ((uint16_t)a[13*i+8] << 12)) & 0x1FFF;
    r->coeffs[8*i+5] = ( (uint16_t)a[13*i+8] >> 1  | ((uint16_t)a[13*i+9] << 7)) & 0x1FFF;
    r->coeffs[8*i+6] = ( (uint16_t)a[13*i+9] >> 6  | ((uint16_t)a[13*i+10] << 2) | ((uint16_t)a[13*i+11] << 10)) & 0x1FFF;
    r->coeffs[8*i+7] = ( (uint16_t)a[13*i+11] >> 3 | ((uint16_t)a[13*i+12] << 5)) & 0x1FFF;
  }
#endif
}

/*************************************************
* Name:        poly_frommsg
*
* Description: Convert 32-byte message to polynomial
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *msg: pointer to input message
**************************************************/
void poly_frommsg(poly *r, const uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES])
{
  unsigned int i, j;
  int16_t mask;

  // 1. 仅精确处理前 256 个系数 (对应 32 字节 message)
  for(i=0; i<COMPASS_KEM_INDCPA_MSGBYTES; i++) {
    for(j=0; j<8; j++) {
      // Extract bit. If 1, mask = 0xFFFF; otherwise 0
      mask = -(int16_t)((msg[i] >> j) & 1);
      
      // Set directly to 0 or (q-1)/2
      r->coeffs[8*i+j] = mask & ((COMPASS_KEM_Q - 1) / 2);
    }
  }

  // 2. MLWR 异构保护：若参数集 N > 256 (例如 512)，安全清零高位残余内存
  for(i = COMPASS_KEM_INDCPA_MSGBYTES * 8; i < COMPASS_KEM_N; i++) {
    r->coeffs[i] = 0;
  }
}
// void poly_frommsg(poly *r, const uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES])
// {
//   unsigned int i, j;
//   int16_t mask;

//   // 先将所有系数清零 (非常重要：防止 n=512 时高位出现未初始化的脏数据)
//   for(i=0; i<COMPASS_KEM_N; i++) {
//     r->coeffs[i] = 0;
//   }

//   // 仅仅处理 32 字节 (256 bits)
//   for(i=0; i<COMPASS_KEM_INDCPA_MSGBYTES; i++) {
//     for(j=0; j<8; j++) {
//       // 提取 msg 的对应 bit。如果是 1 则 mask 为 0xFFFF (-1)，否则为 0
//       mask = -(int16_t)((msg[i] >> j) & 1);
//       // 如果 bit 为 1，则赋值为 (q-1)/2；否则为 0
//       r->coeffs[8*i+j] = mask & ((COMPASS_KEM_Q - 1) / 2);
//     }
//   }
// }
/*************************************************
* Name:        poly_tomsg
*
* Description: Convert polynomial to 32-byte message
*
* Arguments:   - uint8_t *msg: pointer to output message
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_tomsg(uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES], const poly *a)
{
  unsigned int i, j;
  uint16_t t;
  
  // 提前计算常量，避免在循环内部重复除法
  const uint16_t lower_bound = (COMPASS_KEM_Q - 1) / 4;
  const uint16_t width = (COMPASS_KEM_Q - 1) / 2; // upper_bound - lower_bound

  for(i=0; i<COMPASS_KEM_INDCPA_MSGBYTES; i++) {
    msg[i] = 0;
    for(j=0; j<8; j++) {
      t = a->coeffs[8*i+j];
      
      // 1. Map negative values to standard domain [0, q-1]
      t += ((int16_t)t >> 15) & COMPASS_KEM_Q;

      // 2. Constant-time branchless threshold decision
      // Using unsigned underflow: if t < lower_bound, t - lower_bound wraps to a large positive number
      uint16_t bit = (uint16_t)(t - lower_bound) < width;
      
      // 3. Write byte
      msg[i] |= (bit << j);
    }
  }
}
// void poly_tomsg(uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES], const poly *a)
// {
//   unsigned int i, j;
//   uint16_t t;

//   for(i=0; i<COMPASS_KEM_INDCPA_MSGBYTES; i++) {
//     msg[i] = 0;
//     for(j=0; j<8; j++) {
//       t = a->coeffs[8*i+j];
      
//       // 1. Map negative values to standard domain [0, q-1]
//       t += ((int16_t)t >> 15) & COMPASS_KEM_Q;

//       // 2. MLWR 的精确阈值判决逻辑
//       int bit = 0;
//       uint16_t lower_bound = (COMPASS_KEM_Q - 1) / 4;
//       uint16_t upper_bound = 3 * (COMPASS_KEM_Q - 1) / 4;
      
//       if (t >= lower_bound && t < upper_bound) {
//         bit = 1;
//       }
      
//       // 3. Write byte
//       msg[i] |= (bit << j);
//     }
//   }
// }

/*************************************************
* Name:        poly_getnoise_eta1
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with parameter COMPASS_KEM_ETA1
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*                                     (of length COMPASS_KEM_SYMBYTES bytes)
*              - uint8_t nonce: one-byte input nonce
**************************************************/
void poly_getnoise_eta1(poly *r, const uint8_t seed[COMPASS_KEM_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta1(r, buf);
}

/*************************************************
* Name:        poly_getnoise_eta2
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with parameter COMPASS_KEM_ETA2
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*                                     (of length COMPASS_KEM_SYMBYTES bytes)
*              - uint8_t nonce: one-byte input nonce
**************************************************/
void poly_getnoise_eta2(poly *r, const uint8_t seed[COMPASS_KEM_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[COMPASS_KEM_ETA2*COMPASS_KEM_N/4];
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta2(r, buf);
}


/*************************************************
* Name:        poly_ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - uint16_t *r: pointer to in/output polynomial
**************************************************/
// void poly_ntt(poly *r)
// {
//   ntt(r->coeffs);
//   poly_reduce(r);
// }
#if (COMPASS_KEM_Q == 3329)
#endif

void poly_ntt(poly *r)
{
#if (COMPASS_KEM_Q == 3329)
  // Use serial NTT for q=3329 (bit-identical to Reference)
  ntt(r->coeffs);
  poly_reduce(r);
#elif (COMPASS_KEM_Q == 7681)
  // Use AVX2-optimized NTT for q=7681
  ntt_avx_7681(r->coeffs);
#endif
}

void poly_invntt_tomont(poly *r)
{
#if (COMPASS_KEM_Q == 3329)
  // Use serial INTT for q=3329 (bit-identical to Reference)
  invntt(r->coeffs);
#elif (COMPASS_KEM_Q == 7681)
  // Use AVX2-optimized INTT for q=7681
  invntt_avx_7681(r->coeffs);
#endif
}

// void poly_ntt(poly *r)
// {
// #if (COMPASS_KEM_Q == 3329)
//   // 调用 AVX2 汇编极速版
//   ntt_avx(r->coeffs, qdata.coeffs);
// #elif (COMPASS_KEM_Q == 7681)
//   // 调用你原有的标量版
//   ntt(r->coeffs);
// #endif
// }

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT)
*              of a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - uint16_t *a: pointer to in/output polynomial
**************************************************/
// void poly_invntt_tomont(poly *r)
// {
//   invntt(r->coeffs);
// }

// void poly_invntt_tomont(poly *r)
// {
// #if (COMPASS_KEM_Q == 3329)
//   invntt_avx(r->coeffs, qdata.coeffs);
// #elif (COMPASS_KEM_Q == 7681)
//   invntt(r->coeffs);
// #endif
// }

/*************************************************
* Name:        poly_basemul_montgomery
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
// void poly_basemul_montgomery(poly *r, const poly *a, const poly *b)
// {
//   unsigned int i;
//   for(i=0;i<(COMPASS_KEM_N>>2);i++) {
//     basemul(&r->coeffs[4*i], &a->coeffs[4*i], &b->coeffs[4*i], zetas[(COMPASS_KEM_N>>2)+i]);
//     basemul(&r->coeffs[4*i+2], &a->coeffs[4*i+2], &b->coeffs[4*i+2], -zetas[(COMPASS_KEM_N>>2)+i]);
//   }
// }
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b)
{
#if (COMPASS_KEM_Q == 3329)
  // Use serial basemul for q=3329 (bit-identical to Reference)
  unsigned int i;
  for(i=0; i<COMPASS_KEM_N/4; i++) {
    basemul(&r->coeffs[4*i], &a->coeffs[4*i], &b->coeffs[4*i], zetas[(COMPASS_KEM_N>>2)+i]);
    basemul(&r->coeffs[4*i+2], &a->coeffs[4*i+2], &b->coeffs[4*i+2], -zetas[(COMPASS_KEM_N>>2)+i]);
  }
#elif (COMPASS_KEM_Q == 7681)
  __m256i q_vec = _mm256_set1_epi16(Q_7681);
  __m256i qinv_vec = _mm256_set1_epi16(QINV_7681);

  for (int i = 0; i < COMPASS_KEM_N; i += 16) {
      int z_idx = 128 + i/2;
      // 交织 4088 (在蒙哥马利域代表 1)，保护实部不被错误缩放
      __m256i vz = _mm256_set_epi16(
          zetas[z_idx+7], 4088, zetas[z_idx+6], 4088,
          zetas[z_idx+5], 4088, zetas[z_idx+4], 4088,
          zetas[z_idx+3], 4088, zetas[z_idx+2], 4088,
          zetas[z_idx+1], 4088, zetas[z_idx+0], 4088
      );

      __m256i va = _mm256_load_si256((__m256i *)&a->coeffs[i]);
      __m256i vb = _mm256_load_si256((__m256i *)&b->coeffs[i]);
      
      // 交换 B 支路的实部虚部
      __m256i vb_swap = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(vb, 0xB1), 0xB1);

      // p1 存 (a0*b0, a1*b1), p2 存 (a0*b1, a1*b0)
      __m256i p1 = fqmul_avx(va, vb, q_vec, qinv_vec);
      __m256i p2 = fqmul_avx(va, vb_swap, q_vec, qinv_vec);

      // 仅对奇数通道 (a1*b1) 乘 zeta，偶数通道乘 4088 保持不变
      __m256i p1_zeta = fqmul_avx(p1, vz, q_vec, qinv_vec);

      // 错位合并出最终的实部 r0 和虚部 r1
      __m256i r0_vec = _mm256_add_epi16(p1_zeta, _mm256_srli_epi32(p1_zeta, 16));
      __m256i r1_vec = _mm256_add_epi16(p2, _mm256_srli_epi32(p2, 16));

      // 掩码交织并直接写回 (单次乘法不会溢出，故无需 barrett_reduce)
      __m256i res = _mm256_blend_epi16(r0_vec, _mm256_slli_epi32(r1_vec, 16), 0xAA);
      _mm256_store_si256((__m256i *)&r->coeffs[i], res);
  }
#endif
}
/*************************************************
* Name:        poly_tomont
*
* Description: Inplace conversion of all coefficients of a polynomial
*              from normal domain to Montgomery domain
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
// void poly_tomont(poly *r)
// {
//   unsigned int i;
//   const int16_t f = (1ULL << 32) % COMPASS_KEM_Q;
//   for(i=0;i<COMPASS_KEM_N;i++)
//     r->coeffs[i] = montgomery_reduce((int32_t)r->coeffs[i]*f);
// }

// void poly_tomont(poly *r)
// {
// #if (COMPASS_KEM_Q == 3329)
//   tomont_avx(r->coeffs, qdata.coeffs);
// #elif (COMPASS_KEM_Q == 7681)
//   int i;
//   const int16_t f = (1ULL << 32) % COMPASS_KEM_Q;
//   for(i=0;i<COMPASS_KEM_N;i++)
//     r->coeffs[i] = montgomery_reduce((int32_t)r->coeffs[i]*f);
// #endif
// }

void poly_tomont(poly *r)
{
#if (COMPASS_KEM_Q == 3329)
  tomont_avx(r->coeffs, qdata.coeffs);
#elif (COMPASS_KEM_Q == 7681)
  __m256i q_vec = _mm256_set1_epi16(Q_7681);
  __m256i qinv_vec = _mm256_set1_epi16(QINV_7681);
  // f = 2^32 mod 7681 = 5569
  __m256i f_vec = _mm256_set1_epi16(5569); 

  for(int i = 0; i < COMPASS_KEM_N; i += 16) {
      __m256i v = _mm256_load_si256((__m256i *)&r->coeffs[i]);
      // 使用 Montgomery 乘法：v * f * R^-1 mod q
      v = fqmul_avx(v, f_vec, q_vec, qinv_vec);
      _mm256_store_si256((__m256i *)&r->coeffs[i], v);
  }
#endif
}

/*************************************************
* Name:        poly_reduce
*
* Description: Applies Barrett reduction to all coefficients of a polynomial
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
// void poly_reduce(poly *r)
// {
//   unsigned int i;
//   for(i=0;i<COMPASS_KEM_N;i++)
//     r->coeffs[i] = barrett_reduce(r->coeffs[i]);
// }

void poly_reduce(poly *r)
{
#if (COMPASS_KEM_Q == 3329)
  reduce_avx(r->coeffs, qdata.coeffs);
#elif (COMPASS_KEM_Q == 7681)
    __m256i v_vec = _mm256_set1_epi16(V_7681);
    __m256i q_vec = _mm256_set1_epi16(Q_7681);
    for (int i = 0; i < COMPASS_KEM_N; i += 16) {
        __m256i a = _mm256_load_si256((__m256i *)&r->coeffs[i]);
        a = barrett_reduce_avx(a, v_vec, q_vec);
        _mm256_store_si256((__m256i *)&r->coeffs[i], a);
    }
#endif
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials; no modular reduction is performed
*
* Arguments: - poly *r: pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
// void poly_add(poly *r, const poly *a, const poly *b)
// {
//   unsigned int i;
//   for(i=0;i<COMPASS_KEM_N;i++)
//     r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
// }

void poly_add(poly *r, const poly *a, const poly *b) {
    for (int i = 0; i < COMPASS_KEM_N; i += 16) {
        __m256i va = _mm256_load_si256((__m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_load_si256((__m256i *)&b->coeffs[i]);
        __m256i vc = _mm256_add_epi16(va, vb);
        _mm256_store_si256((__m256i *)&r->coeffs[i], vc);
    }
}

// 向量化减法
void poly_sub(poly *r, const poly *a, const poly *b) {
    for (int i = 0; i < COMPASS_KEM_N; i += 16) {
        __m256i va = _mm256_load_si256((__m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_load_si256((__m256i *)&b->coeffs[i]);
        __m256i vc = _mm256_sub_epi16(va, vb);
        _mm256_store_si256((__m256i *)&r->coeffs[i], vc);
    }
}


// #if (COMPASS_KEM_Q == 7681)
// // 真正的架构级榨取：融合了内积、加法、和惰性约减的超级算子
// void polyvec_basemul_acc_montgomery_7681_avx(poly *r, const polyvec *a, const polyvec *b) {
//     __m256i q_vec = _mm256_set1_epi16(Q_7681);
//     __m256i qinv_vec = _mm256_set1_epi16(QINV_7681);
//     __m256i v_vec = _mm256_set1_epi16(V_7681);
//     __m256i zero = _mm256_setzero_si256();

//     for (int i = 0; i < COMPASS_KEM_N; i += 16) {
//         __m256i acc = zero; // In-register accumulator

//         // Load corresponding 8 Zeta values
//         int z_idx = 128 + i/2;
//         // Magic constant 4088 (2^16 mod 7681): equivalent to 1 in Montgomery domain
//         // Interleave with Zetas to protect real parts from being scaled in SIMD multiply
//         __m256i vz = _mm256_set_epi16(
//             zetas[z_idx+7], 4088,
//             zetas[z_idx+6], 4088,
//             zetas[z_idx+5], 4088,
//             zetas[z_idx+4], 4088,
//             zetas[z_idx+3], 4088,
//             zetas[z_idx+2], 4088,
//             zetas[z_idx+1], 4088,
//             zetas[z_idx+0], 4088
//         );

//         // Unroll K-loop in registers to eliminate memory and redundant reduction overhead
//         for (int k = 0; k < COMPASS_KEM_K; k++) {
//             __m256i va = _mm256_load_si256((__m256i *)&a->vec[k].coeffs[i]);
//             __m256i vb = _mm256_load_si256((__m256i *)&b->vec[k].coeffs[i]);
            
//             // Swap adjacent real/imag parts of b: [b1, b0, b1, b0...]
//             // 0xB1 (10110001) permutes 16-bit lanes within 32-bit words
//             __m256i vb_swap = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(vb, 0xB1), 0xB1);

//             // [One multiply, two results]
//             // p1 even lanes = a0*b0, odd lanes = a1*b1
//             __m256i p1 = fqmul_avx(va, vb, q_vec, qinv_vec);
//             // p2 even lanes = a0*b1, odd lanes = a1*b0
//             __m256i p2 = fqmul_avx(va, vb_swap, q_vec, qinv_vec);

//             // Apply Zeta rotation to p1: odd lanes x zeta, even lanes x 4088 (=1) unchanged
//             __m256i p1_zeta = fqmul_avx(p1, vz, q_vec, qinv_vec);

//             // Combine real part: r0 = (a0*b0) + (a1*b1*zeta)
//             __m256i p1_odd_shifted = _mm256_srli_epi32(p1_zeta, 16);
//             __m256i r0_vec = _mm256_add_epi16(p1_zeta, p1_odd_shifted);

//             // Combine imaginary part: r1 = (a0*b1) + (a1*b0)
//             __m256i p2_odd_shifted = _mm256_srli_epi32(p2, 16);
//             __m256i r1_vec = _mm256_add_epi16(p2, p2_odd_shifted);

//             // Re-interleave: 0xAA (10101010) selects r1 imag parts into odd lanes
//             __m256i res = _mm256_blend_epi16(r0_vec, _mm256_slli_epi32(r1_vec, 16), 0xAA);

//             // [Lazy reduction defense]: Single basemul result is in [-q, q]
//             // With K=4, accumulated sum can reach 30724, close to int16_t 32767 limit
//             // For absolute safety, do one high-throughput Barrett reduction per inner loop
//             res = barrett_reduce_avx(res, v_vec, q_vec);

//             // In-register accumulation, zero memory I/O
//             acc = _mm256_add_epi16(acc, res);
//         }

//         // After all K basemul accumulations, do final reduction and write back
//         acc = barrett_reduce_avx(acc, v_vec, q_vec);
//         _mm256_store_si256((__m256i *)&r->coeffs[i], acc);
//     }
// }
// #endif