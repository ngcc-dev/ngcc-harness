#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "BWcoding.h"

#define lambda (1 << KYBER_EQ)
#define mod ((lambda << 2) - 1)

static uint32_t extract_masked_bits_portable(uint64_t value, uint64_t mask)
{
    uint32_t result = 0;
    unsigned output_bit = 0;

    while (mask != 0) {
        uint64_t lowest_mask_bit = mask & (~mask + 1);

        if ((value & lowest_mask_bit) != 0) {
            result |= (uint32_t)1U << output_bit;
        }
        mask &= mask - 1;
        output_bit++;
    }

    return result;
}

__attribute__((target("bmi2")))
static uint32_t extract_masked_bits_bmi2(uint64_t value)
{
    return (uint32_t)_pext_u64(value, 0xfdd5d554d5545440ULL);
}

static uint32_t extract_masked_bits(uint64_t value)
{
    if (__builtin_cpu_supports("bmi2")) {
        return extract_masked_bits_bmi2(value);
    }
    return extract_masked_bits_portable(value, 0xfdd5d554d5545440ULL);
}

static uint32_t delabel_bw32_avx2(const int16_t w[32])
{
    uint64_t t = 0;
    uint8_t mvec[32];
    uint16_t vec[32] __attribute__((aligned(32)));

    __m256i w_lo = _mm256_loadu_si256((const __m256i *)w);
    __m256i w_hi = _mm256_loadu_si256((const __m256i *)(w + 16));
    const __m256i mod_mask = _mm256_set1_epi16(mod);

    w_lo = _mm256_srli_epi16(_mm256_and_si256(w_lo, mod_mask), KYBER_EQ);
    w_hi = _mm256_srli_epi16(_mm256_and_si256(w_hi, mod_mask), KYBER_EQ);
    _mm256_store_si256((__m256i *)vec, w_lo);
    _mm256_store_si256((__m256i *)(vec + 16), w_hi);

    mvec[0] = (uint8_t) vec[0] & 3;
    mvec[1] = (uint8_t) vec[1] & 3;
    mvec[2] = (uint8_t) ((vec[2] + vec[3] - vec[0] - vec[1]) >> 1) & 3;
    mvec[3] = (uint8_t) ((vec[0] + vec[3] - vec[1] - vec[2]) >> 1) & 3;
    mvec[4] = (uint8_t) ((vec[4] + vec[5] - vec[0] - vec[1]) >> 1) & 3;
    mvec[5] = (uint8_t) ((vec[0] + vec[5] - vec[1] - vec[4]) >> 1) & 3;
    mvec[6] = (uint8_t) ((vec[1] + vec[7] - vec[3] - vec[5]) >> 1) & 3;
    mvec[7] = (uint8_t) ((vec[2] + vec[4] - vec[0] - vec[6]) >> 1) & 3;
    mvec[8] = (uint8_t) ((vec[8] + vec[9] - vec[0] - vec[1]) >> 1) & 3;
    mvec[9] = (uint8_t) ((vec[0] + vec[9] - vec[1] - vec[8]) >> 1) & 3;
    mvec[10] = (uint8_t) ((vec[1] + vec[11] - vec[3] - vec[9]) >> 1) & 3;
    mvec[11] = (uint8_t) ((vec[2] + vec[8] - vec[0] - vec[10]) >> 1) & 3;
    mvec[12] = (uint8_t) ((vec[1] + vec[13] - vec[5] - vec[9]) >> 1) & 3;
    mvec[13] = (uint8_t) ((vec[4] + vec[8] - vec[0] - vec[12]) >> 1) & 3;
    mvec[14] = (uint8_t) ((vec[0] + vec[3] + vec[5] + vec[6] + vec[9] + vec[10] + vec[12] + vec[15] - vec[1] - vec[2] - vec[4] - vec[7] - vec[8] - vec[11] - vec[13] - vec[14]) >> 2) & 3;
    mvec[15] = (uint8_t) ((vec[0] + vec[1] + vec[6] + vec[7] + vec[10] + vec[11] + vec[12] + vec[13] - vec[2] - vec[3] - vec[4] - vec[5] - vec[8] - vec[9] - vec[14] - vec[15]) >> 2) & 3;
    mvec[16] = (uint8_t) ((vec[16] + vec[17] - vec[0] - vec[1]) >> 1) & 3;
    mvec[17] = (uint8_t) ((vec[0] + vec[17] - vec[1] - vec[16]) >> 1) & 3;
    mvec[18] = (uint8_t) ((vec[1] + vec[19] - vec[3] - vec[17]) >> 1) & 3;
    mvec[19] = (uint8_t) ((vec[2] + vec[16] - vec[0] - vec[18]) >> 1) & 3;
    mvec[20] = (uint8_t) ((vec[1] + vec[21] - vec[5] - vec[17]) >> 1) & 3;
    mvec[21] = (uint8_t) ((vec[4] + vec[16] - vec[0] - vec[20]) >> 1) & 3;
    mvec[22] = (uint8_t) ((vec[0] + vec[3] + vec[5] + vec[6] + vec[17] + vec[18] + vec[20] + vec[23] - vec[1] - vec[2] - vec[4] - vec[7] - vec[16] - vec[19] - vec[21] - vec[22]) >> 2) & 3;
    mvec[23] = (uint8_t) ((vec[0] + vec[1] + vec[6] + vec[7] + vec[18] + vec[19] + vec[20] + vec[21] - vec[2] - vec[3] - vec[4] - vec[5] - vec[16] - vec[17] - vec[22] - vec[23]) >> 2) & 3;
    mvec[24] = (uint8_t) ((vec[1] + vec[25] - vec[9] - vec[17]) >> 1) & 3;
    mvec[25] = (uint8_t) ((vec[8] + vec[16] - vec[0] - vec[24]) >> 1) & 3;
    mvec[26] = (uint8_t) ((vec[0] + vec[3] + vec[9] + vec[10] + vec[17] + vec[18] + vec[24] + vec[27] - vec[1] - vec[2] - vec[8] - vec[11] - vec[16] - vec[19] - vec[25] - vec[26]) >> 2) & 3;
    mvec[27] = (uint8_t) ((vec[0] + vec[1] + vec[10] + vec[11] + vec[18] + vec[19] + vec[24] + vec[25] - vec[2] - vec[3] - vec[8] - vec[9] - vec[16] - vec[17] - vec[26] - vec[27]) >> 2) & 3;
    mvec[28] = (uint8_t) ((vec[0] + vec[5] + vec[9] + vec[12] + vec[17] + vec[20] + vec[24] + vec[29] - vec[1] - vec[4] - vec[8] - vec[13] - vec[16] - vec[21] - vec[25] - vec[28]) >> 2) & 3;
    mvec[29] = (uint8_t) ((vec[0] + vec[1] + vec[12] + vec[13] + vec[20] + vec[21] + vec[24] + vec[25] - vec[4] - vec[5] - vec[8] - vec[9] - vec[16] - vec[17] - vec[28] - vec[29]) >> 2) & 3;
    mvec[30] = (uint8_t) ((vec[2] + vec[4] + vec[8] + vec[14] + vec[16] + vec[22] + vec[26] + vec[28] - vec[0] - vec[6] - vec[10] - vec[12] - vec[18] - vec[20] - vec[24] - vec[30]) >> 2) & 3;
    mvec[31] = (uint8_t) ((vec[3] + vec[5] + vec[9] + vec[15] + vec[17] + vec[23] + vec[27] + vec[29] - vec[1] - vec[7] - vec[11] - vec[13] - vec[19] - vec[21] - vec[25] - vec[31]) >> 2) & 3;

    for (int i = 0; i < 32; i++) {
        t |= (uint64_t)(mvec[31 - i]) << (2 * i);
    }

    t = (t & 0xfdd5d554d5545440ULL) ^ ((t & 0x0220200120010110ULL) << 2);
    return extract_masked_bits(t);
}

static inline __m256i pack_i32_to_i16_ordered(__m256i lo, __m256i hi)
{
    return _mm256_permute4x64_epi64(_mm256_packs_epi32(lo, hi), 0xD8);
}

static inline __m256i bdd2_avx2_exact(__m256i t)
{
    __m256i half = _mm256_set1_epi32(lambda >> 1);
    __m256i lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t));
    __m256i hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t, 1));

    lo = _mm256_slli_epi32(_mm256_srai_epi32(_mm256_add_epi32(lo, half), KYBER_EQ), KYBER_EQ);
    hi = _mm256_slli_epi32(_mm256_srai_epi32(_mm256_add_epi32(hi, half), KYBER_EQ), KYBER_EQ);

    return pack_i32_to_i16_ordered(lo, hi);
}

static inline __m256i mul_phi_inv_avx2_exact(__m256i y)
{
    __m256i lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(y));
    __m256i hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(y, 1));
    __m256i lo_swapped = _mm256_shuffle_epi32(lo, 0xB1);
    __m256i hi_swapped = _mm256_shuffle_epi32(hi, 0xB1);
    __m256i lo_sum = _mm256_srai_epi32(_mm256_add_epi32(lo, lo_swapped), 1);
    __m256i hi_sum = _mm256_srai_epi32(_mm256_add_epi32(hi, hi_swapped), 1);
    __m256i lo_diff = _mm256_srai_epi32(_mm256_sub_epi32(lo, lo_swapped), 1);
    __m256i hi_diff = _mm256_srai_epi32(_mm256_sub_epi32(hi, hi_swapped), 1);

    lo = _mm256_blend_epi32(lo_sum, lo_diff, 0xAA);
    hi = _mm256_blend_epi32(hi_sum, hi_diff, 0xAA);

    return pack_i32_to_i16_ordered(lo, hi);
}

static inline __m256i mul_phi_avx2(__m256i y)
{
    __m256i swapped = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(y, 0xB1), 0xB1);
    __m256i sum = _mm256_add_epi16(y, swapped);
    __m256i diff = _mm256_sub_epi16(y, swapped);
    return _mm256_blend_epi16(diff, sum, 0xAA);
}

static inline __m256i select_second_mask_epi32(__m256i dis1, __m256i dis2)
{
    return _mm256_srai_epi32(_mm256_sub_epi32(dis2, dis1), 31);
}

static inline __m256i bdd4_avx2_exact(__m256i t)
{
    __m256i y = bdd2_avx2_exact(t);
    __m256i t_swap = _mm256_shuffle_epi32(t, 0xB1);
    __m256i tmp = _mm256_sub_epi16(t_swap, y);
    __m256i z = bdd2_avx2_exact(mul_phi_inv_avx2_exact(tmp));
    __m256i w = _mm256_add_epi16(mul_phi_avx2(z), y);
    __m256i dy = _mm256_sub_epi16(y, t);
    __m256i dw = _mm256_sub_epi16(w, t_swap);
    __m256i dis = _mm256_add_epi32(_mm256_madd_epi16(dy, dy),
                                   _mm256_madd_epi16(dw, dw));
    __m256i dis_swap = _mm256_shuffle_epi32(dis, 0xB1);
    __m256i mask = _mm256_shuffle_epi32(select_second_mask_epi32(dis, dis_swap), 0xA0);
    __m256i w_swap = _mm256_shuffle_epi32(w, 0xB1);
    __m256i opt0 = _mm256_blend_epi32(y, w_swap, 0xAA);
    __m256i opt1 = _mm256_blend_epi32(w_swap, y, 0xAA);

    return _mm256_blendv_epi8(opt0, opt1, mask);
}

static inline __m256i bdd8_avx2_exact(__m256i t)
{
    __m256i y = bdd4_avx2_exact(t);
    __m256i t_swap = _mm256_shuffle_epi32(t, 0x4E);
    __m256i tmp = _mm256_sub_epi16(t_swap, y);
    __m256i z = bdd4_avx2_exact(mul_phi_inv_avx2_exact(tmp));
    __m256i w = _mm256_add_epi16(mul_phi_avx2(z), y);
    __m256i dy = _mm256_sub_epi16(y, t);
    __m256i dw = _mm256_sub_epi16(w, t_swap);
    __m256i dis_parts = _mm256_add_epi32(_mm256_madd_epi16(dy, dy),
                                         _mm256_madd_epi16(dw, dw));
    __m256i dis = _mm256_add_epi32(dis_parts, _mm256_shuffle_epi32(dis_parts, 0xB1));
    __m256i dis_swap = _mm256_shuffle_epi32(dis, 0x4E);
    __m256i mask = _mm256_shuffle_epi32(select_second_mask_epi32(dis, dis_swap), 0x00);
    __m256i w_swap = _mm256_shuffle_epi32(w, 0x4E);
    __m256i opt0 = _mm256_blend_epi32(y, w_swap, 0xCC);
    __m256i opt1 = _mm256_blend_epi32(w_swap, y, 0xCC);

    return _mm256_blendv_epi8(opt0, opt1, mask);
}

static inline __m256i bdd16_avx2_exact(__m256i t)
{
    __m256i y = bdd8_avx2_exact(t);
    __m256i t_swap = _mm256_permute2x128_si256(t, t, 1);
    __m256i tmp = _mm256_sub_epi16(t_swap, y);
    __m256i z = bdd8_avx2_exact(mul_phi_inv_avx2_exact(tmp));
    __m256i w = _mm256_add_epi16(mul_phi_avx2(z), y);
    __m256i dy = _mm256_sub_epi16(y, t);
    __m256i dw = _mm256_sub_epi16(w, t_swap);
    __m256i dis_parts = _mm256_add_epi32(_mm256_madd_epi16(dy, dy),
                                         _mm256_madd_epi16(dw, dw));
    __m256i sum1 = _mm256_add_epi32(dis_parts, _mm256_shuffle_epi32(dis_parts, 0xB1));
    __m256i dis = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4E));
    __m256i dis_swap = _mm256_permute2x128_si256(dis, dis, 1);
    __m256i mask = _mm256_permute2x128_si256(select_second_mask_epi32(dis, dis_swap),
                                             select_second_mask_epi32(dis, dis_swap),
                                             0x00);
    __m256i w_swap = _mm256_permute2x128_si256(w, w, 1);
    __m256i opt0 = _mm256_blend_epi32(y, w_swap, 0xF0);
    __m256i opt1 = _mm256_blend_epi32(w_swap, y, 0xF0);

    return _mm256_blendv_epi8(opt0, opt1, mask);
}

static inline void bdd32_avx2_exact(int16_t w_out[32], __m256i t_lo, __m256i t_hi)
{
    __m256i y1 = bdd16_avx2_exact(t_lo);
    __m256i y2 = bdd16_avx2_exact(t_hi);
    __m256i z1 = bdd16_avx2_exact(mul_phi_inv_avx2_exact(_mm256_sub_epi16(t_hi, y1)));
    __m256i z2 = bdd16_avx2_exact(mul_phi_inv_avx2_exact(_mm256_sub_epi16(t_lo, y2)));
    __m256i w1 = _mm256_add_epi16(mul_phi_avx2(z1), y1);
    __m256i w2 = _mm256_add_epi16(mul_phi_avx2(z2), y2);
    __m256i dy1 = _mm256_sub_epi16(y1, t_lo);
    __m256i dw1 = _mm256_sub_epi16(w1, t_hi);
    __m256i dy2 = _mm256_sub_epi16(y2, t_hi);
    __m256i dw2 = _mm256_sub_epi16(w2, t_lo);
    __m256i dis1_parts = _mm256_add_epi32(_mm256_madd_epi16(dy1, dy1),
                                          _mm256_madd_epi16(dw1, dw1));
    __m256i dis2_parts = _mm256_add_epi32(_mm256_madd_epi16(dy2, dy2),
                                          _mm256_madd_epi16(dw2, dw2));
    __m256i sum1 = _mm256_add_epi32(dis1_parts, _mm256_shuffle_epi32(dis1_parts, 0xB1));
    __m256i sum2 = _mm256_add_epi32(dis2_parts, _mm256_shuffle_epi32(dis2_parts, 0xB1));
    sum1 = _mm256_add_epi32(sum1, _mm256_shuffle_epi32(sum1, 0x4E));
    sum2 = _mm256_add_epi32(sum2, _mm256_shuffle_epi32(sum2, 0x4E));
    sum1 = _mm256_add_epi32(sum1, _mm256_permute2x128_si256(sum1, sum1, 1));
    sum2 = _mm256_add_epi32(sum2, _mm256_permute2x128_si256(sum2, sum2, 1));

    __m256i mask = select_second_mask_epi32(sum1, sum2);
    _mm256_storeu_si256((__m256i *)w_out, _mm256_blendv_epi8(y1, w2, mask));
    _mm256_storeu_si256((__m256i *)(w_out + 16), _mm256_blendv_epi8(w1, y2, mask));
}

static inline __m256i bw32_add_mod4_256(__m256i a, __m256i b)
{
    __m256i mask_lo = _mm256_set1_epi64x(0x5555555555555555ULL);
    __m256i mask_hi = _mm256_set1_epi64x(0xaaaaaaaaaaaaaaaaULL);
    __m256i t = _mm256_add_epi64(_mm256_and_si256(a, mask_lo),
                                  _mm256_and_si256(b, mask_lo));
    __m256i l_res = _mm256_and_si256(t, mask_lo);
    __m256i carry = _mm256_and_si256(t, mask_hi);
    __m256i h_res = _mm256_and_si256(_mm256_xor_si256(_mm256_xor_si256(a, b), carry),
                                      mask_hi);

    return _mm256_or_si256(l_res, h_res);
}

uint32_t decode_bw32(int16_t t[32])
{
    int16_t w[32] __attribute__((aligned(32)));
    __m256i t_lo = _mm256_slli_epi16(_mm256_loadu_si256((const __m256i *)t), 2);
    __m256i t_hi = _mm256_slli_epi16(_mm256_loadu_si256((const __m256i *)(t + 16)), 2);

    _mm256_storeu_si256((__m256i *)t, t_lo);
    _mm256_storeu_si256((__m256i *)(t + 16), t_hi);

    bdd32_avx2_exact(w, t_lo, t_hi);

    return delabel_bw32_avx2(w);
}

uint64_t encode_bw32(uint32_t m)
{
    static const uint64_t bw_c[32] __attribute__((aligned(32))) = {
        0x00000000000000aaULL, 0x0000000000000a0aULL, 0x0000000000008888ULL, 0x0000000000002222ULL,
        0x00000000000a000aULL, 0x0000000000880088ULL, 0x0000000000220022ULL, 0x0000000008080808ULL,
        0x0000000002020202ULL, 0x00000000ddddddddULL, 0x0000000055555555ULL, 0x00000000aaaaaaaaULL,
        0x0000000a0000000aULL, 0x0000008800000088ULL, 0x0000002200000022ULL, 0x0000080800000808ULL,
        0x0000020200000202ULL, 0x0000dddd0000ddddULL, 0x0000555500005555ULL, 0x0000aaaa0000aaaaULL,
        0x0008000800080008ULL, 0x0002000200020002ULL, 0x00dd00dd00dd00ddULL, 0x0055005500550055ULL,
        0x00aa00aa00aa00aaULL, 0x0d0d0d0d0d0d0d0dULL, 0x0505050505050505ULL, 0x0a0a0a0a0a0a0a0aULL,
        0x1111111111111111ULL, 0x2222222222222222ULL, 0x4444444444444444ULL, 0x8888888888888888ULL
    };
    const __m256i *vecs = (const __m256i *)bw_c;
    __m256i m_vec = _mm256_set1_epi64x(m);
    __m256i vals[8];

    for (int g = 0; g < 8; g++) {
        __m256i shift = _mm256_setr_epi64x(g * 4, g * 4 + 1, g * 4 + 2, g * 4 + 3);
        __m256i bits = _mm256_and_si256(_mm256_srlv_epi64(m_vec, shift), _mm256_set1_epi64x(1));
        __m256i mask = _mm256_sub_epi64(_mm256_setzero_si256(), bits);
        vals[g] = _mm256_and_si256(mask, _mm256_load_si256(&vecs[g]));
    }

    __m256i sum0 = bw32_add_mod4_256(vals[0], vals[1]);
    __m256i sum1 = bw32_add_mod4_256(vals[2], vals[3]);
    __m256i sum2 = bw32_add_mod4_256(vals[4], vals[5]);
    __m256i sum3 = bw32_add_mod4_256(vals[6], vals[7]);
    __m256i sum01 = bw32_add_mod4_256(sum0, sum1);
    __m256i sum23 = bw32_add_mod4_256(sum2, sum3);
    __m256i sum_all = bw32_add_mod4_256(sum01, sum23);
    __m256i swapped = _mm256_permute4x64_epi64(sum_all, _MM_SHUFFLE(1, 0, 3, 2));
    __m256i sum_half = bw32_add_mod4_256(sum_all, swapped);
    __m256i swapped2 = _mm256_shuffle_epi32(sum_half, _MM_SHUFFLE(1, 0, 3, 2));
    __m256i final_sum = bw32_add_mod4_256(sum_half, swapped2);

    return (uint64_t)_mm256_extract_epi64(final_sum, 0);
}
