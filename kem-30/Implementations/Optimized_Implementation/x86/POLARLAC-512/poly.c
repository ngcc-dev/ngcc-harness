/*
Copyright (c) 2026 Ying Liu, Yu Zhang, Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial compression, encoding, and arithmetic helpers for the optimized POLARLAC-512 instance.
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

int64_t llr_table[] = {35683454150766, 107050362452299, 178417270753832, 249784179055365, 321151087356897, 392517995658430, 463884903959963, 535251812261495, 606618720563028, 677985628864561, 749352537166094, 820719445467627, 892086353769159, 963453262070692, 1034820170372225, 1106187078673758, 1177553986975290, 1248920895276823, 1320287803578356, 1391654711879888, 1463021620181421, 1534388528482953, 1605755436784485, 1677122345086015, 1748489253387543, 1819856161689065, 1891223069990575, 1962589978292060, 2033956886593491, 2105323794894806, 2176690703195874, 2248057611496418, 2319424519795843, 2390791428092882, 2462158336384837, 2533525244665956, 2604892152923978, 2676259061132772, 2747625969236641, 2818992877116870, 2890359784520433, 2961726690908022, 3033093595130154, 3104460494736810, 3175827384505990, 3247194253307471, 3318561077418193, 3389927806274649, 3461294332105569, 3532660425207667, 3604025595995944, 3675388800995756, 3746747816279836, 3818097902408123, 3889428960695853, 3960719480358583, 4031923675061336, 4102944236139146, 4173575027287499, 4243382379717203, 4311467166456192, 4376021404661801, 4433631808043037, 4478619971806684, 4503599627370476, 4502756711130694, 4476091223087338, 4429417226844128, 4370120990983328, 4303880920298155, 4234110301079602, 4162617116170334, 4090300492542418, 4017594098985043, 3944704071802727, 3871727719660434, 3798710828893139, 3725674910285289, 3652630062521645, 3579581025042269, 3506530021774429, 3433478096192767, 3360425737882283, 3287373176546264, 3214320519955977, 3141267818674933, 3068215096426189, 2995162364339969, 2922109627638273, 2849056888771120, 2776004148887994, 2702951408528201, 2629898667944768, 2556845927256411, 2483793186518825, 2410740445758142, 2337687704986623, 2264634964210020, 2191582223431032, 2118529482650925, 2045476741870292, 1972424001089413, 1899371260308419, 1826318519527370, 1753265778746296, 1680213037965210, 1607160297184119, 1534107556403025, 1461054815621929, 1388002074840833, 1314949334059737, 1241896593278640, 1168843852497544, 1095791111716448, 1022738370935351, 949685630154255, 876632889373158, 803580148592062, 730527407810965, 657474667029868, 584421926248772, 511369185467676, 438316444686579, 365263703905483, 292210963124386, 219158222343290, 146105481562193, 73052740781096, 0, -73052740781096, -146105481562193, -219158222343290, -292210963124386, -365263703905483, -438316444686579, -511369185467676, -584421926248772, -657474667029868, -730527407810965, -803580148592062, -876632889373158, -949685630154255, -1022738370935351, -1095791111716448, -1168843852497544, -1241896593278640, -1314949334059737, -1388002074840833, -1461054815621929, -1534107556403025, -1607160297184119, -1680213037965210, -1753265778746296, -1826318519527370, -1899371260308419, -1972424001089413, -2045476741870292, -2118529482650925, -2191582223431032, -2264634964210020, -2337687704986623, -2410740445758142, -2483793186518825, -2556845927256411, -2629898667944768, -2702951408528201, -2776004148887994, -2849056888771120, -2922109627638273, -2995162364339969, -3068215096426189, -3141267818674932, -3214320519955977, -3287373176546264, -3360425737882283, -3433478096192767, -3506530021774429, -3579581025042269, -3652630062521645, -3725674910285289, -3798710828893139, -3871727719660434, -3944704071802727, -4017594098985043, -4090300492542418, -4162617116170334, -4234110301079602, -4303880920298155, -4370120990983328, -4429417226844127, -4476091223087338, -4502756711130694, -4503599627370476, -4478619971806684, -4433631808043037, -4376021404661801, -4311467166456192, -4243382379717203, -4173575027287499, -4102944236139146, -4031923675061336, -3960719480358583, -3889428960695853, -3818097902408122, -3746747816279836, -3675388800995756, -3604025595995944, -3532660425207667, -3461294332105569, -3389927806274649, -3318561077418193, -3247194253307471, -3175827384505990, -3104460494736810, -3033093595130154, -2961726690908022, -2890359784520433, -2818992877116870, -2747625969236641, -2676259061132772, -2604892152923978, -2533525244665956, -2462158336384837, -2390791428092882, -2319424519795843, -2248057611496418, -2176690703195874, -2105323794894806, -2033956886593491, -1962589978292060, -1891223069990575, -1819856161689065, -1748489253387543, -1677122345086015, -1605755436784485, -1534388528482953, -1463021620181421, -1391654711879888, -1320287803578356, -1248920895276823, -1177553986975290, -1106187078673758, -1034820170372225, -963453262070692, -892086353769159, -820719445467627, -749352537166094, -677985628864561, -606618720563028, -535251812261495, -463884903959963, -392517995658430, -321151087356897, -249784179055365, -178417270753832, -107050362452299, -35683454150766};


/* exact for x in [0, 257] */
static inline uint16_t mod86_u257(uint16_t x)
{
    uint32_t q = ((uint32_t)x * 191U) >> 14;
    return (uint16_t)(x - q * 86U);
}

/* exact for x in [0, 257] */
static inline uint16_t mod3_u257(uint16_t x)
{
    uint32_t q = ((uint32_t)x * 171U) >> 9;
    return (uint16_t)(x - q * 3U);
}

/* exact for x < 86^7 */
static inline uint16_t divmod86_u64(uint64_t *x)
{
    const uint64_t DIV86_M = 13091859381891ULL;
    uint64_t q = (uint64_t)(((__uint128_t)(*x) * DIV86_M) >> 50);
    uint16_t r = (uint16_t)(*x - q * 86ULL);
    *x = q;
    return r;
}

/* exact for x < 3^17 */
static inline uint16_t divmod3_u32(uint32_t *x)
{
    const uint32_t DIV3_M = 44739243U;
    uint32_t q = (uint32_t)(((uint64_t)(*x) * DIV3_M) >> 27);
    uint16_t r = (uint16_t)(*x - q * 3U);
    *x = q;
    return r;
}

/* exact for x <= 7739 */
static inline uint16_t mod258_u7739(uint16_t x)
{
    const uint32_t DIV258_M = 8129U;
    uint32_t q = ((uint32_t)x * DIV258_M) >> 21;
    return (uint16_t)(x - q * 258U);
}

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

/* -------------------------------------------------------------------------
 * AVX2 helpers for length-N post-processing and c2 nibble packing.
 * For q=257, 2^16 = 1 mod q, so Montgomery reduction directly returns the
 * standard residue representative.
 * ------------------------------------------------------------------------- */
static inline __m256i avx2_mod_q_epi32(__m256i a)
{
    const __m256i q = _mm256_set1_epi32((int)RL_KEM_Q);
    const __m256i qinv = _mm256_set1_epi32((int)QINV);
    __m256i u = _mm256_mullo_epi32(a, qinv);
    u = _mm256_srai_epi32(_mm256_slli_epi32(u, 16), 16);
    __m256i t = _mm256_sub_epi32(a, _mm256_mullo_epi32(u, q));
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

int poly_compress(uint8_t *code_c, const int16_t *c)
{
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

    // return 0;
    
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

int polyvec_compress(uint8_t *code_c, const polarlac_polyvec *c)
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
