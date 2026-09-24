/* AVX2 compress/decompress for Weaver Table-2 parameters. */
#include <stdint.h>
#include <immintrin.h>
#include "params.h"

#if defined(WEAVER_USE_AVX_COMPRESS)

#include "poly.h"
#include "poly_compress_avx.h"

static inline __m128i poly_decompress_scale4(__m128i t4, unsigned d)
{
  uint16_t u[4];
  int16_t out[4];
  unsigned k;
  const uint16_t mask = (uint16_t)((1u << d) - 1u);
  const uint32_t bias = 1u << (d - 1);

  _mm_storel_epi64((__m128i *)u, _mm_and_si128(t4, _mm_set1_epi16(mask)));
  for(k = 0; k < 4; k++)
    out[k] = (int16_t)(((uint32_t)(u[k] & mask) * WEAVER_Q + bias) >> d);
  return _mm_set_epi16(0, 0, 0, 0, out[3], out[2], out[1], out[0]);
}

static inline __m128i poly_decompress_scale8(__m128i t8, unsigned d)
{
  __m128i lo = poly_decompress_scale4(t8, d);
  __m128i hi = poly_decompress_scale4(_mm_srli_si128(t8, 8), d);
  return _mm_unpacklo_epi64(lo, hi);
}

#if (WEAVER_Q == 7681)

#define COMPRESS_RECIP7681_D8  559168u
#define COMPRESS_RECIP7681_D9  1118336u
#define COMPRESS_RECIP7681_D10 2236671u
#define COMPRESS_RECIP7681_D11 2236671u

static inline __m256i compress7681_center16(__m256i f)
{
  const __m256i qv = _mm256_set1_epi16(WEAVER_Q);
  __m256i neg = _mm256_srai_epi16(f, 15);
  return _mm256_add_epi16(f, _mm256_and_si256(neg, qv));
}

static inline __m128i compress7681_center8(__m128i f8)
{
  const __m128i qv = _mm_set1_epi16(WEAVER_Q);
  __m128i neg = _mm_srai_epi16(f8, 15);
  return _mm_add_epi16(f8, _mm_and_si128(neg, qv));
}

static __attribute__((noinline)) __m128i compress7681_quant4_d10(__m128i f4)
{
  int16_t c[4];
  uint16_t q[4];
  unsigned k;

  _mm_storel_epi64((__m128i *)c, f4);
  for(k = 0; k < 4; k++) {
    int32_t u = c[k];
    u += (u >> 15) & WEAVER_Q;
    q[k] = (uint16_t)(((((uint64_t)(uint32_t)u << 10) + (WEAVER_Q / 2))
                     * COMPRESS_RECIP7681_D10 >> 34) & 1023u);
  }
  return _mm_set_epi16(0, 0, 0, 0, (short)q[3], (short)q[2], (short)q[1], (short)q[0]);
}

static __attribute__((noinline)) __m128i compress7681_quant4_d11(__m128i f4)
{
  int16_t c[4];
  uint16_t q[4];
  unsigned k;

  _mm_storel_epi64((__m128i *)c, f4);
  for(k = 0; k < 4; k++) {
    int32_t u = c[k];
    u += (u >> 15) & WEAVER_Q;
    q[k] = (uint16_t)(((((uint64_t)(uint32_t)u << 11) + (WEAVER_Q / 2))
                     * COMPRESS_RECIP7681_D11 >> 34) & 2047u);
  }
  return _mm_set_epi16(0, 0, 0, 0, (short)q[3], (short)q[2], (short)q[1], (short)q[0]);
}

static __attribute__((noinline)) __m128i compress7681_quant4_d8(__m128i f4)
{
  int16_t c[4];
  uint16_t q[4];
  unsigned k;

  _mm_storel_epi64((__m128i *)c, f4);
  for(k = 0; k < 4; k++) {
    int32_t u = c[k];
    u += (u >> 15) & WEAVER_Q;
    q[k] = (uint16_t)(((((uint64_t)(uint32_t)u << 8) + (WEAVER_Q / 2))
                     * COMPRESS_RECIP7681_D8 >> 32) & 255u);
  }
  return _mm_set_epi16(0, 0, 0, 0, (short)q[3], (short)q[2], (short)q[1], (short)q[0]);
}

static __attribute__((noinline)) __m128i compress7681_quant4_d9(__m128i f4)
{
  int16_t c[4];
  uint16_t q[4];
  unsigned k;

  _mm_storel_epi64((__m128i *)c, f4);
  for(k = 0; k < 4; k++) {
    int32_t u = c[k];
    u += (u >> 15) & WEAVER_Q;
    q[k] = (uint16_t)(((((uint64_t)(uint32_t)u << 9) + (WEAVER_Q / 2))
                     * COMPRESS_RECIP7681_D9 >> 33) & 511u);
  }
  return _mm_set_epi16(0, 0, 0, 0, (short)q[3], (short)q[2], (short)q[1], (short)q[0]);
}

static void poly_compress_pack4x10(uint8_t r[5], __m128i t)
{
  uint16_t v[4];
  _mm_storeu_si128((__m128i *)v, _mm_and_si128(t, _mm_set1_epi16(1023)));
  r[0] = (uint8_t)(v[0] >> 0);
  r[1] = (uint8_t)((v[0] >> 8) | (v[1] << 2));
  r[2] = (uint8_t)((v[1] >> 6) | (v[2] << 4));
  r[3] = (uint8_t)((v[2] >> 4) | (v[3] << 6));
  r[4] = (uint8_t)(v[3] >> 2);
}

static void poly_compress_pack8x11(uint8_t r[11], __m128i t)
{
  uint16_t v[8];
  _mm_storeu_si128((__m128i *)v, _mm_and_si128(t, _mm_set1_epi16(2047)));
  r[0]  = (uint8_t)(v[0] >> 0);
  r[1]  = (uint8_t)((v[0] >> 8) | (v[1] << 3));
  r[2]  = (uint8_t)((v[1] >> 5) | (v[2] << 6));
  r[3]  = (uint8_t)(v[2] >> 2);
  r[4]  = (uint8_t)((v[2] >> 10) | (v[3] << 1));
  r[5]  = (uint8_t)((v[3] >> 7) | (v[4] << 4));
  r[6]  = (uint8_t)((v[4] >> 4) | (v[5] << 7));
  r[7]  = (uint8_t)(v[5] >> 1);
  r[8]  = (uint8_t)((v[5] >> 9) | (v[6] << 2));
  r[9]  = (uint8_t)((v[6] >> 6) | (v[7] << 5));
  r[10] = (uint8_t)(v[7] >> 3);
}

static __m128i poly_decompress_unpack4x10(const uint8_t a[5])
{
  uint16_t v[8];
  v[0] = (uint16_t)a[0] | ((uint16_t)(a[1] & 3) << 8);
  v[1] = (uint16_t)(a[1] >> 2) | ((uint16_t)(a[2] & 15) << 6);
  v[2] = (uint16_t)(a[2] >> 4) | ((uint16_t)(a[3] & 63) << 4);
  v[3] = (uint16_t)(a[3] >> 6) | ((uint16_t)a[4] << 2);
  v[4] = v[5] = v[6] = v[7] = 0;
  return _mm_and_si128(_mm_loadu_si128((__m128i *)v), _mm_set1_epi16(1023));
}

static __m128i poly_decompress_unpack8x11(const uint8_t a[11])
{
  uint16_t v[8];
  v[0] = (uint16_t)a[0] | ((uint16_t)a[1] << 8);
  v[1] = (uint16_t)(a[1] >> 3) | ((uint16_t)a[2] << 5);
  v[2] = (uint16_t)(a[2] >> 6) | ((uint16_t)a[3] << 2) | ((uint16_t)a[4] << 10);
  v[3] = (uint16_t)(a[4] >> 1) | ((uint16_t)a[5] << 7);
  v[4] = (uint16_t)(a[5] >> 4) | ((uint16_t)a[6] << 4);
  v[5] = (uint16_t)(a[6] >> 7) | ((uint16_t)a[7] << 1) | ((uint16_t)a[8] << 9);
  v[6] = (uint16_t)(a[8] >> 2) | ((uint16_t)a[9] << 6);
  v[7] = (uint16_t)(a[9] >> 5) | ((uint16_t)a[10] << 3);
  return _mm_and_si128(_mm_loadu_si128((__m128i *)v), _mm_set1_epi16(2047));
}

#if (WEAVER_N == 256)

void poly_compress10_avx(uint8_t r[(WEAVER_N * 10) / 8], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 4; i++) {
    __m128i f4 = compress7681_center8(_mm_loadl_epi64((__m128i *)&a->coeffs[4 * i]));
    __m128i q4;

    q4 = compress7681_quant4_d10(f4);
    poly_compress_pack4x10(r + 5 * i, q4);
  }
}

void poly_decompress10_avx(poly *r, const uint8_t a[(WEAVER_N * 10) / 8])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 4; i++) {
    __m128i t4 = poly_decompress_unpack4x10(a + 5 * i);
    _mm_storel_epi64((__m128i *)&r->coeffs[4 * i], poly_decompress_scale4(t4, 10));
  }
}

#endif /* n=256 10-bit */

#if (WEAVER_N == 512)

void poly_compress11_avx(uint8_t r[(WEAVER_N * 11) / 8], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    __m128i f8 = compress7681_center8(_mm_loadu_si128((__m128i *)&a->coeffs[8 * i]));
    __m128i q8;

    q8 = _mm_unpacklo_epi64(compress7681_quant4_d11(f8),
                            compress7681_quant4_d11(_mm_srli_si128(f8, 8)));
    poly_compress_pack8x11(r + 11 * i, q8);
  }
}

void poly_decompress11_avx(poly *r, const uint8_t a[(WEAVER_N * 11) / 8])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    __m128i t8 = poly_decompress_unpack8x11(a + 11 * i);
    _mm_storeu_si128((__m128i *)&r->coeffs[8 * i], poly_decompress_scale8(t8, 11));
  }
}

#endif /* n=512 11-bit */

#if (WEAVER_DV == 8)

void poly_compress_d8_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    __m128i f8 = compress7681_center8(
        _mm_loadu_si128((__m128i *)&a->coeffs[8 * i]));
    __m128i q8 = _mm_unpacklo_epi64(compress7681_quant4_d8(f8),
                                    compress7681_quant4_d8(_mm_srli_si128(f8, 8)));
    _mm_storel_epi64((__m128i *)&r[8 * i],
                     _mm_packus_epi16(q8, _mm_setzero_si128()));
  }
}

void poly_decompress_d8_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N; i++)
    r->coeffs[i] = (int16_t)(((uint32_t)a[i] * WEAVER_Q + 128) >> 8);
}

#endif /* dv=8 */

#if (WEAVER_DV == 9)

static void poly_compress9_pack8_si128(uint8_t r[9], __m128i t)
{
  uint64_t bits;
  uint16_t v[8];

  t = _mm_and_si128(t, _mm_set1_epi16(511));
  _mm_storeu_si128((__m128i *)v, t);
  bits = (uint64_t)v[0]
       | ((uint64_t)v[1] << 9)
       | ((uint64_t)v[2] << 18)
       | ((uint64_t)v[3] << 27)
       | ((uint64_t)v[4] << 36)
       | ((uint64_t)v[5] << 45)
       | ((uint64_t)v[6] << 54)
       | ((uint64_t)v[7] << 63);
  r[0] = (uint8_t)(bits >> 0);
  r[1] = (uint8_t)(bits >> 8);
  r[2] = (uint8_t)(bits >> 16);
  r[3] = (uint8_t)(bits >> 24);
  r[4] = (uint8_t)(bits >> 32);
  r[5] = (uint8_t)(bits >> 40);
  r[6] = (uint8_t)(bits >> 48);
  r[7] = (uint8_t)(bits >> 56);
  r[8] = (uint8_t)(v[7] >> 1);
}

void poly_compress_d9_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    __m128i f8 = compress7681_center8(
        _mm_loadu_si128((__m128i *)&a->coeffs[8 * i]));
    __m128i q8 = _mm_unpacklo_epi64(compress7681_quant4_d9(f8),
                                    compress7681_quant4_d9(_mm_srli_si128(f8, 8)));

    poly_compress9_pack8_si128(r + 9 * i, q8);
  }
}

void poly_decompress_d9_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    uint16_t t[8];
    __m128i packed;

    t[0] = (uint16_t)a[0] | ((uint16_t)a[1] << 8);
    t[1] = (uint16_t)(a[1] >> 1) | ((uint16_t)a[2] << 7);
    t[2] = (uint16_t)(a[2] >> 2) | ((uint16_t)a[3] << 6);
    t[3] = (uint16_t)(a[3] >> 3) | ((uint16_t)a[4] << 5);
    t[4] = (uint16_t)(a[4] >> 4) | ((uint16_t)a[5] << 4);
    t[5] = (uint16_t)(a[5] >> 5) | ((uint16_t)a[6] << 3);
    t[6] = (uint16_t)(a[6] >> 6) | ((uint16_t)a[7] << 2);
    t[7] = (uint16_t)(a[7] >> 7) | ((uint16_t)a[8] << 1);
    a += 9;
    packed = _mm_and_si128(_mm_loadu_si128((__m128i *)t), _mm_set1_epi16(511));
    _mm_storeu_si128((__m128i *)&r->coeffs[8 * i], poly_decompress_scale8(packed, 9));
  }
}

#endif /* dv=9 */

#if (WEAVER_DV == 6)

static void poly_compress_pack4x6(uint8_t r[3], __m128i t)
{
  uint16_t v[4];
  _mm_storel_epi64((__m128i *)v, _mm_and_si128(t, _mm_set1_epi16(63)));
  r[0] = (uint8_t)(v[0] >> 0) | (uint8_t)(v[1] << 6);
  r[1] = (uint8_t)(v[1] >> 2) | (uint8_t)(v[2] << 4);
  r[2] = (uint8_t)(v[2] >> 4) | (uint8_t)(v[3] << 2);
}

static __m128i poly_decompress_unpack4x6(const uint8_t a[3])
{
  uint16_t v[4];
  v[0] = (uint16_t)a[0];
  v[1] = (uint16_t)(a[0] >> 6) | ((uint16_t)a[1] << 2);
  v[2] = (uint16_t)(a[1] >> 4) | ((uint16_t)a[2] << 4);
  v[3] = (uint16_t)(a[2] >> 2);
  return _mm_and_si128(_mm_loadl_epi64((__m128i *)v), _mm_set1_epi16(63));
}

static __m128i compress7681_quant4_d6(__m128i f4)
{
  int16_t c[4];
  uint16_t q[4];
  unsigned k;

  _mm_storel_epi64((__m128i *)c, f4);
  for(k = 0; k < 4; k++) {
    int32_t u = c[k];
    u += (u >> 15) & WEAVER_Q;
    q[k] = (uint16_t)(((((uint32_t)u << 6) + (WEAVER_Q / 2)) / WEAVER_Q) & 63u);
  }
  return _mm_set_epi16(0, 0, 0, 0, (short)q[3], (short)q[2], (short)q[1], (short)q[0]);
}

void poly_compress_d6_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 4; i++) {
    __m128i f4 = compress7681_center8(_mm_loadl_epi64((__m128i *)&a->coeffs[4 * i]));
    __m128i q4 = compress7681_quant4_d6(f4);
    poly_compress_pack4x6(r + 3 * i, q4);
  }
}

void poly_decompress_d6_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 4; i++) {
    __m128i t4 = poly_decompress_unpack4x6(a + 3 * i);
    _mm_storel_epi64((__m128i *)&r->coeffs[4 * i], poly_decompress_scale4(t4, 6));
  }
}

#endif /* dv=6 */

#endif /* WEAVER_Q == 7681 */

#if (WEAVER_DV == 4)

static inline __m128i compress_center8(__m128i f8)
{
  const __m128i qv = _mm_set1_epi16(WEAVER_Q);
  __m128i neg = _mm_srai_epi16(f8, 15);
  return _mm_add_epi16(f8, _mm_and_si128(neg, qv));
}

static __m128i compress_quant4_d4(__m128i f4)
{
  int16_t c[4];
  uint16_t q[4];
  unsigned k;

  _mm_storel_epi64((__m128i *)c, f4);
  for(k = 0; k < 4; k++) {
    int32_t u = c[k];
    u += (u >> 15) & WEAVER_Q;
    q[k] = (uint16_t)(((((uint32_t)u << 4) + (WEAVER_Q / 2)) / WEAVER_Q) & 15u);
  }
  return _mm_set_epi16(0, 0, 0, 0, (short)q[3], (short)q[2], (short)q[1], (short)q[0]);
}

static void poly_compress_pack4x4(uint8_t r[2], __m128i t)
{
  uint16_t v[4];

  _mm_storel_epi64((__m128i *)v, _mm_and_si128(t, _mm_set1_epi16(15)));
  r[0] = (uint8_t)(v[0] | (v[1] << 4));
  r[1] = (uint8_t)(v[2] | (v[3] << 4));
}

static __m128i poly_decompress_unpack4x4(const uint8_t a[2])
{
  uint16_t v[4];

  v[0] = (uint16_t)(a[0] & 15);
  v[1] = (uint16_t)(a[0] >> 4);
  v[2] = (uint16_t)(a[1] & 15);
  v[3] = (uint16_t)(a[1] >> 4);
  return _mm_loadl_epi64((__m128i *)v);
}

void poly_compress_d4_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    __m128i f8 = compress_center8(
        _mm_loadu_si128((__m128i *)&a->coeffs[8 * i]));

    poly_compress_pack4x4(r + 4 * i + 0, compress_quant4_d4(f8));
    poly_compress_pack4x4(r + 4 * i + 2, compress_quant4_d4(_mm_srli_si128(f8, 8)));
  }
}

void poly_decompress_d4_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    __m128i t4 = poly_decompress_unpack4x4(a + 4 * i + 0);
    _mm_storel_epi64((__m128i *)&r->coeffs[8 * i + 0],
                     poly_decompress_scale4(t4, 4));
    t4 = poly_decompress_unpack4x4(a + 4 * i + 2);
    _mm_storel_epi64((__m128i *)&r->coeffs[8 * i + 4],
                     poly_decompress_scale4(t4, 4));
  }
}

#endif /* dv=4 */

#if (WEAVER_DV == 5)

static inline __m128i compress3329_center8(__m128i f8)
{
  const __m128i qv = _mm_set1_epi16(WEAVER_Q);
  __m128i neg = _mm_srai_epi16(f8, 15);
  return _mm_add_epi16(f8, _mm_and_si128(neg, qv));
}

static __m128i compress3329_quant4_d5(__m128i f4)
{
  int16_t c[4];
  uint16_t q[4];
  unsigned k;

  _mm_storel_epi64((__m128i *)c, f4);
  for(k = 0; k < 4; k++) {
    int32_t u = c[k];
    u += (u >> 15) & WEAVER_Q;
    q[k] = (uint16_t)(((((uint32_t)u << 5) + (WEAVER_Q / 2)) / WEAVER_Q) & 31u);
  }
  return _mm_set_epi16(0, 0, 0, 0, (short)q[3], (short)q[2], (short)q[1], (short)q[0]);
}

static void poly_compress_pack8x5(uint8_t r[5], __m128i t)
{
  uint16_t v[8];
  _mm_storeu_si128((__m128i *)v, _mm_and_si128(t, _mm_set1_epi16(31)));
  r[0] = (uint8_t)(v[0] >> 0) | (uint8_t)(v[1] << 5);
  r[1] = (uint8_t)(v[1] >> 3) | (uint8_t)(v[2] << 2) | (uint8_t)(v[3] << 7);
  r[2] = (uint8_t)(v[3] >> 1) | (uint8_t)(v[4] << 4);
  r[3] = (uint8_t)(v[4] >> 4) | (uint8_t)(v[5] << 1) | (uint8_t)(v[6] << 6);
  r[4] = (uint8_t)(v[6] >> 2) | (uint8_t)(v[7] << 3);
}

static __m128i poly_decompress_unpack8x5(const uint8_t a[5])
{
  uint16_t v[8];
  v[0] = (uint16_t)a[0];
  v[1] = (uint16_t)(a[0] >> 5) | ((uint16_t)a[1] << 3);
  v[2] = (uint16_t)(a[1] >> 2);
  v[3] = (uint16_t)(a[1] >> 7) | ((uint16_t)a[2] << 1);
  v[4] = (uint16_t)(a[2] >> 4) | ((uint16_t)a[3] << 4);
  v[5] = (uint16_t)(a[3] >> 1);
  v[6] = (uint16_t)(a[3] >> 6) | ((uint16_t)a[4] << 2);
  v[7] = (uint16_t)(a[4] >> 3);
  return _mm_and_si128(_mm_loadu_si128((__m128i *)v), _mm_set1_epi16(31));
}

void poly_compress_d5_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    __m128i f8 = compress3329_center8(
        _mm_loadu_si128((__m128i *)&a->coeffs[8 * i]));
    __m128i q8 = _mm_unpacklo_epi64(compress3329_quant4_d5(f8),
                                    compress3329_quant4_d5(_mm_srli_si128(f8, 8)));
    poly_compress_pack8x5(r + 5 * i, q8);
  }
}

void poly_decompress_d5_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 8; i++) {
    __m128i t8 = poly_decompress_unpack8x5(a + 5 * i);
    _mm_storeu_si128((__m128i *)&r->coeffs[8 * i], poly_decompress_scale8(t8, 5));
  }
  for(i = WEAVER_N / 8 * 8; i < WEAVER_N; i++)
    r->coeffs[i] = 0;
}

#endif /* dv=5 */

#if (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 9 / 8) || \
     WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 9 / 8))

#include "poly_compress9.h"

#if (WEAVER_Q == 3329)
#define COMPRESS9_RECIP 1290168ULL
#else
#define COMPRESS9_RECIP 559168ULL
#endif

static void poly_compress9_pack8_si128(uint8_t r[9], __m128i t)
{
  uint64_t bits;
  uint16_t v[8];

  t = _mm_and_si128(t, _mm_set1_epi16(511));
  _mm_storeu_si128((__m128i *)v, t);
  bits = (uint64_t)v[0]
       | ((uint64_t)v[1] << 9)
       | ((uint64_t)v[2] << 18)
       | ((uint64_t)v[3] << 27)
       | ((uint64_t)v[4] << 36)
       | ((uint64_t)v[5] << 45)
       | ((uint64_t)v[6] << 54)
       | ((uint64_t)v[7] << 63);
  r[0] = (uint8_t)(bits >> 0);
  r[1] = (uint8_t)(bits >> 8);
  r[2] = (uint8_t)(bits >> 16);
  r[3] = (uint8_t)(bits >> 24);
  r[4] = (uint8_t)(bits >> 32);
  r[5] = (uint8_t)(bits >> 40);
  r[6] = (uint8_t)(bits >> 48);
  r[7] = (uint8_t)(bits >> 56);
  r[8] = (uint8_t)(v[7] >> 1);
}

static __m128i poly_compress9_quant8_avx(__m128i f16)
{
  const __m256i qv = _mm256_set1_epi32(WEAVER_Q);
  const __m256i half = _mm256_set1_epi32(WEAVER_Q / 2);
  const __m256i recip = _mm256_set1_epi64x(COMPRESS9_RECIP);
  __m256i u, neg, p0, p1;
  uint32_t q[8];

  u = _mm256_cvtepi16_epi32(f16);
  neg = _mm256_srai_epi32(u, 31);
  u = _mm256_add_epi32(u, _mm256_and_si256(neg, qv));
  u = _mm256_and_si256(u, _mm256_set1_epi32(0xffff));
  u = _mm256_add_epi32(_mm256_slli_epi32(u, 9), half);
  p0 = _mm256_mul_epu32(u, recip);
  p1 = _mm256_mul_epu32(_mm256_srli_si256(u, 4), recip);
  q[0] = (uint32_t)_mm256_extract_epi32(p0, 1);
  q[2] = (uint32_t)_mm256_extract_epi32(p0, 3);
  q[4] = (uint32_t)_mm256_extract_epi32(p0, 5);
  q[6] = (uint32_t)_mm256_extract_epi32(p0, 7);
  q[1] = (uint32_t)_mm256_extract_epi32(p1, 1);
  q[3] = (uint32_t)_mm256_extract_epi32(p1, 3);
  q[5] = (uint32_t)_mm256_extract_epi32(p1, 5);
  q[7] = (uint32_t)_mm256_extract_epi32(p1, 7);
  return _mm_set_epi16((short)(q[7] & 511), (short)(q[6] & 511),
                       (short)(q[5] & 511), (short)(q[4] & 511),
                       (short)(q[3] & 511), (short)(q[2] & 511),
                       (short)(q[1] & 511), (short)(q[0] & 511));
}

static __m256i poly_compress9_quant16_avx(__m256i f0)
{
  return _mm256_set_m128i(
      poly_compress9_quant8_avx(_mm256_extracti128_si256(f0, 1)),
      poly_compress9_quant8_avx(_mm256_castsi256_si128(f0)));
}

void poly_compress9_quant_avx(uint16_t t[WEAVER_N], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N; i += 16) {
    __m256i q = poly_compress9_quant16_avx(
        _mm256_loadu_si256((__m256i *)&a->coeffs[i]));
    _mm_storeu_si128((__m128i *)&t[i + 0], _mm256_castsi256_si128(q));
    _mm_storeu_si128((__m128i *)&t[i + 8], _mm256_extracti128_si256(q, 1));
  }
}

void poly_compress9_avx(uint8_t r[(WEAVER_N * 9) / 8], const poly *a)
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 16; i++) {
    __m256i q = poly_compress9_quant16_avx(
        _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]));
    poly_compress9_pack8_si128(r + 18 * i + 0, _mm256_castsi256_si128(q));
    poly_compress9_pack8_si128(r + 18 * i + 9, _mm256_extracti128_si256(q, 1));
  }
}

#endif /* 9-bit pk/c1 */

#endif /* WEAVER_USE_AVX_COMPRESS */
