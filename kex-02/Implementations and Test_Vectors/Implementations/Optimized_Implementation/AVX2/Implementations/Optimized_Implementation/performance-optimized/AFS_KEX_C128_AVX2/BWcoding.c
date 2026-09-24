#include <stdint.h>
#include <immintrin.h>
#include "BWcoding.h"

/*
 * AVX2 D8 codec for BW_KEM_C128.
 *
 * The D8 codec used by BW_KEM_C128 is algorithmically identical to the E8
 * codec used by CTRU-Prime; the only differences are the modulus
 * (BWKEM128_QHAT = 4096 instead of CTRU_Q2 = 1024) and the input element
 * type (int16_t coefficients in [0, BWKEM128_QHAT - 1] instead of uint32_t
 * vec[8]). On the encode side the BW_KEM codec additionally bit-spreads
 * each 8-bit codeword into 8 int16_t coefficients in {0, KYBER_Q/2 = 1664}.
 *
 * This implementation is ported from CTRU-Prime-AVX/avx2/avx2_coding.c with
 * those adaptations. It is bit-exact with the previous reference scalar
 * implementation (verified via test_codec.c).
 */

#define BWKEM128_CODEC_BLOCK_COEFFS 8
#define BWKEM128_CODEC_LEVEL (KYBER_Q >> 1)

/*
 * 4-bit nibble -> 8-bit codeword lookup table.
 *
 * Equivalent to the previous scalar bwkem128_encode_codeword() which XORs
 * generator masks {0x55, 0x0F, 0x3C, 0xF0} according to the bits of the
 * nibble. The 16 entries below were independently produced by both forms
 * and verified to match in the codec harness.
 */
static const uint8_t bwkem128_encode_table_data[16] __attribute__((aligned(16))) = {
    0x00, 0x55, 0x0F, 0x5A, 0x3C, 0x69, 0x33, 0x66,
    0xF0, 0xA5, 0xFF, 0xAA, 0xCC, 0x99, 0xC3, 0x96
};

static inline __m128i bwkem128_min_with_index_4(__m128i v)
{
  __m128i v_shuf = _mm_shuffle_epi32(v, 0xB1);
  __m128i min1 = _mm_min_epi32(v, v_shuf);
  __m128i min1_s = _mm_shuffle_epi32(min1, 0x4E);
  __m128i min_all = _mm_min_epi32(min1, min1_s);

  __m128i mask = _mm_cmpeq_epi32(v, min_all);
  int mask_bits = _mm_movemask_ps(_mm_castsi128_ps(mask));
  int min_idx = __builtin_ctz((unsigned)mask_bits);

  return _mm_set_epi32(0, 0, min_idx, _mm_cvtsi128_si32(min_all));
}

static inline uint8_t bwkem128_decode_d8_core_avx2(__m128i c0_vec,
                                                   __m128i c1_vec,
                                                   uint32_t *cost_out)
{
  __m128i cmp = _mm_cmpgt_epi32(c0_vec, c1_vec);
  int r_mask = _mm_movemask_ps(_mm_castsi128_ps(cmp));
  uint8_t m0 = (uint8_t)(r_mask & 0xF);

  uint8_t xor_sum = (uint8_t)(__builtin_popcount((unsigned)m0) & 1);

  __m128i tmp = _mm_min_epi32(c0_vec, c1_vec);
  __m128i tmp_xor = _mm_max_epi32(c0_vec, c1_vec);

  __m128i sum_tmp = _mm_hadd_epi32(tmp, tmp);
  sum_tmp = _mm_hadd_epi32(sum_tmp, sum_tmp);
  uint32_t total_tmp = (uint32_t)_mm_cvtsi128_si32(sum_tmp);

  __m128i diffs = _mm_sub_epi32(tmp_xor, tmp);
  __m128i res_min = bwkem128_min_with_index_4(diffs);
  uint32_t min_val = (uint32_t)_mm_cvtsi128_si32(res_min);
  uint32_t min_idx = (uint32_t)_mm_extract_epi32(res_min, 1);

  uint8_t m1 = (uint8_t)(m0 ^ (1u << min_idx));

  *cost_out = total_tmp;
  if(xor_sum) {
    *cost_out += min_val;
    return m1;
  }
  return m0;
}

static uint8_t bwkem128_decode_block_avx2(const int16_t in[BWKEM128_CODEC_BLOCK_COEFFS])
{
  uint32_t cost_total[2];
  uint8_t m_out[2];
  uint8_t res;
  uint32_t r;

  /* Zero-extend 8 int16_t -> 8 uint32_t and mask into [0, QHAT-1]. */
  __m128i v16 = _mm_loadu_si128((const __m128i *)in);
  __m256i v = _mm256_cvtepu16_epi32(v16);
  v = _mm256_and_si256(v, _mm256_set1_epi32(BWKEM128_QHAT - 1));

  __m256i q2 = _mm256_set1_epi32(BWKEM128_QHAT);
  __m256i half_q2 = _mm256_set1_epi32(BWKEM128_QHAT_HALF);

  /* Per-coordinate cost: |v|_QHAT^2 (distance to 0 mod QHAT). */
  __m256i v_diff = _mm256_sub_epi32(q2, v);
  __m256i abs_q2_v = _mm256_min_epi32(v, v_diff);
  __m256i cost0_v = _mm256_mullo_epi32(abs_q2_v, abs_q2_v);

  /* Per-coordinate cost: |v - QHAT/2|^2 (distance to QHAT/2). */
  __m256i v_sub = _mm256_sub_epi32(v, half_q2);
  __m256i v_const_abs = _mm256_abs_epi32(v_sub);
  __m256i cost1_v = _mm256_mullo_epi32(v_const_abs, v_const_abs);

  __m128i a_lo = _mm256_castsi256_si128(cost0_v);
  __m128i a_hi = _mm256_extracti128_si256(cost0_v, 1);
  __m128i b_lo = _mm256_castsi256_si128(cost1_v);
  __m128i b_hi = _mm256_extracti128_si256(cost1_v, 1);

  /* coset10 = 0: per-pair (cost0[2k]+cost0[2k+1], cost1[2k]+cost1[2k+1]). */
  {
    __m128i low = _mm_hadd_epi32(a_lo, b_lo);
    __m128i high = _mm_hadd_epi32(a_hi, b_hi);
    __m128i c0_vec = _mm_unpacklo_epi64(low, high);
    __m128i c1_vec = _mm_unpacklo_epi64(_mm_srli_si128(low, 8),
                                        _mm_srli_si128(high, 8));
    m_out[0] = bwkem128_decode_d8_core_avx2(c0_vec, c1_vec, &cost_total[0]);
  }

  /* coset10 = 1: per-pair (cost1[even]+cost0[odd], cost0[even]+cost1[odd]). */
  {
    __m128i a_even_lo = _mm_shuffle_epi32(a_lo, 0x88);
    __m128i a_odd_lo = _mm_shuffle_epi32(a_lo, 0xDD);
    __m128i b_even_lo = _mm_shuffle_epi32(b_lo, 0x88);
    __m128i b_odd_lo = _mm_shuffle_epi32(b_lo, 0xDD);

    __m128i a_even_hi = _mm_shuffle_epi32(a_hi, 0x88);
    __m128i a_odd_hi = _mm_shuffle_epi32(a_hi, 0xDD);
    __m128i b_even_hi = _mm_shuffle_epi32(b_hi, 0x88);
    __m128i b_odd_hi = _mm_shuffle_epi32(b_hi, 0xDD);

    __m128i c0_low = _mm_add_epi32(b_even_lo, a_odd_lo);
    __m128i c1_low = _mm_add_epi32(a_even_lo, b_odd_lo);
    __m128i c0_high = _mm_add_epi32(b_even_hi, a_odd_hi);
    __m128i c1_high = _mm_add_epi32(a_even_hi, b_odd_hi);

    __m128i c0_vec = _mm_unpacklo_epi64(c0_low, c0_high);
    __m128i c1_vec = _mm_unpacklo_epi64(c1_low, c1_high);
    m_out[1] = bwkem128_decode_d8_core_avx2(c0_vec, c1_vec, &cost_total[1]);
  }

  /* Choose the coset with smaller total cost (ties favour coset 0). */
  r = ((cost_total[1] - cost_total[0]) >> 31) & 1u;
  res = (uint8_t)(((-(r ^ 1)) & (uint32_t)m_out[0]) ^
                  ((-(r & 1)) & (uint32_t)m_out[1]));
  /* Convert the 4-bit pair-label vector back to the 4-bit message nibble. */
  res = (uint8_t)(((((res ^ (res << 1)) & 0x3u) | ((res >> 1) & 0x4u)) << 1) | r);
  return res;
}

static inline void bwkem128_codeword_to_int16(int16_t out[BWKEM128_CODEC_BLOCK_COEFFS],
                                              uint8_t cw)
{
  unsigned int i;
  for(i = 0; i < BWKEM128_CODEC_BLOCK_COEFFS; i++)
    out[i] = (int16_t)(((cw >> i) & 1u) * BWKEM128_CODEC_LEVEL);
}

void codec_encode(int16_t out[BWKEM128_N],
                  const uint8_t msg[BWKEM128_INDCPA_MSGBYTES])
{
  /* Spread 16 message bytes into 32 4-bit nibble indices. */
  uint8_t nibbles[32] __attribute__((aligned(32)));
  unsigned int i;

  for(i = 0; i < BWKEM128_INDCPA_MSGBYTES; i++) {
    nibbles[2u * i + 0u] = (uint8_t)(msg[i] & 0x0fu);
    nibbles[2u * i + 1u] = (uint8_t)(msg[i] >> 4);
  }

  /* Single 32-byte AVX2 lookup converts all 32 nibble indices into codewords. */
  __m128i tbl128 = _mm_load_si128((const __m128i *)bwkem128_encode_table_data);
  __m256i tbl = _mm256_broadcastsi128_si256(tbl128);
  __m256i mask4 = _mm256_set1_epi8(0x0f);

  __m256i n = _mm256_load_si256((const __m256i *)nibbles);
  n = _mm256_and_si256(n, mask4);
  __m256i cw = _mm256_shuffle_epi8(tbl, n);

  uint8_t codewords[32] __attribute__((aligned(32)));
  _mm256_store_si256((__m256i *)codewords, cw);

  /* Bit-spread each codeword into 8 int16_t coefficients in {0, KYBER_Q/2}. */
  for(i = 0; i < 32u; i++)
    bwkem128_codeword_to_int16(out + BWKEM128_CODEC_BLOCK_COEFFS * i,
                               codewords[i]);
}

void codec_decode(uint8_t msg[BWKEM128_INDCPA_MSGBYTES],
                  const int16_t in[BWKEM128_N])
{
  unsigned int i;

  for(i = 0; i < BWKEM128_INDCPA_MSGBYTES; i++) {
    unsigned int base = 2u * i * BWKEM128_CODEC_BLOCK_COEFFS;
    uint8_t lo = bwkem128_decode_block_avx2(in + base);
    uint8_t hi = bwkem128_decode_block_avx2(in + base + BWKEM128_CODEC_BLOCK_COEFFS);
    msg[i] = (uint8_t)(lo | (uint8_t)(hi << 4));
  }
}
