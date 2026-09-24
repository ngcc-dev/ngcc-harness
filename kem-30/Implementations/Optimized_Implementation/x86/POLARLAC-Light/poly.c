/*
Copyright (c) 2026 Ying Liu, Yu Zhang, Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial compression, encoding, and arithmetic helpers for the optimized POLARLAC-Light instance.
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


int64_t llr_table[] = {282331693575682, 846995080727043, 1411658467878406, 1976321855029769, 2540985242181130, 3105648629332493, 3670312016483855, 4234975403635218, 4799638790786580, 5364302177937941, 5928965565089304, 6493628952240667, 7058292339392028, 7622955726543391, 8187619113694754, 8752282500846115, 9316945887997478, 9881609275148840, 10446272662300202, 11010936049451564, 11575599436602926, 12140262823754288, 12704926210905652, 13269589598057014, 13834252985208376, 14398916372359738, 14963579759511100, 15528243146662462, 16092906533813826, 16657569920965188, 17222233308116552, 17786896695267912, 18351560082419276, 18916223469570636, 19480886856722000, 20045550243873364, 20610213631024724, 21174877018176084, 21739540405327444, 22304203792478796, 22868867179630124, 23433530566781376, 23998193953932336, 24562857341082304, 25127520728228796, 25692184115363164, 26256847502455216, 26821510889399616, 27386174275828768, 27950837660459908, 28515501038816740, 29080164395278804, 29644827675337196, 30209490688778332, 30774152771836272, 31338811608253808, 31903459115397728, 32468067090030568, 33032537136641104, 33596526165102212, 34158840207084848, 34715352316432664, 35252127223491972, 35725639166415748, 36028797018963932, 36022127766359780, 35705631408603296, 35218780960471224, 34668667548203612, 34098816933647500, 33523164386456564, 32945836852787156, 32368028300968316, 31790081821127176, 31212095808774960, 30634098467149120, 30056097878882880, 29478096360233444, 28900094574966760, 28322092713296388, 27744090829731260, 27166088939891820, 26588087048254368, 26010085156101668, 25432083263801312, 24854081371458648, 24276079479103852, 23698077586745592, 23120075694386324, 22542073802026776, 21964071909667152, 21386070017307500, 20808068124947836, 20230066232588180, 19652064340228516, 19074062447868852, 18496060555509188, 17918058663149530, 17340056770789866, 16762054878430206, 16184052986070542, 15606051093710880, 15028049201351218, 14450047308991554, 13872045416631894, 13294043524272228, 12716041631912570, 12138039739552906, 11560037847193244, 10982035954833580, 10404034062473918, 9826032170114256, 9248030277754596, 8670028385394933, 8092026493035271, 7514024600675607, 6936022708315946, 6358020815956283, 5780018923596623, 5202017031236960, 4624015138877297, 4046013246517636, 3468011354157974, 2890009461798312, 2312007569438648, 1734005677078986, 1156003784719324, 578001892359662, 0, -578001892359662, -1156003784719324, -1734005677078986, -2312007569438648, -2890009461798312, -3468011354157974, -4046013246517636, -4624015138877297, -5202017031236960, -5780018923596623, -6358020815956283, -6936022708315946, -7514024600675607, -8092026493035271, -8670028385394933, -9248030277754596, -9826032170114256, -10404034062473918, -10982035954833580, -11560037847193244, -12138039739552906, -12716041631912570, -13294043524272228, -13872045416631894, -14450047308991554, -15028049201351218, -15606051093710880, -16184052986070542, -16762054878430204, -17340056770789866, -17918058663149530, -18496060555509188, -19074062447868852, -19652064340228516, -20230066232588180, -20808068124947836, -21386070017307500, -21964071909667152, -22542073802026776, -23120075694386324, -23698077586745592, -24276079479103852, -24854081371458648, -25432083263801312, -26010085156101668, -26588087048254368, -27166088939891820, -27744090829731260, -28322092713296388, -28900094574966760, -29478096360233444, -30056097878882880, -30634098467149120, -31212095808774956, -31790081821127176, -32368028300968316, -32945836852787156, -33523164386456564, -34098816933647500, -34668667548203612, -35218780960471224, -35705631408603296, -36022127766359780, -36028797018963932, -35725639166415748, -35252127223491972, -34715352316432664, -34158840207084848, -33596526165102212, -33032537136641104, -32468067090030568, -31903459115397728, -31338811608253808, -30774152771836272, -30209490688778332, -29644827675337196, -29080164395278804, -28515501038816740, -27950837660459908, -27386174275828768, -26821510889399616, -26256847502455216, -25692184115363164, -25127520728228796, -24562857341082304, -23998193953932336, -23433530566781376, -22868867179630124, -22304203792478796, -21739540405327444, -21174877018176084, -20610213631024724, -20045550243873364, -19480886856722000, -18916223469570636, -18351560082419276, -17786896695267912, -17222233308116552, -16657569920965188, -16092906533813826, -15528243146662462, -14963579759511100, -14398916372359738, -13834252985208376, -13269589598057014, -12704926210905652, -12140262823754288, -11575599436602926, -11010936049451564, -10446272662300202, -9881609275148840, -9316945887997478, -8752282500846116, -8187619113694754, -7622955726543391, -7058292339392028, -6493628952240667, -5928965565089304, -5364302177937941, -4799638790786580, -4234975403635218, -3670312016483855, -3105648629332493, -2540985242181130, -1976321855029769, -1411658467878406, -846995080727043, -282331693575682};


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


/* -------------------------------------------------------------------------
 * AVX2 helpers for q=257 length-N post-processing and byte/nibble packing.
 * The reduction below is intended for the small ranges produced by the PKE
 * post-processing paths: one signed correction followed by one subtract-q
 * correction maps values in roughly [-257, 2q) to [0, q).
 * ------------------------------------------------------------------------- */
static inline __m256i avx2_mod_q257_epi32(__m256i a)
{
    const __m256i q = _mm256_set1_epi32((int)RL_KEM_Q);
    const __m256i qinv = _mm256_set1_epi32((int)QINV);
    __m256i u = _mm256_mullo_epi32(a, qinv);
    u = _mm256_srai_epi32(_mm256_slli_epi32(u, 16), 16);
    __m256i t = _mm256_sub_epi32(a, _mm256_mullo_epi32(u, q));
    __m256i r = _mm256_srai_epi32(t, 16);
    r = _mm256_add_epi32(r, _mm256_and_si256(_mm256_srai_epi32(r, 31), q));
    return r;
}

static inline __m256i avx2_normalize_small_q257_epi16(__m256i x)
{
    const __m256i q = _mm256_set1_epi16((int16_t)RL_KEM_Q);
    x = _mm256_add_epi16(x, _mm256_and_si256(_mm256_srai_epi16(x, 15), q));
    __m256i y = _mm256_sub_epi16(x, q);
    return _mm256_add_epi16(y, _mm256_and_si256(_mm256_srai_epi16(y, 15), q));
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
        __m256i r0 = avx2_mod_q257_epi32(_mm256_add_epi32(a0, b0));
        __m256i r1 = avx2_mod_q257_epi32(_mm256_add_epi32(a1, b1));
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
        __m256i r0 = avx2_mod_q257_epi32(_mm256_sub_epi32(a0, b0));
        __m256i r1 = avx2_mod_q257_epi32(_mm256_sub_epi32(a1, b1));
        _mm256_storeu_si256((__m256i *)(void *)(dst + i), pack_i32_pair_to_i16x16(r0, r1));
    }
}

void poly_normalize_q_avx2(int16_t *dst)
{
    for (int i = 0; i < RL_KEM_N; i += 16) {
        __m256i v = _mm256_loadu_si256((const __m256i *)(const void *)(dst + i));
        __m256i x0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(v));
        __m256i x1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(v, 1));
        __m256i r0 = avx2_mod_q257_epi32(x0);
        __m256i r1 = avx2_mod_q257_epi32(x1);
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
        __m256i r0 = avx2_mod_q257_epi32(_mm256_add_epi32(v0, _mm256_mullo_epi32(m0, ratio32)));
        __m256i r1 = avx2_mod_q257_epi32(_mm256_add_epi32(v1, _mm256_mullo_epi32(m1, ratio32)));
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
int poly_compress(uint8_t *code_c, const int16_t *c)
{
    /*
     * Fixed zero-selector lossless PK encoding for Z_257.
     * AVX2 writes the low-byte payload first; the tiny selector stream is then
     * built in scalar scan order because only PK_ZERO_SELECTOR_BITS candidates
     * are allowed before KeyGen resamples.
     */
    // uint8_t *selector = code_c + RL_KEM_N;
    // uint16_t candidates = 0;

    // byte_compress(code_c, c);
    // memset(selector, 0, PK_ZERO_SELECTOR_BYTES);

    // for (int i = 0; i < RL_KEM_N; i++) {
    //     if (code_c[i] == 0) {
    //         if (candidates >= PK_ZERO_SELECTOR_BITS) {
    //             return -1;
    //         }
    //         if (c[i] == 256) {
    //             selector[candidates >> 3] |= (uint8_t)(1U << (candidates & 7U));
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

static void poly_compress_c2_scalar(uint8_t *code_c, const int16_t *c, unsigned int d)
{
    uint32_t mask = (1u << d) - 1u;
    for (uint32_t i = 0; i < RL_KEM_Lv; i++) {
        uint32_t x = (uint32_t)rl_kem_mod_q(c[i]);
        uint32_t y = (x << d) + (RL_KEM_Q >> 1);
        uint32_t t = (uint32_t)(((uint64_t)y * (uint64_t)RECIP_Q) >> RECIP_K);
        code_c[i] = (uint8_t)(t & mask);
    }
}

void poly_compress_c2(uint8_t *code_c, const int16_t *c, unsigned int d)
{
    if (d != 3u && d != 4u) {
        poly_compress_c2_scalar(code_c, c, d);
        return;
    }

    const __m256i half = _mm256_set1_epi32(RL_KEM_Q >> 1);
    const __m256i recip = _mm256_set1_epi32((int)RECIP_Q);
    const __m256i mask = _mm256_set1_epi32((d == 3u) ? 0x07 : 0x0F);
    const int shift = (d == 3u) ? 3 : 4;

    for (int i = 0; i < RL_KEM_Lv; i += 16) {
        __m256i x16 = avx2_normalize_small_q257_epi16(
            _mm256_loadu_si256((const __m256i *)(const void *)(c + i)));
        __m256i x0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(x16));
        __m256i x1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(x16, 1));
        __m256i y0 = _mm256_add_epi32(_mm256_slli_epi32(x0, shift), half);
        __m256i y1 = _mm256_add_epi32(_mm256_slli_epi32(x1, shift), half);
        __m256i t0 = _mm256_and_si256(_mm256_srli_epi32(_mm256_mullo_epi32(y0, recip), RECIP_K), mask);
        __m256i t1 = _mm256_and_si256(_mm256_srli_epi32(_mm256_mullo_epi32(y1, recip), RECIP_K), mask);
        __m128i bytes = pack_u32_to_u8x16(t0, t1);
        _mm_storeu_si128((__m128i *)(void *)(code_c + i), bytes);
    }
}


static void poly_decompress_c2_scalar(int16_t *c, const uint8_t *code_c, unsigned int d)
{
    uint32_t i;
    uint32_t half = 1u << (d - 1);

    for (i = 0; i < RL_KEM_Lv; i++) {
        c[i] = (uint16_t)((((uint32_t)code_c[i] * (uint32_t)RL_KEM_Q) + half) >> d);
    }
    for (; i < RL_KEM_N; i++) {
        c[i] = 0;
    }
}

void poly_decompress_c2(int16_t *c, const uint8_t *code_c, unsigned int d)
{
    if (d != 3u && d != 4u) {
        poly_decompress_c2_scalar(c, code_c, d);
        return;
    }

    const __m256i q = _mm256_set1_epi16((int16_t)RL_KEM_Q);
    const __m256i half = _mm256_set1_epi16(1 << (d - 1));
    const int shift = (d == 3u) ? 3 : 4;
    for (int i = 0; i < RL_KEM_Lv; i += 32) {
        __m256i bytes = _mm256_loadu_si256((const __m256i *)(const void *)(code_c + i));
        __m128i lo = _mm256_castsi256_si128(bytes);
        __m128i hi = _mm256_extracti128_si256(bytes, 1);
        __m256i x0 = _mm256_cvtepu8_epi16(lo);
        __m256i x1 = _mm256_cvtepu8_epi16(hi);
        x0 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_mullo_epi16(x0, q), half), shift);
        x1 = _mm256_srli_epi16(_mm256_add_epi16(_mm256_mullo_epi16(x1, q), half), shift);
        _mm256_storeu_si256((__m256i *)(void *)(c + i), x0);
        _mm256_storeu_si256((__m256i *)(void *)(c + i + 16), x1);
    }
}


//d-bit压缩结果的紧凑打包和对应解包函数


static inline void pack_c2_3bit_block8(uint8_t *dst, const uint8_t *x)
{
    uint32_t x0 = x[0] & 7u;
    uint32_t x1 = x[1] & 7u;
    uint32_t x2 = x[2] & 7u;
    uint32_t x3 = x[3] & 7u;
    uint32_t x4 = x[4] & 7u;
    uint32_t x5 = x[5] & 7u;
    uint32_t x6 = x[6] & 7u;
    uint32_t x7 = x[7] & 7u;

    dst[0] = (uint8_t)(x0 | (x1 << 3) | (x2 << 6));
    dst[1] = (uint8_t)((x2 >> 2) | (x3 << 1) | (x4 << 4) | (x5 << 7));
    dst[2] = (uint8_t)((x5 >> 1) | (x6 << 2) | (x7 << 5));
}

static inline void unpack_c2_3bit_block8(uint8_t *x, const uint8_t *src)
{
    uint32_t b0 = src[0];
    uint32_t b1 = src[1];
    uint32_t b2 = src[2];

    x[0] = (uint8_t)( b0        & 7u);
    x[1] = (uint8_t)((b0 >> 3)  & 7u);
    x[2] = (uint8_t)(((b0 >> 6) | (b1 << 2)) & 7u);
    x[3] = (uint8_t)((b1 >> 1)  & 7u);
    x[4] = (uint8_t)((b1 >> 4)  & 7u);
    x[5] = (uint8_t)(((b1 >> 7) | (b2 << 1)) & 7u);
    x[6] = (uint8_t)((b2 >> 2)  & 7u);
    x[7] = (uint8_t)((b2 >> 5)  & 7u);
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
#elif D_C2_BITS == 3
    const __m256i mask07 = _mm256_set1_epi8(0x07);
    uint8_t tmp[32] POLARLAC_ALIGN64;
    for (int i = 0; i < RL_KEM_Lv; i += 32) {
        __m256i x = _mm256_and_si256(
            _mm256_loadu_si256((const __m256i *)(const void *)(com_c2 + i)),
            mask07);
        _mm256_store_si256((__m256i *)(void *)tmp, x);
        uint8_t *out = dst + (i * 3) / 8;
        pack_c2_3bit_block8(out + 0, tmp + 0);
        pack_c2_3bit_block8(out + 3, tmp + 8);
        pack_c2_3bit_block8(out + 6, tmp + 16);
        pack_c2_3bit_block8(out + 9, tmp + 24);
    }
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
#elif D_C2_BITS == 3
    uint8_t tmp[32] POLARLAC_ALIGN64;
    for (int i = 0; i < RL_KEM_Lv; i += 32) {
        const uint8_t *in = src + (i * 3) / 8;
        unpack_c2_3bit_block8(tmp + 0, in + 0);
        unpack_c2_3bit_block8(tmp + 8, in + 3);
        unpack_c2_3bit_block8(tmp + 16, in + 6);
        unpack_c2_3bit_block8(tmp + 24, in + 9);
        _mm256_storeu_si256((__m256i *)(void *)(com_c2 + i),
                            _mm256_load_si256((const __m256i *)(const void *)tmp));
    }
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


    poly_add_reduce_q_avx2(out, prod, e);

}


void poly_mul_add_with_cached_ntt(int16_t *out, const int16_t *a_ntt, const int16_t *b_ntt, const int16_t *e)
{
    int16_t prod[RL_KEM_N];

    mq_poly_pointwise_mul(prod, (int16_t *)a_ntt, (int16_t *)b_ntt);
    mq_poly_intt(prod);

    poly_add_reduce_q_avx2(out, prod, e);
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


    poly_add_reduce_q_avx2(out, prod, e);

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
    __attribute__((aligned(16))) uint8_t u_8[CODE_LEN]; /* source sequence: each element stores 8 bit */
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
