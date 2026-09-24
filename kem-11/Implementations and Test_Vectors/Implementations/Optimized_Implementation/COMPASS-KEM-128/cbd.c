#include <stdint.h>
#include "params.h"
#include "cbd.h"

/*************************************************
* Name:        load32_littleendian
*
* Description: load 4 bytes into a 32-bit integer
*              in little-endian order
*
* Arguments:   - const uint8_t *x: pointer to input byte array
*
* Returns 32-bit unsigned integer loaded from x
**************************************************/
#if COMPASS_KEM_ETA1 == 2 || COMPASS_KEM_ETA1 == 4 || COMPASS_KEM_ETA2 == 4
static uint32_t load32_littleendian(const uint8_t x[4])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  r |= (uint32_t)x[3] << 24;
  return r;
}
#endif
/*************************************************
* Name:        load24_littleendian
*
* Description: load 3 bytes into a 32-bit integer
*              in little-endian order.
*              This function is only needed for COMPASS_KEM-512
*
* Arguments:   - const uint8_t *x: pointer to input byte array
*
* Returns 32-bit unsigned integer loaded from x (most significant byte is zero)
**************************************************/
#if COMPASS_KEM_ETA1 == 3
__attribute__((unused))
static uint32_t load24_littleendian(const uint8_t x[3])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  return r;
}
#endif


/*************************************************
* Name:        cbd2
*
* Description: Given an array of uniformly random bytes, compute
*              polynomial with coefficients distributed according to
*              a centered binomial distribution with parameter eta=2
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *buf: pointer to input byte array
**************************************************/
#if COMPASS_KEM_ETA1 == 2 || COMPASS_KEM_ETA2 == 2
static void cbd2(poly *r, const uint8_t buf[2*COMPASS_KEM_N/4])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  for(i=0;i<COMPASS_KEM_N/8;i++) {
    t  = load32_littleendian(buf+4*i);
    d  = t & 0x55555555;
    d += (t>>1) & 0x55555555;

    for(j=0;j<8;j++) {
      a = (d >> (4*j+0)) & 0x3;
      b = (d >> (4*j+2)) & 0x3;
      r->coeffs[8*i+j] = a - b;
    }
  }
}
#endif
/*************************************************
* Name:        cbd3
*
* Description: Given an array of uniformly random bytes, compute
*              polynomial with coefficients distributed according to
*              a centered binomial distribution with parameter eta=3.
*              This function is only needed for COMPASS_KEM-512
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *buf: pointer to input byte array
**************************************************/
#if COMPASS_KEM_ETA1 == 3 || COMPASS_KEM_ETA2 == 3
static void cbd3(poly *r, const uint8_t buf[3*COMPASS_KEM_N/4])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  for(i=0;i<COMPASS_KEM_N/4;i++) {
    t  = load24_littleendian(buf+3*i);
    d  = t & 0x00249249;
    d += (t>>1) & 0x00249249;
    d += (t>>2) & 0x00249249;

    for(j=0;j<4;j++) {
      a = (d >> (6*j+0)) & 0x7;
      b = (d >> (6*j+3)) & 0x7;
      r->coeffs[4*i+j] = a - b;
    }
  }
}
#endif

#if COMPASS_KEM_ETA1 == 4 || COMPASS_KEM_ETA2 == 4
static void cbd4(poly *r, const uint8_t buf[4*COMPASS_KEM_N/4])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  // Each iteration processes 32 bits (4 bytes), generates 4 coefficients
  for(i=0;i<COMPASS_KEM_N/4;i++) {
    t  = load32_littleendian(buf+4*i);
    
    // Compute Hamming weight in parallel for each 4-bit nibble
    d  = t & 0x11111111;
    d += (t>>1) & 0x11111111;
    d += (t>>2) & 0x11111111;
    d += (t>>3) & 0x11111111;

    for(j=0;j<4;j++) {
      a = (d >> (8*j+0)) & 0xf; // Extract even nibble (low 4 bits) for popcount
      b = (d >> (8*j+4)) & 0xf; // Extract odd nibble (high 4 bits) for popcount
      r->coeffs[4*i+j] = a - b;
    }
  }
}
#endif

void poly_cbd2_avx(int16_t *r, const uint8_t *buf) {
    for (int i = 0; i < COMPASS_KEM_N; i += 16) {
        // Read 8 bytes of PRF output
        __m128i t = _mm_loadl_epi64((__m128i*)&buf[i / 2]); 
        
        // Zero-extend 8 bytes into 8 x 32-bit lanes
        __m256i v32 = _mm256_cvtepu8_epi32(t); 
        
        // Extract low 4 bits (for Coeff 0) and high 4 bits (for Coeff 1)
        __m256i d0 = _mm256_and_si256(v32, _mm256_set1_epi32(0x0F));
        __m256i d1 = _mm256_and_si256(_mm256_srli_epi32(v32, 4), _mm256_set1_epi32(0x0F));
        
        // Interleave into 16 x 16-bit integers, low 4 bits contain the target bits
        __m256i v16 = _mm256_or_si256(d0, _mm256_slli_epi32(d1, 16));
        
        // Extract bit 0 and bit 1 and add (positive part a)
        __m256i a0 = _mm256_and_si256(v16, _mm256_set1_epi16(1));
        __m256i a1 = _mm256_and_si256(_mm256_srli_epi16(v16, 1), _mm256_set1_epi16(1));
        __m256i a = _mm256_add_epi16(a0, a1);
        
        // Extract bit 2 and bit 3 and add (negative part b)
        __m256i b0 = _mm256_and_si256(_mm256_srli_epi16(v16, 2), _mm256_set1_epi16(1));
        __m256i b1 = _mm256_and_si256(_mm256_srli_epi16(v16, 3), _mm256_set1_epi16(1));
        __m256i b = _mm256_add_epi16(b0, b1);
        
        // Final coefficient = a - b (range [-2, 2], sign in two's complement, NTT-compatible)
        __m256i coeff = _mm256_sub_epi16(a, b);
        
        _mm256_storeu_si256((__m256i*)&r[i], coeff);
    }
}
void poly_cbd3_avx(int16_t *r, const uint8_t *buf) {
    // Carefully crafted 256-bit byte-level shuffle mask to extract fragments for 16 coefficients from 12 bytes
    const __m256i shuf = _mm256_set_epi8(
        12, 11, 11, 10, 10,  9, 10,  9,
         9,  8,  8,  7,  7,  6,  7,  6,
         6,  5,  5,  4,  4,  3,  4,  3,
         3,  2,  2,  1,  1,  0,  1,  0
    );
    
    // Use left-shift trick: push misaligned 6-bit chunks to bits [10-15] of 16-bit words
    const __m256i shift_left = _mm256_set_epi16(
        1<<8, 1<<6, 1<<4, 1<<10,
        1<<8, 1<<6, 1<<4, 1<<10,
        1<<8, 1<<6, 1<<4, 1<<10,
        1<<8, 1<<6, 1<<4, 1<<10
    );

    for (int i = 0; i < COMPASS_KEM_N; i += 16) {
        // 16 coefficients from 12 bytes of data; read 16 bytes, extra 4 masked out
        __m128i t = _mm_loadu_si128((__m128i*)&buf[12 * i / 16]); 
        
        // Broadcast to high and low 128-bit lanes of 256-bit register
        __m256i v = _mm256_set_m128i(t, t);
        
        // Align irregular bytes into 16 x 16-bit lanes in one operation
        v = _mm256_shuffle_epi8(v, shuf);
        
        // Push all relevant 6-bit chunks to bits [10-15]
        v = _mm256_mullo_epi16(v, shift_left);
        // Unified right-shift by 10 bits; all 6-bit chunks now aligned at bits [0-5]
        v = _mm256_srli_epi16(v, 10);
        v = _mm256_and_si256(v, _mm256_set1_epi16(0x3F));

        // Extract first 3 bits and add (positive part a)
        __m256i a0 = _mm256_and_si256(v, _mm256_set1_epi16(1));
        __m256i a1 = _mm256_and_si256(_mm256_srli_epi16(v, 1), _mm256_set1_epi16(1));
        __m256i a2 = _mm256_and_si256(_mm256_srli_epi16(v, 2), _mm256_set1_epi16(1));
        __m256i a = _mm256_add_epi16(_mm256_add_epi16(a0, a1), a2);

        // Extract last 3 bits and add (negative part b)
        __m256i b0 = _mm256_and_si256(_mm256_srli_epi16(v, 3), _mm256_set1_epi16(1));
        __m256i b1 = _mm256_and_si256(_mm256_srli_epi16(v, 4), _mm256_set1_epi16(1));
        __m256i b2 = _mm256_and_si256(_mm256_srli_epi16(v, 5), _mm256_set1_epi16(1));
        __m256i b = _mm256_add_epi16(_mm256_add_epi16(b0, b1), b2);

        // Final coefficient = a - b (range [-3, 3])
        __m256i coeff = _mm256_sub_epi16(a, b);
        
        _mm256_storeu_si256((__m256i*)&r[i], coeff);
    }
}

void poly_cbd_eta1(poly *r, const uint8_t buf[COMPASS_KEM_ETA1*COMPASS_KEM_N/4]) {
#if (COMPASS_KEM_ETA1 == 2)
    poly_cbd2_avx(r->coeffs, buf);
#elif (COMPASS_KEM_ETA1 == 3)
    poly_cbd3_avx(r->coeffs, buf);
#elif COMPASS_KEM_ETA1 == 4
  // [Mod]: Add routing for eta=4
  cbd4(r, buf);
#else
    #error "Unsupported COMPASS_KEM_ETA1 for AVX2 CBD"
#endif
}

void poly_cbd_eta2(poly *r, const uint8_t buf[COMPASS_KEM_ETA2*COMPASS_KEM_N/4]) {
#if (COMPASS_KEM_ETA2 == 2)
    poly_cbd2_avx(r->coeffs, buf);
#elif (COMPASS_KEM_ETA2 == 3)
    poly_cbd3_avx(r->coeffs, buf);
#elif COMPASS_KEM_ETA2 == 4
  // [Mod]: Add routing for eta=4
  cbd4(r, buf);
#else
    #error "Unsupported COMPASS_KEM_ETA2 for AVX2 CBD"
#endif
}
