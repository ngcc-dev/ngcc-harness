/*
Copyright (c) 2026 Ying Liu, Yu Zhang, Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial compression, encoding, and arithmetic helpers for the optimized POLARLAC-256 instance.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "polar.h"


int64_t llr_table[] = {141089181650599, 423267544951794, 705445908252992, 987624271554188, 1269802634855385, 1551980998156581, 1834159361457778, 2116337724758975, 2398516088060172, 2680694451361368, 2962872814662565, 3245051177963761, 3527229541264958, 3809407904566154, 4091586267867351, 4373764631168548, 4655942994469744, 4938121357770941, 5220299721072138, 5502478084373333, 5784656447674530, 6066834810975727, 6349013174276923, 6631191537578121, 6913369900879316, 7195548264180514, 7477726627481709, 7759904990782907, 8042083354084104, 8324261717385299, 8606440080686497, 8888618443987694, 9170796807288890, 9452975170590086, 9735153533891284, 10017331897192480, 10299510260493676, 10581688623794872, 10863866987096068, 11146045350397264, 11428223713698454, 11710402076999632, 11992580440300752, 12274758803601676, 12556937166901882, 12839115530199462, 13121293893487482, 13403472256740692, 13685650619867092, 13967828982531580, 14250007343513516, 14532185698366624, 14814364030895008, 15096542282103860, 15378720237100262, 15660897113122826, 15943070058938790, 16225228689192344, 16507335180792430, 16789251844690412, 17070478256459890, 17349206306448962, 17619039711573300, 17858960429665366, 18014398509481964, 18011065694167382, 17848961983721626, 17602375635000394, 17325876599246896, 17040482918628660, 16752590876230024, 16464008581702876, 16175236459473630, 15886412198590908, 15597573622145784, 15308731115494062, 15019887529868496, 14731043648030482, 14442199684872936, 14153355699390664, 13864511707779568, 13575667714485918, 13286823720730356, 12997979726847984, 12709135732930800, 12420291739004058, 12131447745074692, 11842603751144606, 11553759757214322, 11264915763283982, 10976071769353628, 10687227775423272, 10398383781492916, 10109539787562556, 9820695793632196, 9531851799701840, 9243007805771480, 8954163811841120, 8665319817910763, 8376475823980403, 8087631830050044, 7798787836119686, 7509943842189327, 7221099848258969, 6932255854328609, 6643411860398251, 6354567866467892, 6065723872537533, 5776879878607175, 5488035884676815, 5199191890746457, 4910347896816099, 4621503902885740, 4332659908955381, 4043815915025023, 3754971921094664, 3466127927164305, 3177283933233946, 2888439939303588, 2599595945373229, 2310751951442871, 2021907957512511, 1733063963582153, 1444219969651793, 1155375975721435, 866531981791076, 577687987860718, 288843993930359, 0, -288843993930358, -577687987860718, -866531981791076, -1155375975721435, -1444219969651793, -1733063963582153, -2021907957512511, -2310751951442871, -2599595945373229, -2888439939303588, -3177283933233946, -3466127927164305, -3754971921094664, -4043815915025023, -4332659908955382, -4621503902885740, -4910347896816099, -5199191890746457, -5488035884676815, -5776879878607175, -6065723872537533, -6354567866467892, -6643411860398251, -6932255854328609, -7221099848258969, -7509943842189327, -7798787836119686, -8087631830050044, -8376475823980403, -8665319817910763, -8954163811841120, -9243007805771480, -9531851799701840, -9820695793632196, -10109539787562556, -10398383781492916, -10687227775423272, -10976071769353628, -11264915763283982, -11553759757214322, -11842603751144606, -12131447745074692, -12420291739004058, -12709135732930800, -12997979726847984, -13286823720730356, -13575667714485918, -13864511707779568, -14153355699390664, -14442199684872936, -14731043648030482, -15019887529868496, -15308731115494062, -15597573622145784, -15886412198590908, -16175236459473630, -16464008581702876, -16752590876230024, -17040482918628660, -17325876599246896, -17602375635000394, -17848961983721626, -18011065694167382, -18014398509481964, -17858960429665366, -17619039711573300, -17349206306448962, -17070478256459890, -16789251844690412, -16507335180792430, -16225228689192344, -15943070058938790, -15660897113122826, -15378720237100262, -15096542282103860, -14814364030895008, -14532185698366624, -14250007343513516, -13967828982531580, -13685650619867092, -13403472256740692, -13121293893487482, -12839115530199462, -12556937166901882, -12274758803601676, -11992580440300752, -11710402076999632, -11428223713698454, -11146045350397264, -10863866987096068, -10581688623794872, -10299510260493676, -10017331897192480, -9735153533891284, -9452975170590086, -9170796807288890, -8888618443987692, -8606440080686497, -8324261717385299, -8042083354084104, -7759904990782907, -7477726627481709, -7195548264180514, -6913369900879316, -6631191537578120, -6349013174276923, -6066834810975727, -5784656447674530, -5502478084373333, -5220299721072138, -4938121357770941, -4655942994469744, -4373764631168548, -4091586267867351, -3809407904566154, -3527229541264958, -3245051177963761, -2962872814662565, -2680694451361368, -2398516088060172, -2116337724758975, -1834159361457778, -1551980998156581, -1269802634855385, -987624271554188, -705445908252992, -423267544951794, -141089181650599};


void byte_compress(uint8_t *dst, const int16_t *src)
{
    const __m256i lowmask = _mm256_set1_epi16(0x00FF);
    for (int i = 0; i < RL_KEM_N; i += 32) {
        __m256i a0 = _mm256_loadu_si256((const __m256i *)(const void *)(src + i));
        __m256i a1 = _mm256_loadu_si256((const __m256i *)(const void *)(src + i + 16));
        a0 = _mm256_and_si256(a0, lowmask);
        a1 = _mm256_and_si256(a1, lowmask);
        __m256i packed = _mm256_packus_epi16(a0, a1);
        packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
        _mm256_storeu_si256((__m256i *)(void *)(dst + i), packed);
    }
}
void byte_decompress(int16_t *dst, const uint8_t *src)
{
    for (int i = 0; i < RL_KEM_N; i += 32) {
        __m256i bytes = _mm256_loadu_si256((const __m256i *)(const void *)(src + i));
        __m128i lo = _mm256_castsi256_si128(bytes);
        __m128i hi = _mm256_extracti128_si256(bytes, 1);
        __m256i v0 = _mm256_cvtepu8_epi16(lo);
        __m256i v1 = _mm256_cvtepu8_epi16(hi);
        _mm256_storeu_si256((__m256i *)(void *)(dst + i), v0);
        _mm256_storeu_si256((__m256i *)(void *)(dst + i + 16), v1);
    }
}

void polyvec_byte_compress(uint8_t *dst, const polarlac_polyvec *src)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        byte_compress(dst + k * RL_KEM_N, src->vec[k].coeffs);
    }
}

void polyvec_byte_decompress(polarlac_polyvec *dst, const uint8_t *src)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        byte_decompress(dst->vec[k].coeffs, src + k * RL_KEM_N);
    }
}

static inline uint8_t mod86_u257(uint16_t x)
{
    uint32_t q = ((uint32_t)x * 191U) >> 14;
    return (uint8_t)(x - q * 86U);
}

static inline uint8_t mod3_u257(uint16_t x)
{
    uint32_t q = ((uint32_t)x * 171U) >> 9;
    return (uint8_t)(x - q * 3U);
}

static inline uint8_t divmod86_u64(uint64_t *x)
{
    const uint64_t M = 13091859381890ULL; /* floor(2^50 / 86) */
    uint64_t q = (uint64_t)(((__uint128_t)(*x) * M) >> 50);
    uint64_t r = *x - q * 86ULL;
    uint64_t c = (uint64_t)(r >= 86ULL);

    q += c;
    r -= c * 86ULL;

    *x = q;
    return (uint8_t)r;
}

static inline uint8_t divmod3_u32(uint32_t *x)
{
    const uint32_t M = 44739242U; /* floor(2^27 / 3) */
    uint32_t q = (uint32_t)(((uint64_t)(*x) * M) >> 27);
    uint32_t r = *x - q * 3U;
    uint32_t c = (uint32_t)(r >= 3U);

    q += c;
    r -= c * 3U;

    *x = q;
    return (uint8_t)r;
}

static inline uint16_t mod258_u7739(uint16_t x)
{
    uint32_t q = ((uint32_t)x * 8129U) >> 21;
    return (uint16_t)(x - q * 258U);
}


/* -------------------------------------------------------------------------
 * AVX2 helpers for length-N post-processing and c2 nibble packing.
 * q=257 uses Montgomery factor 1; q=769 uses factor 171 = 2^16 mod 769.
 * ------------------------------------------------------------------------- */
static inline __m256i avx2_mod_q_epi32(__m256i a)
{
#if RL_KEM_Q == 769
    const __m256i mont_factor = _mm256_set1_epi32(171);
#else
    const __m256i mont_factor = _mm256_set1_epi32(1);
#endif
    const __m256i q = _mm256_set1_epi32((int)RL_KEM_Q);
    const __m256i qinv = _mm256_set1_epi32((int)QINV);
    __m256i m = _mm256_mullo_epi32(a, mont_factor);
    __m256i u = _mm256_mullo_epi32(m, qinv);
    u = _mm256_srai_epi32(_mm256_slli_epi32(u, 16), 16);
    __m256i t = _mm256_sub_epi32(m, _mm256_mullo_epi32(u, q));
    __m256i r = _mm256_srai_epi32(t, 16);
    r = _mm256_add_epi32(r, _mm256_and_si256(_mm256_srai_epi32(r, 31), q));
    __m256i r_sub = _mm256_sub_epi32(r, q);
    __m256i geq = _mm256_cmpgt_epi32(r_sub, _mm256_set1_epi32(-1));
    return _mm256_blendv_epi8(r, r_sub, geq);
}

static inline __m128i pack_i32_to_i16x8(__m256i x)
{
    return _mm_packs_epi32(_mm256_castsi256_si128(x), _mm256_extracti128_si256(x, 1));
}

static inline __m256i pack_i32_pair_to_i16x16(__m256i x0, __m256i x1)
{
    __m128i p0 = pack_i32_to_i16x8(x0);
    __m128i p1 = pack_i32_to_i16x8(x1);
    return _mm256_inserti128_si256(_mm256_castsi128_si256(p0), p1, 1);
}

void poly_add_reduce_q_avx2(int16_t *dst, const int16_t *a, const int16_t *b)
{
    for (int i = 0; i < RL_KEM_N; i += 16) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(const void *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(const void *)(b + i));
        __m256i a0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(va));
        __m256i a1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(va, 1));
        __m256i b0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(vb));
        __m256i b1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(vb, 1));
        __m256i r0 = avx2_mod_q_epi32(_mm256_add_epi32(a0, b0));
        __m256i r1 = avx2_mod_q_epi32(_mm256_add_epi32(a1, b1));
        _mm256_storeu_si256((__m256i *)(void *)(dst + i), pack_i32_pair_to_i16x16(r0, r1));
    }
}

void poly_add_inplace_reduce_q_avx2(int16_t *dst, const int16_t *src)
{
    poly_add_reduce_q_avx2(dst, dst, src);
}

void poly_sub_reduce_q_avx2(int16_t *dst, const int16_t *a, const int16_t *b)
{
    for (int i = 0; i < RL_KEM_N; i += 16) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(const void *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(const void *)(b + i));
        __m256i a0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(va));
        __m256i a1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(va, 1));
        __m256i b0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(vb));
        __m256i b1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(vb, 1));
        __m256i r0 = avx2_mod_q_epi32(_mm256_sub_epi32(a0, b0));
        __m256i r1 = avx2_mod_q_epi32(_mm256_sub_epi32(a1, b1));
        _mm256_storeu_si256((__m256i *)(void *)(dst + i), pack_i32_pair_to_i16x16(r0, r1));
    }
}

void poly_normalize_q_avx2(int16_t *dst)
{
    for (int i = 0; i < RL_KEM_N; i += 16) {
        __m256i v = _mm256_loadu_si256((const __m256i *)(const void *)(dst + i));
        __m256i x0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(v));
        __m256i x1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(v, 1));
        __m256i r0 = avx2_mod_q_epi32(x0);
        __m256i r1 = avx2_mod_q_epi32(x1);
        _mm256_storeu_si256((__m256i *)(void *)(dst + i), pack_i32_pair_to_i16x16(r0, r1));
    }
}

void poly_add_msg_inplace_reduce_q_avx2(int16_t *dst, const uint8_t *msg_bits)
{
    const __m256i ratio32 = _mm256_set1_epi32((int)RATIO);
    for (int i = 0; i < RL_KEM_Lv; i += 16) {
        __m256i v = _mm256_loadu_si256((const __m256i *)(const void *)(dst + i));
        __m128i mb = _mm_loadu_si128((const __m128i *)(const void *)(msg_bits + i));
        __m256i m16 = _mm256_cvtepu8_epi16(mb);
        __m256i v0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(v));
        __m256i v1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(v, 1));
        __m256i m0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(m16));
        __m256i m1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(m16, 1));
        __m256i r0 = avx2_mod_q_epi32(_mm256_add_epi32(v0, _mm256_mullo_epi32(m0, ratio32)));
        __m256i r1 = avx2_mod_q_epi32(_mm256_add_epi32(v1, _mm256_mullo_epi32(m1, ratio32)));
        _mm256_storeu_si256((__m256i *)(void *)(dst + i), pack_i32_pair_to_i16x16(r0, r1));
    }
}

static inline __m128i pack_u32_to_u8x16(__m256i t0, __m256i t1)
{
    __m128i t0_lo = _mm256_castsi256_si128(t0);
    __m128i t0_hi = _mm256_extracti128_si256(t0, 1);
    __m128i t1_lo = _mm256_castsi256_si128(t1);
    __m128i t1_hi = _mm256_extracti128_si256(t1, 1);
    __m128i p0 = _mm_packs_epi32(t0_lo, t0_hi);
    __m128i p1 = _mm_packs_epi32(t1_lo, t1_hi);
    return _mm_packus_epi16(p0, p1);
}

int poly_compress(uint8_t *code_c, int16_t *c)
{
    /*
     * Fixed zero-selector lossless PK encoding for coefficients in Z_257.
     * The low-byte payload is vectorized with AVX2; the selector is kept as
     * a scalar scan because it depends on the order of low-zero candidates.
     */
    // uint8_t *selector = code_c + RL_KEM_N;
    // uint16_t candidates = 0;
    // uint16_t byte_idx = 0;
    // uint8_t bit_pos = 0;

    // byte_compress(code_c, c);
    // memset(selector, 0, PK_ZERO_SELECTOR_BYTES);

    // for (int i = 0; i < RL_KEM_N; i++) {
    //     if (code_c[i] == 0) {
    //         if (candidates >= PK_ZERO_SELECTOR_BITS) {
    //             return -1;
    //         }
    //         byte_idx = candidates >> 3;
    //         bit_pos = candidates & 7U;
    //         if (c[i] == 256) {
    //             selector[byte_idx] |= (uint8_t)(1U << bit_pos);
    //         }
    //         candidates++;
    //     }
    // }


 uint8_t *selector = code_c + RL_KEM_N;
    uint16_t candidates = 0;

    byte_compress(code_c, c);
    memset(selector, 0, PK_ZERO_SELECTOR_BYTES);
    for (int i = 0; i < RL_KEM_N; i += 32) {
    __m256i v = _mm256_loadu_si256((const __m256i *)(const void *)(code_c + i));
    __m256i z = _mm256_cmpeq_epi8(v, _mm256_setzero_si256());
    uint32_t mask = (uint32_t)_mm256_movemask_epi8(z);

    while (mask != 0) {
        unsigned bit = (unsigned)__builtin_ctz(mask);
        int idx = i + (int)bit;

        if (candidates >= PK_ZERO_SELECTOR_BITS) {
            return -1;
        }

        if (c[idx] == 256) {
            selector[candidates >> 3] |= (uint8_t)(1U << (candidates & 7U));
        }

        candidates++;
        mask &= mask - 1;
    }
}
    return 0;
    return 0;
}

void poly_decompress(int16_t *c, const uint8_t *code_c)
{
    const uint8_t *selector = code_c + RL_KEM_N;
    uint16_t candidates = 0;
    uint32_t selector_bits = 0;
    const __m256i zero = _mm256_setzero_si256();

    for (unsigned int j = 0; j < PK_ZERO_SELECTOR_BYTES; j++) {
        selector_bits |= (uint32_t)selector[j] << (8U * j);
    }

    byte_decompress(c, code_c);
    for (int i = 0; i < RL_KEM_N && candidates < PK_ZERO_SELECTOR_BITS; i += 32) {
        __m256i v = _mm256_loadu_si256((const __m256i *)(const void *)(code_c + i));
        __m256i z = _mm256_cmpeq_epi8(v, zero);
        uint32_t mask = (uint32_t)_mm256_movemask_epi8(z);

        while (mask != 0 && candidates < PK_ZERO_SELECTOR_BITS) {
            unsigned bit = (unsigned)__builtin_ctz(mask);
            uint16_t sel = (uint16_t)((selector_bits >> candidates) & 1U);
            c[i + (int)bit] = (int16_t)(sel << 8);
            candidates++;
            mask &= mask - 1;
        }
    }
}

int polyvec_compress(uint8_t *code_c, polarlac_polyvec *c)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        if (poly_compress(code_c + k * PK_POLY_BYTES, c->vec[k].coeffs) != 0) {
            return -1;
        }
    }
    return 0;
}

void polyvec_decompress(polarlac_polyvec *c, const uint8_t *code_c)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        poly_decompress(c->vec[k].coeffs, code_c + k * PK_POLY_BYTES);
    }
}


void poly_compress_c2(uint8_t *code_c, const int16_t *c, unsigned int d)
{
    if (d != 4u) {
        uint32_t mask = (1u << d) - 1u;
        for (uint32_t i = 0; i < RL_KEM_Lv; i++) {
            uint32_t x = (uint32_t)rl_kem_mod_q(c[i]);
            uint32_t y = (x << d) + (RL_KEM_Q >> 1);
            uint32_t t = (uint32_t)(((uint64_t)y * (uint64_t)RECIP_Q) >> RECIP_K);
            code_c[i] = (uint8_t)(t & mask);
        }
        return;
    }

    const __m256i half = _mm256_set1_epi32(RL_KEM_Q >> 1);
    const __m256i recip = _mm256_set1_epi32((int)RECIP_Q);
    const __m256i mask = _mm256_set1_epi32(0x0F);
    for (int i = 0; i < RL_KEM_Lv; i += 16) {
        __m256i x16 = _mm256_loadu_si256((const __m256i *)(const void *)(c + i));
        __m256i x0 = avx2_mod_q_epi32(_mm256_cvtepi16_epi32(_mm256_castsi256_si128(x16)));
        __m256i x1 = avx2_mod_q_epi32(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(x16, 1)));
        __m256i y0 = _mm256_add_epi32(_mm256_slli_epi32(x0, 4), half);
        __m256i y1 = _mm256_add_epi32(_mm256_slli_epi32(x1, 4), half);
        __m256i t0 = _mm256_and_si256(_mm256_srli_epi32(_mm256_mullo_epi32(y0, recip), RECIP_K), mask);
        __m256i t1 = _mm256_and_si256(_mm256_srli_epi32(_mm256_mullo_epi32(y1, recip), RECIP_K), mask);
        __m128i bytes = pack_u32_to_u8x16(t0, t1);
        _mm_storeu_si128((__m128i *)(void *)(code_c + i), bytes);
    }
}


void poly_decompress_c2(int16_t *c, const uint8_t *code_c, unsigned int d)
{
    if (d != 4u) {
        uint32_t i;
        uint32_t half = 1u << (d - 1);
        for (i = 0; i < RL_KEM_Lv; i++) {
            c[i] = (uint16_t)((((uint32_t)code_c[i] * (uint32_t)RL_KEM_Q) + half) >> d);
        }
        for (; i < RL_KEM_N; i++) c[i] = 0;
        return;
    }

    const __m256i q = _mm256_set1_epi16((int16_t)RL_KEM_Q);
    const __m256i half = _mm256_set1_epi16(1 << 3);
    for (int i = 0; i < RL_KEM_Lv; i += 32) {
        __m256i bytes = _mm256_loadu_si256((const __m256i *)(const void *)(code_c + i));
        __m128i lo = _mm256_castsi256_si128(bytes);
        __m128i hi = _mm256_extracti128_si256(bytes, 1);
        __m256i x0 = _mm256_cvtepu8_epi16(lo);
        __m256i x1 = _mm256_cvtepu8_epi16(hi);
        x0 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_mullo_epi16(x0, q), half), 4);
        x1 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_mullo_epi16(x1, q), half), 4);
        _mm256_storeu_si256((__m256i *)(void *)(c + i), x0);
        _mm256_storeu_si256((__m256i *)(void *)(c + i + 16), x1);
    }
}

//压缩到4bit后的紧凑打包和对应解包函数
void pack_c2_dbit(uint8_t *dst, const uint8_t *com_c2)
{
#if D_C2_BITS == 4
    const __m256i mask0f = _mm256_set1_epi16(0x000F);
    for (int i = 0; i < RL_KEM_Lv; i += 32) {
        __m256i x = _mm256_loadu_si256((const __m256i *)(const void *)(com_c2 + i));
        __m256i lanes = _mm256_or_si256(
            _mm256_and_si256(x, mask0f),
            _mm256_and_si256(_mm256_srli_epi16(x, 4), _mm256_set1_epi16(0x00F0)));
        __m128i lo = _mm256_castsi256_si128(lanes);
        __m128i hi = _mm256_extracti128_si256(lanes, 1);
        __m128i packed = _mm_packus_epi16(lo, hi);
        _mm_storeu_si128((__m128i *)(void *)(dst + (i >> 1)), packed);
    }
#else
#error "Unsupported D_C2_BITS in AVX2 pack"
#endif
}

void unpack_c2_dbit(uint8_t *com_c2, const uint8_t *src)
{
#if D_C2_BITS == 4
    const __m128i mask0f = _mm_set1_epi8(0x0F);
    for (int i = 0; i < C2_LEN_BYTES; i += 16) {
        __m128i packed = _mm_loadu_si128((const __m128i *)(const void *)(src + i));
        __m128i lo = _mm_and_si128(packed, mask0f);
        __m128i hi = _mm_and_si128(_mm_srli_epi16(packed, 4), mask0f);
        __m128i out0 = _mm_unpacklo_epi8(lo, hi);
        __m128i out1 = _mm_unpackhi_epi8(lo, hi);
        _mm_storeu_si128((__m128i *)(void *)(com_c2 + (i << 1)), out0);
        _mm_storeu_si128((__m128i *)(void *)(com_c2 + (i << 1) + 16), out1);
    }
#else
#error "Unsupported D_C2_BITS in AVX2 unpack"
#endif
}

void poly_mul(int16_t *out, const int16_t *a, const int16_t *s)
{
    int16_t a_ntt[RL_KEM_N];
    int16_t s_ntt[RL_KEM_N];

    memcpy(a_ntt, a, RL_KEM_N * sizeof(int16_t));
    memcpy(s_ntt, s, RL_KEM_N * sizeof(int16_t));

    mq_poly_ntt(a_ntt);
    mq_poly_ntt(s_ntt);
    mq_poly_pointwise_mul(out, a_ntt, s_ntt);
    mq_poly_intt(out);
}

void poly_mul_with_cached_ntt(int16_t *out, const int16_t *a, const int16_t *s_ntt)
{
    int16_t a_ntt[RL_KEM_N];

    memcpy(a_ntt, a, RL_KEM_N * sizeof(int16_t));

    mq_poly_ntt(a_ntt);
    mq_poly_pointwise_mul(out, a_ntt, (int16_t *)s_ntt);
    mq_poly_intt(out);
}

void poly_mul_add_with_ntt(int16_t *out, const int16_t *a, const int16_t *s, const int16_t *e)
{
    int16_t s_ntt[RL_KEM_N];
    int16_t prod[RL_KEM_N];

    memcpy(s_ntt, s, RL_KEM_N * sizeof(int16_t));

    mq_poly_ntt(s_ntt);
    mq_poly_pointwise_mul(prod, (int16_t *)a, s_ntt);
    mq_poly_intt(prod);


    for (int i = 0; i < RL_KEM_N; i++) {
        out[i] = rl_kem_mod_q((int32_t)prod[i] + e[i]);
    }


}


void poly_mul_add_with_cached_ntt(int16_t *out, const int16_t *a_ntt, const int16_t *b_ntt, const int16_t *e)
{
    int16_t prod[RL_KEM_N];

    mq_poly_pointwise_mul(prod, (int16_t *)a_ntt, (int16_t *)b_ntt);
    mq_poly_intt(prod);

    for (int i = 0; i < RL_KEM_N; i++) {
        out[i] = rl_kem_mod_q((int32_t)prod[i] + e[i]);
    }
}


void poly_mul_add(int16_t *out, const int16_t *a, const int16_t *s, const int16_t *e)
{
    int16_t a_ntt[RL_KEM_N];
    int16_t s_ntt[RL_KEM_N];
    int16_t prod[RL_KEM_N];

    memcpy(a_ntt, a, RL_KEM_N * sizeof(int16_t));
    memcpy(s_ntt, s, RL_KEM_N * sizeof(int16_t));

    mq_poly_ntt(a_ntt);
    mq_poly_ntt(s_ntt);
    mq_poly_pointwise_mul(prod, a_ntt, s_ntt);
    mq_poly_intt(prod);


    for (int i = 0; i < RL_KEM_N; i++) {
        out[i] = rl_kem_mod_q((int32_t)prod[i] + e[i]);
    }


}


/* FOR POLAR */
// #define MAX_RECORDS 100000 // number of tests
// uint64_t polar_enc_cycle[MAX_RECORDS];
// int polar_enc_index = 0;
// uint64_t polar_dec_cycle[MAX_RECORDS];
// int polar_dec_index = 0;


void Encode_m(uint8_t *code_m, uint8_t *m)
{
	int i, j;
    int info_cnt = 0;

    /* polar encoding */
    __attribute__((aligned(32))) uint8_t u_8[CODE_LEN]; /* source sequence: each element stores 8 bit */
    memset(u_8, 0, CODE_LEN*sizeof(uint8_t));

	/* fill the message m into the source sequence */
    for (i = 0; i < CODE_LEN; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(info_nodes[8*i+j] == 1)
            {
				u_8[i] |= (((m[info_cnt/8] >> (info_cnt%8)) & 0x01) << j);
                info_cnt++;
            }
        }
    }

    // uint64_t polar_enc_start = rl_kem_read_cycles();
    encode_polar_avx(u_8);
    // uint64_t polar_enc_end = rl_kem_read_cycles();
    // if (polar_enc_index < MAX_RECORDS) {
    //     polar_enc_cycle[polar_enc_index] = polar_enc_end - polar_enc_start;
    //     polar_enc_index++;
    // }

    /* extract first RL_KEM_Lv bits */
    for(i = 0; i < CODE_LEN; i++)
    {
        for(j = 0; j < 8; j++)
        {
            code_m[i*8+j] = (u_8[i] >> j) & 0x01;
        }
    }
}



void Decode_m(uint8_t *m, int16_t *hatm)
{
    int half_2 = 65; // q/2 /2
    int64_t llr[CODE_LEN*8]; // log-likelihood ratio of the received signal
   
	// compute llr
	for(int i = 0; i < RL_KEM_Lv; i++)
	{
		int16_t centered = (int16_t)(hatm[i] - half_2);
		if (centered > (RATIO - 1)) {
            centered = (int16_t)(centered - RL_KEM_Q);
		} else if (centered < -RATIO) {
            centered = (int16_t)(centered + RL_KEM_Q);
        }
		llr[i] = llr_table[centered + RATIO - 1]; // 0 is modulated to -q/4, and 1 is modulated to q/4
	}

	// uint64_t polar_dec_start = rl_kem_read_cycles();
	//polar decode to recover m
	decode_polar_avx(m, llr);
	// uint64_t polar_dec_end = rl_kem_read_cycles();
	// if (polar_dec_index < MAX_RECORDS) {
    //     polar_dec_cycle[polar_dec_index] = polar_dec_end - polar_dec_start;
	// 	polar_dec_index++;
    // }
}
/* END FOR POLAR */
