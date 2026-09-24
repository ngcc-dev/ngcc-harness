#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "reduce.h"

#include "reduce.h"
#include "consts.h"
#if (COMPASS_KEM_Q == 7681)
#include "ntt.h"
#include "zetas_7681_avx.h"
#endif
#if (COMPASS_KEM_Q == 3329)
#endif

/*************************************************
* Name:        polyvec_compress
*
* Description: Compress and serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for COMPASS_KEM_POLYVECCOMPRESSEDBYTES)
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
// void polyvec_compress(uint8_t *r, const polyvec *a, int8_t d) {
//   unsigned int i;
//   unsigned int bytes_per_poly = 0;
  
//   // Dynamically compute bytes per compressed polynomial
// #if (COMPASS_KEM_Q == 3329)
//   // q=3329, n=256
//   if (d == 2) {
//     bytes_per_poly = 320; // 256 * 10 bits / 8
//   } else if (d == 6) {
//     bytes_per_poly = 192; // 256 * 6 bits / 8
//   }
// #elif (COMPASS_KEM_Q == 7681)
//   // q=7681, n=512
//   if (d == 2) {
//     bytes_per_poly = 704; // 512 * 11 bits / 8
//   } else if (d == 6) {
//     bytes_per_poly = 448; // 512 * 7 bits / 8
//   }
// #else
//   #error "Unsupported COMPASS_KEM_Q in polyvec_compress"
// #endif

//   // Call per-polynomial compression in a loop
//   for(i = 0; i < COMPASS_KEM_K; i++) {
//     poly_compress(r + i * bytes_per_poly, &a->vec[i], d);
//   }
// }

void polyvec_compress(uint8_t *r, const polyvec *a, int8_t d) {
  unsigned int i;
  unsigned int bytes_per_poly = 0;
  
  // Dynamically compute bytes per compressed polynomial
#if (COMPASS_KEM_Q == 3329)
  // q=3329, n=256
  if (d == 2) {
    bytes_per_poly = 320; // 256 * 10 bits / 8
  } else if (d == 6) {
    bytes_per_poly = 192; // 256 * 6 bits / 8
  } else if (d == 8) {
    bytes_per_poly = 128; // 256 * 4 bits / 8 (drop 8 bits, keep 4 bits)
  }
#elif (COMPASS_KEM_Q == 7681)
  // q=7681, n=512
  if (d == 2) {
    bytes_per_poly = 704; // 512 * 11 bits / 8
  } else if (d == 6) {
    bytes_per_poly = 448; // 512 * 7 bits / 8
  } else if (d == 8) {
    bytes_per_poly = 320; // 512 * 5 bits / 8 (drop 8 bits, keep 5 bits)
  }
#else
  #error "Unsupported COMPASS_KEM_Q in polyvec_compress"
#endif

  // Call per-polynomial compression in a loop
  for(i = 0; i < COMPASS_KEM_K; i++) {
    poly_compress(r + i * bytes_per_poly, &a->vec[i], d);
  }
}

/*************************************************
* Name:        polyvec_decompress
*
* Description: De-serialize and decompress vector of polynomials;
* approximate inverse of polyvec_compress
*
* Arguments:   - polyvec *r:       pointer to output vector of polynomials
* - const uint8_t *a: pointer to input byte array
* (of length COMPASS_KEM_POLYVECCOMPRESSEDBYTES)
**************************************************/
void polyvec_decompress(polyvec *r, const uint8_t *a, int8_t d) {
  unsigned int i;
  unsigned int bytes_per_poly = 0;

  // Dynamically compute bytes per compressed polynomial
#if (COMPASS_KEM_Q == 3329)
  if (d == 2) {
    bytes_per_poly = 320; 
  } else if (d == 6) {
    bytes_per_poly = 192; 
  } else if (d == 8) {
    bytes_per_poly = 128; 
  }
#elif (COMPASS_KEM_Q == 7681)
  if (d == 2) {
    bytes_per_poly = 704; 
  } else if (d == 6) {
    bytes_per_poly = 448; 
  } else if (d == 8) {
    bytes_per_poly = 320; 
  }
#else
  #error "Unsupported COMPASS_KEM_Q in polyvec_decompress"
#endif

  // Call per-polynomial decompression in a loop
  for(i = 0; i < COMPASS_KEM_K; i++) {
    poly_decompress(&r->vec[i], a + i * bytes_per_poly, d);
  }
}

/*************************************************
* Name:        polyvec_decompress
*
* Description: De-serialize and decompress vector of polynomials;
*              approximate inverse of polyvec_compress
*
* Arguments:   - polyvec *r:       pointer to output vector of polynomials
*              - const uint8_t *a: pointer to input byte array
*                                  (of length COMPASS_KEM_POLYVECCOMPRESSEDBYTES)
**************************************************/
// void polyvec_decompress(polyvec *r, const uint8_t *a, int8_t d) {
//   unsigned int i;
//   unsigned int bytes_per_poly = 0;

//   // Dynamically compute bytes per compressed polynomial
// #if (COMPASS_KEM_Q == 3329)
//   if (d == 2) {
//     bytes_per_poly = 320; 
//   } else if (d == 6) {
//     bytes_per_poly = 192; 
//   }
// #elif (COMPASS_KEM_Q == 7681)
//   if (d == 2) {
//     bytes_per_poly = 704; 
//   } else if (d == 6) {
//     bytes_per_poly = 448; 
//   }
// #else
//   #error "Unsupported COMPASS_KEM_Q in polyvec_decompress"
// #endif

//   // Call per-polynomial decompression in a loop
//   for(i = 0; i < COMPASS_KEM_K; i++) {
//     poly_decompress(&r->vec[i], a + i * bytes_per_poly, d);
//   }
// }

/*************************************************
* Name:        polyvec_tobytes
*
* Description: Serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for COMPASS_KEM_POLYVECBYTES)
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_tobytes(uint8_t r[COMPASS_KEM_POLYVECBYTES], const polyvec *a)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_tobytes(r+i*COMPASS_KEM_POLYBYTES, &a->vec[i]);
}

/*************************************************
* Name:        polyvec_frombytes
*
* Description: De-serialize vector of polynomials;
*              inverse of polyvec_tobytes
*
* Arguments:   - uint8_t *r:       pointer to output byte array
*              - const polyvec *a: pointer to input vector of polynomials
*                                  (of length COMPASS_KEM_POLYVECBYTES)
**************************************************/
void polyvec_frombytes(polyvec *r, const uint8_t a[COMPASS_KEM_POLYVECBYTES])
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_frombytes(&r->vec[i], a+i*COMPASS_KEM_POLYBYTES);
}

/*************************************************
* Name:        polyvec_ntt
*
* Description: Apply forward NTT to all elements of a vector of polynomials
*
* Arguments:   - polyvec *r: pointer to in/output vector of polynomials
**************************************************/
void polyvec_ntt(polyvec *r)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_ntt(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_invntt_tomont
*
* Description: Apply inverse NTT to all elements of a vector of polynomials
*              and multiply by Montgomery factor 2^16
*
* Arguments:   - polyvec *r: pointer to in/output vector of polynomials
**************************************************/
void polyvec_invntt_tomont(polyvec *r)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_invntt_tomont(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_basemul_acc_montgomery
*
* Description: Multiply elements of a and b in NTT domain, accumulate into r,
*              and multiply by 2^-16.
*
* Arguments: - poly *r: pointer to output polynomial
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/
// void polyvec_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b)
// {
//   unsigned int i;
//   poly t;

//   poly_basemul_montgomery(r, &a->vec[0], &b->vec[0]);
//   for(i=1;i<COMPASS_KEM_K;i++) {
//     poly_basemul_montgomery(&t, &a->vec[i], &b->vec[i]);
//     poly_add(r, r, &t);
//   }

//   poly_reduce(r);
// }
#if (COMPASS_KEM_Q == 7681)
// 真正的架构级榨取：融合了内积、加法、和惰性约减的超级算子
void polyvec_basemul_acc_montgomery_7681_avx(poly *r, const polyvec *a, const polyvec *b) {
    __m256i q_vec = _mm256_set1_epi16(Q_7681);
    __m256i qinv_vec = _mm256_set1_epi16(QINV_7681);
    __m256i v_vec = _mm256_set1_epi16(V_7681);
    __m256i zero = _mm256_setzero_si256();

    for (int i = 0; i < COMPASS_KEM_N; i += 16) {
        __m256i acc = zero; // In-register accumulator

        // 获取对应的 8 个 Zeta 值
        int z_idx = 128 + i / 4; 
        
        // Magic constant 4088 (2^16 mod 7681): equivalent to 1 in Montgomery domain
        // 注意 AVX2 set_epi16 的参数是从高到低 (e15, e14, ..., e0)
        // e0/e1 = pair 0 (positive Zeta), e2/e3 = pair 1 (negative Zeta), etc.
        __m256i vz = _mm256_set_epi16(
            -zetas[z_idx+3], 4088, // e15, e14 -> coefficients 14, 15 (pair 7)
             zetas[z_idx+3], 4088, // e13, e12 -> 对应系数 12, 13 (第6对)
            -zetas[z_idx+2], 4088, // e11, e10 -> coefficients 10, 11 (pair 5)
             zetas[z_idx+2], 4088, // e9,  e8  -> 对应系数 8, 9   (第4对)
            -zetas[z_idx+1], 4088, // e7, e6 -> coefficients 6, 7   (pair 3)
             zetas[z_idx+1], 4088, // e5,  e4  -> 对应系数 4, 5   (第2对)
            -zetas[z_idx+0], 4088, // e3, e2 -> coefficients 2, 3   (pair 1)
             zetas[z_idx+0], 4088  // e1,  e0  -> 对应系数 0, 1   (第0对)
        );

        // 寄存器内展开 K 的循环，极限消除内存和无用约减开销
        for (int k = 0; k < COMPASS_KEM_K; k++) {
            __m256i va = _mm256_load_si256((__m256i *)&a->vec[k].coeffs[i]);
            __m256i vb = _mm256_load_si256((__m256i *)&b->vec[k].coeffs[i]);
            
            // Swap adjacent real/imag parts of b: [b1, b0, b1, b0...]
            // 0xB1 (10110001) permutes 16-bit lanes within 32-bit words
            __m256i vb_swap = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(vb, 0xB1), 0xB1);

            // [One multiply, two results]
            // p1 even lanes = a0*b0, odd lanes = a1*b1
            __m256i p1 = fqmul_avx(va, vb, q_vec, qinv_vec);
            // p2 even lanes = a0*b1, odd lanes = a1*b0
            __m256i p2 = fqmul_avx(va, vb_swap, q_vec, qinv_vec);

            // Apply Zeta rotation to p1: odd lanes x zeta, even lanes x 4088 (i.e., 1) unchanged
            __m256i p1_zeta = fqmul_avx(p1, vz, q_vec, qinv_vec);

            // Combine real part: r0 = (a0*b0) + (a1*b1*zeta)
            __m256i p1_odd_shifted = _mm256_srli_epi32(p1_zeta, 16);
            __m256i r0_vec = _mm256_add_epi16(p1_zeta, p1_odd_shifted);

            // Combine imag part: r1 = (a0*b1) + (a1*b0)
            __m256i p2_odd_shifted = _mm256_srli_epi32(p2, 16);
            __m256i r1_vec = _mm256_add_epi16(p2, p2_odd_shifted);

            // Re-interleave: 0xAA (10101010) selects r1 imag parts into odd lanes
            __m256i res = _mm256_blend_epi16(r0_vec, _mm256_slli_epi32(r1_vec, 16), 0xAA);

            // [Lazy reduction defense]: Single basemul result in [-q, q]
            // With K=4, accumulated sum reaches 30724, close to int16_t limit of 32767
            // For absolute safety, do one high-throughput Barrett reduction per inner loop
            res = barrett_reduce_avx(res, v_vec, q_vec);

            // In-register accumulation, zero memory I/O
            acc = _mm256_add_epi16(acc, res);
        }

        // After all K basemul accumulations, do final reduction and write back
        acc = barrett_reduce_avx(acc, v_vec, q_vec);
        _mm256_store_si256((__m256i *)&r->coeffs[i], acc);
    }
}
#endif
void polyvec_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b)
{
#if (COMPASS_KEM_Q == 3329)
  // Use serial basemul for q=3329 (bit-identical to Reference)
  unsigned int i;
  poly t;

  poly_basemul_montgomery(r, &a->vec[0], &b->vec[0]);
  for(i = 1; i < COMPASS_KEM_K; i++) {
    poly_basemul_montgomery(&t, &a->vec[i], &b->vec[i]);
    poly_add(r, r, &t);
  }
  poly_reduce(r);
#elif (COMPASS_KEM_Q == 7681)
    // Use AVX2-optimized basemul for q=7681
    polyvec_basemul_acc_montgomery_7681_avx(r, a, b);

#endif
}

/*************************************************
* Name:        polyvec_reduce
*
* Description: Applies Barrett reduction to each coefficient
*              of each element of a vector of polynomials;
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - polyvec *r: pointer to input/output polynomial
**************************************************/
void polyvec_reduce(polyvec *r)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_reduce(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_add
*
* Description: Add vectors of polynomials
*
* Arguments: - polyvec *r: pointer to output vector of polynomials
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}
