#include "axis_core.h"

#include <stdlib.h>
#include <string.h>

#include "axis_zero32_expr_word_generated.h"

#ifndef AXIS_OPT_LEVEL
#define AXIS_OPT_LEVEL 3
#endif

#define AXIS_OPT_LEVEL_PORTABLE 1
#define AXIS_OPT_LEVEL_AVX2 2
#define AXIS_OPT_LEVEL_BEST 3

#if AXIS_OPT_LEVEL < AXIS_OPT_LEVEL_PORTABLE || AXIS_OPT_LEVEL > AXIS_OPT_LEVEL_BEST
#error "AXIS_OPT_LEVEL must be 1(portable), 2(AVX2), or 3(best)"
#endif

#if (((AXIS_OPT_LEVEL >= AXIS_OPT_LEVEL_AVX2) && defined(__AVX2__)) || ((AXIS_OPT_LEVEL >= AXIS_OPT_LEVEL_BEST) && defined(__BMI2__))) && (defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86))
#include <immintrin.h>
#endif

#if (AXIS_OPT_LEVEL >= AXIS_OPT_LEVEL_BEST) && defined(__BMI2__) && (defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86))
#define AXIS_HAS_BMI2 1
#else
#define AXIS_HAS_BMI2 0
#endif

#if (AXIS_OPT_LEVEL >= AXIS_OPT_LEVEL_AVX2) && defined(__AVX2__) && (defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86))
#define AXIS_HAS_AVX2 1
#else
#define AXIS_HAS_AVX2 0
#endif

#if defined(__GNUC__) || defined(__clang__)
#define AXIS_MAYBE_UNUSED __attribute__((unused))
#define AXIS_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define AXIS_MAYBE_UNUSED
#define AXIS_ALWAYS_INLINE inline
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define AXIS_RESTRICT restrict
#else
#define AXIS_RESTRICT
#endif

#if defined(__GNUC__) || defined(__clang__)
#define AXIS_ALIGN32 __attribute__((aligned(32)))
#else
#define AXIS_ALIGN32
#endif

#define AXIS_INIT_WORDS \
    { \
        {0x9E3779B97F4A7C15ULL, 0xB7E151628AED2A6AULL, 0x243F6A8885A308D3ULL}, \
        {0x49CA76E19F938DA4ULL, 0xD81F8650363C93F8ULL, 0x7160C6B758B90A76ULL}, \
        {0xCD71C257B752EEB5ULL, 0xB54CBDE35B659CC7ULL, 0x70FF7724ACB762F7ULL}, \
        {0x47EA14CFA7D43857ULL, 0x3120355B0549D6B9ULL, 0x4FD6E6B4FBAB7ECDULL}, \
        {0x5DCD2735332E4029ULL, 0x03F92D7B7FAEEF10ULL, 0x6B634EF404919AAAULL}, \
        {0xD57AC41E845348D4ULL, 0xCD0814056FA6E00BULL, 0x53C1A2C96C4DE7E3ULL}, \
        {0xABDBF53A09476735ULL, 0x352FF6633C6EFBA6ULL, 0xF3FF90522F99DAE5ULL}, \
        {0x0D875B906F276D2CULL, 0xD94570230179F812ULL, 0xB1A270D3653FB5B8ULL} \
    }

static const uint64_t k_axis_init_words[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS] = AXIS_INIT_WORDS;

static const axis_variant_config_t k_axis_512_config = {
    AXIS_VARIANT_512,
    AXIS_MAX_REGISTERS,
    AXIS_512_RESULT_LENGTH,
    AXIS512_BLANK_ROUNDS,
    AXIS_LENGTH_BITS,
    AXIS_INIT_WORDS,
    {AXIS_EXL_0, AXIS_EXL_1, AXIS_EXL_2, AXIS_EXL_3, AXIS_EXL_4, AXIS_EXL_5},
    AXIS_EXL_PREV,
    {AXIS_UPL_0, AXIS_UPL_1, AXIS_UPL_2, AXIS_UPL_3, AXIS_UPL_4, AXIS_UPL_5}
};

static const axis_variant_config_t k_axis_1024_config = {
    AXIS_VARIANT_1024,
    AXIS_MAX_REGISTERS,
    AXIS_1024_RESULT_LENGTH,
    AXIS1024_BLANK_ROUNDS,
    AXIS_LENGTH_BITS,
    AXIS_INIT_WORDS,
    {AXIS_EXL_0, AXIS_EXL_1, AXIS_EXL_2, AXIS_EXL_3, AXIS_EXL_4, AXIS_EXL_5},
    AXIS_EXL_PREV,
    {AXIS_UPL_0, AXIS_UPL_1, AXIS_UPL_2, AXIS_UPL_3, AXIS_UPL_4, AXIS_UPL_5}
};

static const axis_variant_config_t k_axis_768_config = {
    AXIS_VARIANT_768,
    AXIS_MAX_REGISTERS,
    AXIS_768_RESULT_LENGTH,
    AXIS768_BLANK_ROUNDS,
    AXIS_LENGTH_BITS,
    AXIS_INIT_WORDS,
    {AXIS_EXL_0, AXIS_EXL_1, AXIS_EXL_2, AXIS_EXL_3, AXIS_EXL_4, AXIS_EXL_5},
    AXIS_EXL_PREV,
    {AXIS_UPL_0, AXIS_UPL_1, AXIS_UPL_2, AXIS_UPL_3, AXIS_UPL_4, AXIS_UPL_5}
};
/* Function: axis_default_config. Returns the static parameter set and initial state for an AXIS output variant. */

const axis_variant_config_t* axis_default_config(axis_variant_t variant) {
    if (variant == AXIS_VARIANT_1024) {
        return &k_axis_1024_config;
    }
    if (variant == AXIS_VARIANT_768) {
        return &k_axis_768_config;
    }
    return &k_axis_512_config;
}
/* Function: axis_result_bits. Returns the digest length in bits for an AXIS output variant. */

uint32_t axis_result_bits(axis_variant_t variant) {
    return axis_default_config(variant)->result_bits;
}
/* Function: axis_get_bit. Reads one bit from a packed 192-bit AXIS register using the document bit numbering. */

static uint8_t axis_get_bit(const uint64_t reg[AXIS_REGISTER_WORDS], uint32_t bit_index) {
    return (uint8_t)((reg[bit_index >> 6] >> (bit_index & 63U)) & 1U);
}
/* Function: axis_sbox6. Evaluates the six-input AXIS nonlinear SBox over GF(2). */

static uint8_t axis_sbox6(uint8_t x0, uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4, uint8_t x5) {
    return (uint8_t)((x0 & x1) ^ (x2 & x3) ^ (x4 & x5));
}

typedef struct AXIS_ALIGN32 axis_soa_state_t {
    uint64_t w0[AXIS_MAX_REGISTERS];
    uint64_t w1[AXIS_MAX_REGISTERS];
    uint64_t w2[AXIS_MAX_REGISTERS];
} axis_soa_state_t;

#if AXIS_HAS_AVX2
typedef struct AXIS_ALIGN32 axis_soa32_state_t {
    uint32_t w0l[AXIS_MAX_REGISTERS];
    uint32_t w0h[AXIS_MAX_REGISTERS];
    uint32_t w1l[AXIS_MAX_REGISTERS];
    uint32_t w1h[AXIS_MAX_REGISTERS];
    uint32_t w2l[AXIS_MAX_REGISTERS];
    uint32_t w2h[AXIS_MAX_REGISTERS];
} axis_soa32_state_t;
#endif
/* Function: axis_soa_from_regs. Converts packed register words into the scalar structure-of-arrays state layout. */

static AXIS_MAYBE_UNUSED void axis_soa_from_regs(axis_soa_state_t* AXIS_RESTRICT soa, uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS]) {
    uint32_t i;

    for (i = 0U; i < AXIS_MAX_REGISTERS; ++i) {
        soa->w0[i] = regs[i][0];
        soa->w1[i] = regs[i][1];
        soa->w2[i] = regs[i][2];
    }
}
/* Function: axis_soa_to_regs. Converts the scalar structure-of-arrays state layout back into packed registers. */

static AXIS_MAYBE_UNUSED void axis_soa_to_regs(const axis_soa_state_t* AXIS_RESTRICT soa, uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS]) {
    uint32_t i;

    for (i = 0U; i < AXIS_MAX_REGISTERS; ++i) {
        regs[i][0] = soa->w0[i];
        regs[i][1] = soa->w1[i];
        regs[i][2] = soa->w2[i];
    }
}
/* Function: axis_soa_zero32_doc_step. Applies the generated document-equivalent 32-beat zero update in scalar SoA layout. */

static AXIS_MAYBE_UNUSED void axis_soa_zero32_doc_step(axis_soa_state_t* AXIS_RESTRICT soa) {
    uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS];

    axis_soa_to_regs(soa, regs);
    axis_zero32_expr_word_step(regs);
    axis_soa_from_regs(soa, regs);
}

static AXIS_MAYBE_UNUSED void axis_soa_zero32_legacy_step(axis_soa_state_t* AXIS_RESTRICT soa);
/* Function: axis_reg_sbox. Implements the axis_reg_sbox helper used by the AXIS submission code. */

static AXIS_MAYBE_UNUSED uint8_t axis_reg_sbox(const uint64_t reg[AXIS_REGISTER_WORDS], const uint16_t exl[6]) {
    return axis_sbox6(
        axis_get_bit(reg, exl[0]),
        axis_get_bit(reg, exl[1]),
        axis_get_bit(reg, exl[2]),
        axis_get_bit(reg, exl[3]),
        axis_get_bit(reg, exl[4]),
        axis_get_bit(reg, exl[5]));
}
/* Function: axis_flip_bit. Conditionally toggles one bit in a packed AXIS register. */

static void axis_flip_bit(uint64_t reg[AXIS_REGISTER_WORDS], uint32_t bit_index, uint8_t bit) {
    if (bit != 0U) {
        reg[bit_index >> 6] ^= 1ULL << (bit_index & 63U);
    }
}
/* Function: axis_shift_right_one. Applies the document register shift by one bit with the circular boundary bit assignment. */

static void axis_shift_right_one(const uint64_t src[AXIS_REGISTER_WORDS], uint64_t dst[AXIS_REGISTER_WORDS]) {
    dst[0] = (src[0] >> 1) | (src[1] << 63);
    dst[1] = (src[1] >> 1) | (src[2] << 63);
    dst[2] = (src[2] >> 1) | ((src[0] & 1ULL) << 63);
}
/* Function: axis_shift_right_32. Applies thirty-two document shift beats at once to a packed AXIS register. */

static inline void axis_shift_right_32(const uint64_t src[AXIS_REGISTER_WORDS], uint64_t dst[AXIS_REGISTER_WORDS]) {
    dst[0] = (src[0] >> 32) | (src[1] << 32);
    dst[1] = (src[1] >> 32) | (src[2] << 32);
    dst[2] = (src[2] >> 32) | ((src[0] & 0xFFFFFFFFULL) << 32);
}
/* Function: axis_xor_tape32. XORs a 32-bit update tape into a packed register starting at the selected bit position. */

static inline void axis_xor_tape32(uint64_t reg[AXIS_REGISTER_WORDS], uint32_t start_bit, uint32_t tape) {
    const uint32_t word_index = start_bit >> 6;
    const uint32_t shift = start_bit & 63U;
    const uint64_t value = (uint64_t)tape;

    reg[word_index] ^= value << shift;
    if (shift > 32U) {
        reg[word_index + 1U] ^= value >> (64U - shift);
    }
}
/* Function: axis_compose_tape32. Builds the next packed register value for a 32-beat block from t0 and t1 update tapes. */

static inline void axis_compose_tape32(
    const uint64_t src[AXIS_REGISTER_WORDS],
    uint32_t t0_seq,
    uint32_t t1_seq,
    uint64_t dst[AXIS_REGISTER_WORDS]
) {
    axis_shift_right_32(src, dst);
    dst[0] ^= ((uint64_t)t1_seq << 58) ^ ((uint64_t)t1_seq << 43) ^ ((uint64_t)t1_seq << 34);
    dst[1] ^= ((uint64_t)t1_seq >> 6) ^ ((uint64_t)t1_seq >> 21) ^ ((uint64_t)t1_seq >> 30);
    dst[2] ^= ((uint64_t)t0_seq << 28) ^ ((uint64_t)t0_seq << 18) ^ ((uint64_t)t0_seq << 2);
}
/* Function: axis_compose_tape32_inplace. Updates one packed register in place using the precomputed 32-beat t0 and t1 tapes. */

static inline void axis_compose_tape32_inplace(
    uint64_t* AXIS_RESTRICT reg,
    uint32_t t0_seq,
    uint32_t t1_seq
) {
    const uint64_t w0 = reg[0];
    const uint64_t w1 = reg[1];
    const uint64_t w2 = reg[2];

    const uint64_t shifted0 = (w0 >> 32) | (w1 << 32);
    const uint64_t shifted1 = (w1 >> 32) | (w2 << 32);
    const uint64_t shifted2 = (w2 >> 32) | ((w0 & 0xFFFFFFFFULL) << 32);

    reg[0] = shifted0 ^ ((uint64_t)t1_seq << 58) ^ ((uint64_t)t1_seq << 43) ^ ((uint64_t)t1_seq << 34);
    reg[1] = shifted1 ^ ((uint64_t)t1_seq >> 6) ^ ((uint64_t)t1_seq >> 21) ^ ((uint64_t)t1_seq >> 30);
    reg[2] = shifted2 ^ ((uint64_t)t0_seq << 28) ^ ((uint64_t)t0_seq << 18) ^ ((uint64_t)t0_seq << 2);
}

#if AXIS_HAS_AVX2
/* Function: axis_soa_compose_tape32x4. Writes four scalar SoA register updates at once for the optimized grouped path. */
static AXIS_ALWAYS_INLINE void axis_soa_compose_tape32x4(
    axis_soa_state_t* AXIS_RESTRICT soa,
    uint32_t start,
    const uint32_t* AXIS_RESTRICT t0_seq,
    const uint32_t* AXIS_RESTRICT t1_seq
) {
    const __m256i w0 = _mm256_load_si256((const __m256i*)(soa->w0 + start));
    const __m256i w1 = _mm256_load_si256((const __m256i*)(soa->w1 + start));
    const __m256i w2 = _mm256_load_si256((const __m256i*)(soa->w2 + start));
    const __m256i t0 = _mm256_set_epi64x(
        (long long)t0_seq[start + 3U],
        (long long)t0_seq[start + 2U],
        (long long)t0_seq[start + 1U],
        (long long)t0_seq[start + 0U]);
    const __m256i t1 = _mm256_set_epi64x(
        (long long)t1_seq[start + 3U],
        (long long)t1_seq[start + 2U],
        (long long)t1_seq[start + 1U],
        (long long)t1_seq[start + 0U]);
    const __m256i shifted0 = _mm256_or_si256(_mm256_srli_epi64(w0, 32), _mm256_slli_epi64(w1, 32));
    const __m256i shifted1 = _mm256_or_si256(_mm256_srli_epi64(w1, 32), _mm256_slli_epi64(w2, 32));
    const __m256i shifted2 = _mm256_or_si256(
        _mm256_srli_epi64(w2, 32),
        _mm256_slli_epi64(_mm256_and_si256(w0, _mm256_set1_epi64x(0xFFFFFFFFLL)), 32));
    const __m256i r0 = _mm256_xor_si256(
        _mm256_xor_si256(shifted0, _mm256_slli_epi64(t1, 58)),
        _mm256_xor_si256(_mm256_slli_epi64(t1, 43), _mm256_slli_epi64(t1, 34)));
    const __m256i r1 = _mm256_xor_si256(
        _mm256_xor_si256(shifted1, _mm256_srli_epi64(t1, 6)),
        _mm256_xor_si256(_mm256_srli_epi64(t1, 21), _mm256_srli_epi64(t1, 30)));
    const __m256i r2 = _mm256_xor_si256(
        _mm256_xor_si256(shifted2, _mm256_slli_epi64(t0, 28)),
        _mm256_xor_si256(_mm256_slli_epi64(t0, 18), _mm256_slli_epi64(t0, 2)));

    _mm256_store_si256((__m256i*)(soa->w0 + start), r0);
    _mm256_store_si256((__m256i*)(soa->w1 + start), r1);
    _mm256_store_si256((__m256i*)(soa->w2 + start), r2);
}
/* Function: axis_compose_tape32x4. Implements the axis_compose_tape32x4 helper used by the AXIS submission code. */

static inline void axis_compose_tape32x4(
    uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS],
    uint32_t start,
    const uint32_t* AXIS_RESTRICT t0_seq,
    const uint32_t* AXIS_RESTRICT t1_seq
) {
    const __m256i w0 = _mm256_set_epi64x(
        (long long)regs[start + 3U][0],
        (long long)regs[start + 2U][0],
        (long long)regs[start + 1U][0],
        (long long)regs[start + 0U][0]);
    const __m256i w1 = _mm256_set_epi64x(
        (long long)regs[start + 3U][1],
        (long long)regs[start + 2U][1],
        (long long)regs[start + 1U][1],
        (long long)regs[start + 0U][1]);
    const __m256i w2 = _mm256_set_epi64x(
        (long long)regs[start + 3U][2],
        (long long)regs[start + 2U][2],
        (long long)regs[start + 1U][2],
        (long long)regs[start + 0U][2]);
    const __m256i t0 = _mm256_set_epi64x(
        (long long)t0_seq[start + 3U],
        (long long)t0_seq[start + 2U],
        (long long)t0_seq[start + 1U],
        (long long)t0_seq[start + 0U]);
    const __m256i t1 = _mm256_set_epi64x(
        (long long)t1_seq[start + 3U],
        (long long)t1_seq[start + 2U],
        (long long)t1_seq[start + 1U],
        (long long)t1_seq[start + 0U]);
    const __m256i shifted0 = _mm256_or_si256(_mm256_srli_epi64(w0, 32), _mm256_slli_epi64(w1, 32));
    const __m256i shifted1 = _mm256_or_si256(_mm256_srli_epi64(w1, 32), _mm256_slli_epi64(w2, 32));
    const __m256i shifted2 = _mm256_or_si256(
        _mm256_srli_epi64(w2, 32),
        _mm256_slli_epi64(_mm256_and_si256(w0, _mm256_set1_epi64x(0xFFFFFFFFLL)), 32));
    const __m256i r0 = _mm256_xor_si256(
        _mm256_xor_si256(shifted0, _mm256_slli_epi64(t1, 58)),
        _mm256_xor_si256(_mm256_slli_epi64(t1, 43), _mm256_slli_epi64(t1, 34)));
    const __m256i r1 = _mm256_xor_si256(
        _mm256_xor_si256(shifted1, _mm256_srli_epi64(t1, 6)),
        _mm256_xor_si256(_mm256_srli_epi64(t1, 21), _mm256_srli_epi64(t1, 30)));
    const __m256i r2 = _mm256_xor_si256(
        _mm256_xor_si256(shifted2, _mm256_slli_epi64(t0, 28)),
        _mm256_xor_si256(_mm256_slli_epi64(t0, 18), _mm256_slli_epi64(t0, 2)));
    regs[start + 0U][0] = (uint64_t)_mm256_extract_epi64(r0, 0);
    regs[start + 1U][0] = (uint64_t)_mm256_extract_epi64(r0, 1);
    regs[start + 2U][0] = (uint64_t)_mm256_extract_epi64(r0, 2);
    regs[start + 3U][0] = (uint64_t)_mm256_extract_epi64(r0, 3);
    regs[start + 0U][1] = (uint64_t)_mm256_extract_epi64(r1, 0);
    regs[start + 1U][1] = (uint64_t)_mm256_extract_epi64(r1, 1);
    regs[start + 2U][1] = (uint64_t)_mm256_extract_epi64(r1, 2);
    regs[start + 3U][1] = (uint64_t)_mm256_extract_epi64(r1, 3);
    regs[start + 0U][2] = (uint64_t)_mm256_extract_epi64(r2, 0);
    regs[start + 1U][2] = (uint64_t)_mm256_extract_epi64(r2, 1);
    regs[start + 2U][2] = (uint64_t)_mm256_extract_epi64(r2, 2);
    regs[start + 3U][2] = (uint64_t)_mm256_extract_epi64(r2, 3);
}
#endif
/* Function: axis_extract_seq32. Extracts thirty-two consecutive beat bits from a packed register. */

static inline uint32_t axis_extract_seq32(
    const uint64_t reg[AXIS_REGISTER_WORDS],
    uint32_t start_bit
) {
    const uint32_t word_index = start_bit >> 6;
    const uint32_t shift = start_bit & 63U;
    uint64_t value = reg[word_index] >> shift;

    if (shift > 32U && word_index + 1U < AXIS_REGISTER_WORDS) {
        value |= reg[word_index + 1U] << (64U - shift);
    }
    return (uint32_t)value;
}
/* Function: axis_reg_sbox_seq32. Computes the register-local SBox output sequence for a 32-beat window. */

static inline uint32_t axis_reg_sbox_seq32(
    const uint64_t reg[AXIS_REGISTER_WORDS],
    uint32_t advance
) {
    const uint32_t x0 = axis_extract_seq32(reg, AXIS_EXL_0 + advance);
    const uint32_t x1 = axis_extract_seq32(reg, AXIS_EXL_1 + advance);
    const uint32_t x2 = axis_extract_seq32(reg, AXIS_EXL_2 + advance);
    const uint32_t x3 = axis_extract_seq32(reg, AXIS_EXL_3 + advance);
    const uint32_t x4 = axis_extract_seq32(reg, AXIS_EXL_4 + advance);
    const uint32_t x5 = axis_extract_seq32(reg, AXIS_EXL_5 + advance);

    return (x0 & x1) ^ (x2 & x3) ^ (x4 & x5);
}
/* Function: axis_reg_sbox_seq32_a0. Computes the register-local SBox sequence for the current 32-beat alignment. */

static inline uint32_t axis_reg_sbox_seq32_a0(const uint64_t reg[AXIS_REGISTER_WORDS]) {
    const uint32_t x0 = axis_extract_seq32(reg, AXIS_EXL_0);
    const uint32_t x1 = axis_extract_seq32(reg, AXIS_EXL_1);
    const uint32_t x2 = axis_extract_seq32(reg, AXIS_EXL_2);
    const uint32_t x3 = axis_extract_seq32(reg, AXIS_EXL_3);
    const uint32_t x4 = axis_extract_seq32(reg, AXIS_EXL_4);
    const uint32_t x5 = axis_extract_seq32(reg, AXIS_EXL_5);

    return (x0 & x1) ^ (x2 & x3) ^ (x4 & x5);
}
/* Function: axis_reg_sbox_seq32_a1. Computes the register-local SBox sequence for the one-beat advanced alignment. */

static inline uint32_t axis_reg_sbox_seq32_a1(const uint64_t reg[AXIS_REGISTER_WORDS]) {
    const uint32_t x0 = (uint32_t)(reg[0] >> 1);
    const uint32_t x1 = (uint32_t)(reg[0] >> 13);
    const uint32_t x2 = (uint32_t)((reg[0] >> 33) | (reg[1] << 31));
    const uint32_t x3 = (uint32_t)((reg[1] >> 33) | (reg[2] << 31));
    const uint32_t x4 = (uint32_t)((reg[1] >> 44) | (reg[2] << 20));
    const uint32_t x5 = (uint32_t)((reg[1] >> 62) | (reg[2] << 2));

    return (x0 & x1) ^ (x2 & x3) ^ (x4 & x5);
}
/* Function: axis_soa_reg_sbox_seq32_a1. Computes a one-beat advanced register SBox sequence in scalar SoA layout. */

static AXIS_ALWAYS_INLINE uint32_t axis_soa_reg_sbox_seq32_a1(const axis_soa_state_t* AXIS_RESTRICT soa, uint32_t j) {
    const uint64_t w0 = soa->w0[j];
    const uint64_t w1 = soa->w1[j];
    const uint64_t w2 = soa->w2[j];
    const uint32_t x0 = (uint32_t)(w0 >> 1);
    const uint32_t x1 = (uint32_t)(w0 >> 13);
    const uint32_t x2 = (uint32_t)((w0 >> 33) | (w1 << 31));
    const uint32_t x3 = (uint32_t)((w1 >> 33) | (w2 << 31));
    const uint32_t x4 = (uint32_t)((w1 >> 44) | (w2 << 20));
    const uint32_t x5 = (uint32_t)((w1 >> 62) | (w2 << 2));

    return (x0 & x1) ^ (x2 & x3) ^ (x4 & x5);
}
/* Function: axis_soa_extract_seq32. Extracts a thirty-two-bit beat sequence from the scalar SoA state. */

static inline uint32_t axis_soa_extract_seq32(
    const axis_soa_state_t* AXIS_RESTRICT soa,
    uint32_t j,
    uint32_t start_bit
) {
    const uint32_t word_index = start_bit >> 6;
    const uint32_t shift = start_bit & 63U;
    uint64_t word;
    uint64_t value;

    if (word_index == 0U) {
        word = soa->w0[j];
        value = word >> shift;
        if (shift > 32U) {
            value |= soa->w1[j] << (64U - shift);
        }
    } else if (word_index == 1U) {
        word = soa->w1[j];
        value = word >> shift;
        if (shift > 32U) {
            value |= soa->w2[j] << (64U - shift);
        }
    } else {
        value = soa->w2[j] >> shift;
    }
    return (uint32_t)value;
}
/* Function: axis_soa_reg_sbox_current_seq32. Computes the current-register SBox sequence in scalar SoA layout. */

static AXIS_ALWAYS_INLINE uint32_t axis_soa_reg_sbox_current_seq32(const axis_soa_state_t* AXIS_RESTRICT soa, uint32_t j) {
    return
        (axis_soa_extract_seq32(soa, j, AXIS_EXL_5) & axis_soa_extract_seq32(soa, j, AXIS_EXL_2))
        ^ (axis_soa_extract_seq32(soa, j, AXIS_EXL_4) & axis_soa_extract_seq32(soa, j, AXIS_EXL_1))
        ^ (axis_soa_extract_seq32(soa, j, AXIS_EXL_3) & axis_soa_extract_seq32(soa, j, AXIS_EXL_0));
}
/* Function: axis_soa_extract_seq32_advanced. Extracts a scalar SoA beat sequence after a virtual shift advance. */

static inline uint32_t axis_soa_extract_seq32_advanced(
    const axis_soa_state_t* AXIS_RESTRICT soa,
    uint32_t j,
    uint32_t bit_index,
    uint32_t advance
) {
    return axis_soa_extract_seq32(soa, j, bit_index + advance);
}
/* Function: axis_soa_block32_sbox_new_formula. Evaluates the 32-beat UpRes SBox term in scalar SoA layout. */

static inline uint32_t axis_soa_block32_sbox_new_formula(
    const axis_soa_state_t* AXIS_RESTRICT soa,
    uint32_t j
) {
    const uint32_t next1 = (j + 1U) & 7U;
    const uint32_t next3 = (j + 3U) & 7U;
    const uint32_t next5 = (j + 5U) & 7U;
    const uint32_t advance1 = (next1 < j) ? 1U : 0U;
    const uint32_t advance3 = (next3 < j) ? 1U : 0U;
    const uint32_t advance5 = (next5 < j) ? 1U : 0U;
    const uint32_t x0 = axis_soa_extract_seq32_advanced(soa, next5, AXIS_EXL_3, advance5);
    const uint32_t x1 = axis_soa_extract_seq32_advanced(soa, next5, AXIS_EXL_2, advance5);
    const uint32_t x2 = axis_soa_extract_seq32_advanced(soa, next3, AXIS_EXL_5, advance3);
    const uint32_t x3 = axis_soa_extract_seq32_advanced(soa, next3, AXIS_EXL_1, advance3);
    const uint32_t x4 = axis_soa_extract_seq32_advanced(soa, next1, AXIS_EXL_4, advance1);
    const uint32_t x5 = axis_soa_extract_seq32_advanced(soa, next1, AXIS_EXL_0, advance1);

    return (x0 & x1) ^ (x2 & x3) ^ (x4 & x5);
}
/* Function: axis_soa_block32_prev_seq. Extracts the previous-register contribution in scalar SoA layout. */

static inline uint32_t axis_soa_block32_prev_seq(
    const axis_soa_state_t* AXIS_RESTRICT soa,
    uint32_t j
) {
    const uint32_t prev = (j + AXIS_MAX_REGISTERS - 1U) & 7U;
    const uint32_t advance = (j == 0U) ? 0U : 1U;

    return axis_soa_extract_seq32_advanced(soa, prev, AXIS_EXL_PREV, advance);
}
/* Function: axis_soa_make_tape32. Forms scalar SoA t0 and t1 update tapes for one target register. */

static inline void axis_soa_make_tape32(
    const axis_soa_state_t* AXIS_RESTRICT soa,
    uint32_t j,
    uint32_t b_seq,
    uint32_t* t0_seq,
    uint32_t* t1_seq
) {
    *t0_seq =
        axis_soa_extract_seq32(soa, j, AXIS_EXL_0)
        ^ axis_soa_extract_seq32(soa, j, AXIS_EXL_1)
        ^ axis_soa_extract_seq32(soa, j, AXIS_EXL_2)
        ^ b_seq;
    *t1_seq =
        axis_soa_extract_seq32(soa, j, AXIS_EXL_3)
        ^ axis_soa_extract_seq32(soa, j, AXIS_EXL_4)
        ^ axis_soa_extract_seq32(soa, j, AXIS_EXL_5)
        ^ b_seq;
}
/* Function: axis_soa_compose_tape32. Writes one scalar SoA register after applying the 32-beat update tapes. */

static inline void axis_soa_compose_tape32(
    const axis_soa_state_t* AXIS_RESTRICT source,
    axis_soa_state_t* AXIS_RESTRICT dst,
    uint32_t j,
    uint32_t t0_seq,
    uint32_t t1_seq
) {
    const uint64_t w0 = source->w0[j];
    const uint64_t w1 = source->w1[j];
    const uint64_t w2 = source->w2[j];

    dst->w0[j] = ((w0 >> 32) | (w1 << 32))
        ^ ((uint64_t)t1_seq << 58) ^ ((uint64_t)t1_seq << 43) ^ ((uint64_t)t1_seq << 34);
    dst->w1[j] = ((w1 >> 32) | (w2 << 32))
        ^ ((uint64_t)t1_seq >> 6) ^ ((uint64_t)t1_seq >> 21) ^ ((uint64_t)t1_seq >> 30);
    dst->w2[j] = ((w2 >> 32) | ((w0 & 0xFFFFFFFFULL) << 32))
        ^ ((uint64_t)t0_seq << 28) ^ ((uint64_t)t0_seq << 18) ^ ((uint64_t)t0_seq << 2);
}
/* Function: axis_reg_mix_seq32_a0. Computes the two-bit nonlinear contribution of a source register for the current 32-beat alignment. */

static inline void axis_reg_mix_seq32_a0(
    const uint64_t* AXIS_RESTRICT reg,
    uint32_t* AXIS_RESTRICT sbox_seq,
    uint32_t* AXIS_RESTRICT t0_base,
    uint32_t* AXIS_RESTRICT t1_base
) {
    const uint32_t x0 = (uint32_t)reg[0];
    const uint32_t x1 = (uint32_t)(reg[0] >> 12);
    const uint32_t x2 = (uint32_t)(reg[0] >> 32);
    const uint32_t x3 = (uint32_t)(reg[1] >> 32);
    const uint32_t x4 = (uint32_t)((reg[1] >> 43) | (reg[2] << 21));
    const uint32_t x5 = (uint32_t)((reg[1] >> 61) | (reg[2] << 3));

    *sbox_seq = (x0 & x1) ^ (x2 & x3) ^ (x4 & x5);
    *t0_base = x0 ^ x1 ^ x2;
    *t1_base = x3 ^ x4 ^ x5;
}

#if AXIS_HAS_AVX2
/* Function: axis_soa_reg_mix_seq32_a0x4. Computes four scalar SoA register-mix sequences for the optimized grouped path. */
static AXIS_ALWAYS_INLINE void axis_soa_reg_mix_seq32_a0x4(
    const axis_soa_state_t* AXIS_RESTRICT soa,
    uint32_t start,
    uint32_t* AXIS_RESTRICT sbox_seq,
    uint32_t* AXIS_RESTRICT t0_base,
    uint32_t* AXIS_RESTRICT t1_base
) {
    const __m256i w0 = _mm256_load_si256((const __m256i*)(soa->w0 + start));
    const __m256i w1 = _mm256_load_si256((const __m256i*)(soa->w1 + start));
    const __m256i w2 = _mm256_load_si256((const __m256i*)(soa->w2 + start));
    const __m256i x0 = w0;
    const __m256i x1 = _mm256_srli_epi64(w0, 12);
    const __m256i x2 = _mm256_srli_epi64(w0, 32);
    const __m256i x3 = _mm256_srli_epi64(w1, 32);
    const __m256i x4 = _mm256_or_si256(_mm256_srli_epi64(w1, 43), _mm256_slli_epi64(w2, 21));
    const __m256i x5 = _mm256_or_si256(_mm256_srli_epi64(w1, 61), _mm256_slli_epi64(w2, 3));
    const __m256i s = _mm256_xor_si256(
        _mm256_xor_si256(_mm256_and_si256(x0, x1), _mm256_and_si256(x2, x3)),
        _mm256_and_si256(x4, x5));
    const __m256i t0 = _mm256_xor_si256(_mm256_xor_si256(x0, x1), x2);
    const __m256i t1 = _mm256_xor_si256(_mm256_xor_si256(x3, x4), x5);
    uint64_t lanes[4] AXIS_ALIGN32;

    _mm256_store_si256((__m256i*)lanes, s);
    sbox_seq[start + 0U] = (uint32_t)lanes[0];
    sbox_seq[start + 1U] = (uint32_t)lanes[1];
    sbox_seq[start + 2U] = (uint32_t)lanes[2];
    sbox_seq[start + 3U] = (uint32_t)lanes[3];
    _mm256_store_si256((__m256i*)lanes, t0);
    t0_base[start + 0U] = (uint32_t)lanes[0];
    t0_base[start + 1U] = (uint32_t)lanes[1];
    t0_base[start + 2U] = (uint32_t)lanes[2];
    t0_base[start + 3U] = (uint32_t)lanes[3];
    _mm256_store_si256((__m256i*)lanes, t1);
    t1_base[start + 0U] = (uint32_t)lanes[0];
    t1_base[start + 1U] = (uint32_t)lanes[1];
    t1_base[start + 2U] = (uint32_t)lanes[2];
    t1_base[start + 3U] = (uint32_t)lanes[3];
}
/* Function: axis_reg_mix_seq32_a0x4. Computes four packed register-mix sequences in parallel for the optimized 32-beat path. */

static inline void axis_reg_mix_seq32_a0x4(
    uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS],
    uint32_t start,
    uint32_t* AXIS_RESTRICT sbox_seq,
    uint32_t* AXIS_RESTRICT t0_base,
    uint32_t* AXIS_RESTRICT t1_base
) {
    const __m256i w0 = _mm256_set_epi64x(
        (long long)regs[start + 3U][0],
        (long long)regs[start + 2U][0],
        (long long)regs[start + 1U][0],
        (long long)regs[start + 0U][0]);
    const __m256i w1 = _mm256_set_epi64x(
        (long long)regs[start + 3U][1],
        (long long)regs[start + 2U][1],
        (long long)regs[start + 1U][1],
        (long long)regs[start + 0U][1]);
    const __m256i w2 = _mm256_set_epi64x(
        (long long)regs[start + 3U][2],
        (long long)regs[start + 2U][2],
        (long long)regs[start + 1U][2],
        (long long)regs[start + 0U][2]);
    const __m256i x0 = w0;
    const __m256i x1 = _mm256_srli_epi64(w0, 12);
    const __m256i x2 = _mm256_srli_epi64(w0, 32);
    const __m256i x3 = _mm256_srli_epi64(w1, 32);
    const __m256i x4 = _mm256_or_si256(_mm256_srli_epi64(w1, 43), _mm256_slli_epi64(w2, 21));
    const __m256i x5 = _mm256_or_si256(_mm256_srli_epi64(w1, 61), _mm256_slli_epi64(w2, 3));
    const __m256i s = _mm256_xor_si256(
        _mm256_xor_si256(_mm256_and_si256(x0, x1), _mm256_and_si256(x2, x3)),
        _mm256_and_si256(x4, x5));
    const __m256i t0 = _mm256_xor_si256(_mm256_xor_si256(x0, x1), x2);
    const __m256i t1 = _mm256_xor_si256(_mm256_xor_si256(x3, x4), x5);
    {
        uint64_t lanes[4];
        uint32_t i;

        _mm256_storeu_si256((__m256i*)lanes, s);
        for (i = 0U; i < 4U; ++i) {
            sbox_seq[start + i] = (uint32_t)lanes[i];
        }
        _mm256_storeu_si256((__m256i*)lanes, t0);
        for (i = 0U; i < 4U; ++i) {
            t0_base[start + i] = (uint32_t)lanes[i];
        }
        _mm256_storeu_si256((__m256i*)lanes, t1);
        for (i = 0U; i < 4U; ++i) {
            t1_base[start + i] = (uint32_t)lanes[i];
        }
    }
}
#endif

static AXIS_ALWAYS_INLINE void AXIS_MAYBE_UNUSED axis_soa_step_default_block32(axis_soa_state_t* AXIS_RESTRICT soa, uint32_t m0_seq, uint32_t m1_seq);
static AXIS_ALWAYS_INLINE void AXIS_MAYBE_UNUSED axis_soa_step_default_block32_register_seqs(axis_soa_state_t* AXIS_RESTRICT soa, const uint32_t message_seq[AXIS_MAX_REGISTERS]);
static void axis_step_default(axis_core_t* ctx, uint8_t m0, uint8_t m1);
static void axis_emit_digest(axis_core_t* ctx, uint8_t* digest);
/* Function: axis_make_tape32. Forms the t0 and t1 update tapes for a 32-beat target-register update. */

static inline void AXIS_MAYBE_UNUSED axis_make_tape32(
    const uint64_t reg[AXIS_REGISTER_WORDS],
    uint32_t b_seq,
    uint32_t* t0_seq,
    uint32_t* t1_seq
) {
    *t0_seq =
        axis_extract_seq32(reg, AXIS_EXL_0)
        ^ axis_extract_seq32(reg, AXIS_EXL_1)
        ^ axis_extract_seq32(reg, AXIS_EXL_2)
        ^ b_seq;
    *t1_seq =
        axis_extract_seq32(reg, AXIS_EXL_3)
        ^ axis_extract_seq32(reg, AXIS_EXL_4)
        ^ axis_extract_seq32(reg, AXIS_EXL_5)
        ^ b_seq;
}
/* Function: axis_extract_seq32_advanced. Extracts a thirty-two-bit sequence after applying an additional virtual shift advance. */

static inline uint32_t axis_extract_seq32_advanced(
    const uint64_t reg[AXIS_REGISTER_WORDS],
    uint32_t bit_index,
    uint32_t advance
) {
    return axis_extract_seq32(reg, bit_index + advance);
}
/* Function: axis_reg_sbox_current_seq32. Computes the current-register SBox sequence used by the b term over thirty-two beats. */

static inline uint32_t axis_reg_sbox_current_seq32(const uint64_t reg[AXIS_REGISTER_WORDS]) {
    return
        (axis_extract_seq32(reg, AXIS_EXL_5) & axis_extract_seq32(reg, AXIS_EXL_2))
        ^ (axis_extract_seq32(reg, AXIS_EXL_4) & axis_extract_seq32(reg, AXIS_EXL_1))
        ^ (axis_extract_seq32(reg, AXIS_EXL_3) & axis_extract_seq32(reg, AXIS_EXL_0));
}
/* Function: axis_block32_sbox_new_formula. Evaluates the document UpRes SBox term over thirty-two beats for one target register. */

static inline uint32_t axis_block32_sbox_new_formula(
    uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS],
    uint32_t j
) {
    const uint32_t next1 = (j + 1U) & 7U;
    const uint32_t next3 = (j + 3U) & 7U;
    const uint32_t next5 = (j + 5U) & 7U;
    const uint32_t advance1 = (next1 < j) ? 1U : 0U;
    const uint32_t advance3 = (next3 < j) ? 1U : 0U;
    const uint32_t advance5 = (next5 < j) ? 1U : 0U;
    const uint32_t x0 = axis_extract_seq32_advanced(regs[next5], AXIS_EXL_3, advance5);
    const uint32_t x1 = axis_extract_seq32_advanced(regs[next5], AXIS_EXL_2, advance5);
    const uint32_t x2 = axis_extract_seq32_advanced(regs[next3], AXIS_EXL_5, advance3);
    const uint32_t x3 = axis_extract_seq32_advanced(regs[next3], AXIS_EXL_1, advance3);
    const uint32_t x4 = axis_extract_seq32_advanced(regs[next1], AXIS_EXL_4, advance1);
    const uint32_t x5 = axis_extract_seq32_advanced(regs[next1], AXIS_EXL_0, advance1);

    return (x0 & x1) ^ (x2 & x3) ^ (x4 & x5);
}
/* Function: axis_block32_prev_seq. Extracts the previous-register linear contribution for the 32-beat update formula. */

static inline uint32_t axis_block32_prev_seq(
    uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS],
    uint32_t j
) {
    const uint32_t prev = (j + AXIS_MAX_REGISTERS - 1U) & 7U;
    const uint32_t advance = (j == 0U) ? 0U : 1U;

    return axis_extract_seq32_advanced(regs[prev], AXIS_EXL_PREV, advance);
}
/* Function: axis_step_default_block32_register_seqs. Applies one 32-beat UpFull block when each register has its own prepacked message sequence. */

static void axis_step_default_block32_register_seqs(axis_core_t* ctx, const uint32_t message_seq[AXIS_MAX_REGISTERS]) {
    uint64_t source[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS];
    uint32_t t0_seq[AXIS_MAX_REGISTERS];
    uint32_t t1_seq[AXIS_MAX_REGISTERS];
    uint32_t j;

    memcpy(source, ctx->regs, sizeof(source));
    for (j = 0U; j < AXIS_MAX_REGISTERS; ++j) {
        const uint32_t b_seq =
            message_seq[j]
            ^ axis_reg_sbox_current_seq32(source[j])
            ^ axis_block32_sbox_new_formula(source, j)
            ^ axis_block32_prev_seq(source, j);

        axis_make_tape32(source[j], b_seq, &t0_seq[j], &t1_seq[j]);
    }
    for (j = 0U; j < AXIS_MAX_REGISTERS; ++j) {
        axis_compose_tape32(source[j], t0_seq[j], t1_seq[j], ctx->regs[j]);
    }
}
/* Function: axis_step_default_block32. Applies one 32-beat UpFull block from the two alternating message-bit sequences. */

static void axis_step_default_block32(axis_core_t* ctx, uint32_t m0_seq, uint32_t m1_seq) {
    uint32_t message_seq[AXIS_MAX_REGISTERS];
    uint32_t j;

    for (j = 0U; j < AXIS_MAX_REGISTERS; ++j) {
        message_seq[j] = ((j & 1U) == 0U) ? m0_seq : m1_seq;
    }
    axis_step_default_block32_register_seqs(ctx, message_seq);
}
/* Function: axis_soa_step_default_block32_register_seqs. Applies a 32-beat scalar SoA UpFull block with per-register message sequences. */

static AXIS_ALWAYS_INLINE void AXIS_MAYBE_UNUSED axis_soa_step_default_block32_register_seqs(axis_soa_state_t* AXIS_RESTRICT soa, const uint32_t message_seq[AXIS_MAX_REGISTERS]) {
    axis_core_t temp;

    axis_core_init(&temp, AXIS_VARIANT_512);
    axis_soa_to_regs(soa, temp.regs);
    axis_step_default_block32_register_seqs(&temp, message_seq);
    axis_soa_from_regs(soa, temp.regs);
}
/* Function: axis_soa_step_default_block32. Applies a 32-beat scalar SoA UpFull block with alternating message sequences. */

static AXIS_ALWAYS_INLINE void AXIS_MAYBE_UNUSED axis_soa_step_default_block32(axis_soa_state_t* AXIS_RESTRICT soa, uint32_t m0_seq, uint32_t m1_seq) {
    uint32_t message_seq[AXIS_MAX_REGISTERS];
    uint32_t j;

    for (j = 0U; j < AXIS_MAX_REGISTERS; ++j) {
        message_seq[j] = ((j & 1U) == 0U) ? m0_seq : m1_seq;
    }
    axis_soa_step_default_block32_register_seqs(soa, message_seq);
}
/* Function: axis_step_default_block32_zero. Applies one optimized 32-beat zero-message update block. */

static void axis_step_default_block32_zero(axis_core_t* ctx) {
    axis_step_default_block32(ctx, 0U, 0U);
}
/* Function: axis_step_default_block32_zero_legacy. Applies the legacy scalar 32-beat zero-message update path kept for comparison and fallback. */

static AXIS_MAYBE_UNUSED void axis_step_default_block32_zero_legacy(axis_core_t* ctx) {
    axis_step_default_block32_zero(ctx);
}
/* Function: axis_soa_zero32_legacy_step. Applies the legacy scalar SoA zero-update step kept as a fallback implementation. */

static AXIS_MAYBE_UNUSED void axis_soa_zero32_legacy_step(axis_soa_state_t* AXIS_RESTRICT soa) {
    axis_core_t temp;

    axis_core_init(&temp, AXIS_VARIANT_512);
    axis_soa_to_regs(soa, temp.regs);
    axis_step_default_block32_zero_legacy(&temp);
    axis_soa_from_regs(soa, temp.regs);
}
/* Function: axis_reg_sbox_current. Computes the current-register SBox value for a single beat. */

static AXIS_MAYBE_UNUSED uint8_t axis_reg_sbox_current(const uint64_t reg[AXIS_REGISTER_WORDS]) {
    return (uint8_t)(
        (axis_get_bit(reg, AXIS_EXL_5) & axis_get_bit(reg, AXIS_EXL_2))
        ^ (axis_get_bit(reg, AXIS_EXL_4) & axis_get_bit(reg, AXIS_EXL_1))
        ^ (axis_get_bit(reg, AXIS_EXL_3) & axis_get_bit(reg, AXIS_EXL_0)));
}
/* Function: axis_upres_b_default. Computes the b term of the document UpRes function for one register and one message bit. */

static uint8_t axis_upres_b_default(const axis_core_t* ctx, uint32_t j, uint8_t message_bit) {
    const uint32_t prev = (j + AXIS_MAX_REGISTERS - 1U) % AXIS_MAX_REGISTERS;
    const uint32_t next1 = (j + 1U) % AXIS_MAX_REGISTERS;
    const uint32_t next3 = (j + 3U) % AXIS_MAX_REGISTERS;
    const uint32_t next5 = (j + 5U) % AXIS_MAX_REGISTERS;

    return (uint8_t)(
        message_bit
        ^ axis_reg_sbox_current(ctx->regs[j])
        ^ axis_sbox6(
            axis_get_bit(ctx->regs[next5], AXIS_EXL_3),
            axis_get_bit(ctx->regs[next5], AXIS_EXL_2),
            axis_get_bit(ctx->regs[next3], AXIS_EXL_5),
            axis_get_bit(ctx->regs[next3], AXIS_EXL_1),
            axis_get_bit(ctx->regs[next1], AXIS_EXL_4),
            axis_get_bit(ctx->regs[next1], AXIS_EXL_0))
        ^ axis_get_bit(ctx->regs[prev], AXIS_EXL_PREV));
}
/* Function: axis_step_default. Applies one document UpFull beat with the default eight-register update rule. */

static void axis_step_default(axis_core_t* ctx, uint8_t m0, uint8_t m1) {
    const uint32_t register_count = AXIS_MAX_REGISTERS;
    uint64_t old_reg[AXIS_REGISTER_WORDS];
    uint32_t j;

    for (j = 0; j < register_count; ++j) {
        const uint8_t message_bit = ((j & 1U) == 0U) ? m0 : m1;

        memcpy(old_reg, ctx->regs[j], sizeof(old_reg));
        const uint8_t b = axis_upres_b_default(ctx, j, message_bit);
        const uint8_t t0 = (uint8_t)(
            axis_get_bit(old_reg, AXIS_EXL_0)
            ^ axis_get_bit(old_reg, AXIS_EXL_1)
            ^ axis_get_bit(old_reg, AXIS_EXL_2)
            ^ b);
        const uint8_t t1 = (uint8_t)(
            axis_get_bit(old_reg, AXIS_EXL_3)
            ^ axis_get_bit(old_reg, AXIS_EXL_4)
            ^ axis_get_bit(old_reg, AXIS_EXL_5)
            ^ b);
        axis_shift_right_one(old_reg, ctx->regs[j]);
        axis_flip_bit(ctx->regs[j], AXIS_UPL_0 - 1U, t1);
        axis_flip_bit(ctx->regs[j], AXIS_UPL_1 - 1U, t1);
        axis_flip_bit(ctx->regs[j], AXIS_UPL_2 - 1U, t1);
        axis_flip_bit(ctx->regs[j], AXIS_UPL_3 - 1U, t0);
        axis_flip_bit(ctx->regs[j], AXIS_UPL_4 - 1U, t0);
        axis_flip_bit(ctx->regs[j], AXIS_UPL_5 - 1U, t0);
    }
}
/* Function: axis_step_generic. Applies one UpFull beat using the configured register count and index tables. */

static void axis_step_generic(axis_core_t* ctx, uint8_t m0, uint8_t m1) {
    const axis_variant_config_t* config = ctx->config;
    const uint32_t register_count = config->register_count;
    uint64_t old_reg[AXIS_REGISTER_WORDS];
    uint32_t j;

    for (j = 0; j < register_count; ++j) {
        const uint32_t prev = (j == 0U) ? (register_count - 1U) : (j - 1U);
        const uint32_t next1 = (j + 1U) % register_count;
        const uint8_t message_bit = ((j & 1U) == 0U) ? m0 : m1;

        memcpy(old_reg, ctx->regs[j], sizeof(old_reg));
        const uint32_t next3 = (j + 3U) % register_count;
        const uint32_t next5 = (j + 5U) % register_count;
        const uint8_t b = (uint8_t)(
            message_bit
            ^ axis_sbox6(
                axis_get_bit(old_reg, config->exl[5]),
                axis_get_bit(old_reg, config->exl[2]),
                axis_get_bit(old_reg, config->exl[4]),
                axis_get_bit(old_reg, config->exl[1]),
                axis_get_bit(old_reg, config->exl[3]),
                axis_get_bit(old_reg, config->exl[0]))
            ^ axis_sbox6(
                axis_get_bit(ctx->regs[next5], config->exl[3]),
                axis_get_bit(ctx->regs[next5], config->exl[2]),
                axis_get_bit(ctx->regs[next3], config->exl[5]),
                axis_get_bit(ctx->regs[next3], config->exl[1]),
                axis_get_bit(ctx->regs[next1], config->exl[4]),
                axis_get_bit(ctx->regs[next1], config->exl[0]))
            ^ axis_get_bit(ctx->regs[prev], config->exl_prev));
        const uint8_t t0 = (uint8_t)(
            axis_get_bit(old_reg, config->exl[0])
            ^ axis_get_bit(old_reg, config->exl[1])
            ^ axis_get_bit(old_reg, config->exl[2])
            ^ b);
        const uint8_t t1 = (uint8_t)(
            axis_get_bit(old_reg, config->exl[3])
            ^ axis_get_bit(old_reg, config->exl[4])
            ^ axis_get_bit(old_reg, config->exl[5])
            ^ b);
        axis_shift_right_one(old_reg, ctx->regs[j]);
        axis_flip_bit(ctx->regs[j], (uint32_t)config->upl[0] - 1U, t1);
        axis_flip_bit(ctx->regs[j], (uint32_t)config->upl[1] - 1U, t1);
        axis_flip_bit(ctx->regs[j], (uint32_t)config->upl[2] - 1U, t1);
        axis_flip_bit(ctx->regs[j], (uint32_t)config->upl[3] - 1U, t0);
        axis_flip_bit(ctx->regs[j], (uint32_t)config->upl[4] - 1U, t0);
        axis_flip_bit(ctx->regs[j], (uint32_t)config->upl[5] - 1U, t0);
    }
}
/* Function: axis_step. Dispatches a single UpFull beat to the specialized or generic update implementation. */

static void axis_step(axis_core_t* ctx, uint8_t m0, uint8_t m1) {
    if (ctx->config == &k_axis_512_config || ctx->config == &k_axis_1024_config) {
        axis_step_default(ctx, m0, m1);
        return;
    }
    axis_step_generic(ctx, m0, m1);
}
/* Function: axis_constant_stream_bit. Returns one bit from the repeated finalization constant stream determined by the padded length. */

static uint8_t axis_constant_stream_bit(uint64_t padded_bits, uint64_t bit_index) {
    const uint64_t stream_bits = (uint64_t)AXIS_MAX_REGISTERS * (uint64_t)AXIS_REGISTER_BITS;
    const uint64_t position = (padded_bits + bit_index) % stream_bits;
    const uint32_t constant_index = (uint32_t)(position / AXIS_REGISTER_BITS);
    const uint32_t bit_in_constant = (uint32_t)(position % AXIS_REGISTER_BITS);

    return axis_get_bit(k_axis_init_words[constant_index], bit_in_constant);
}
/* Function: axis_reupfull_constant_byte. Packs the eight constant-stream bits used by one ReUpFull beat. */

static uint8_t axis_reupfull_constant_byte(uint64_t padded_bits, uint64_t beat) {
    uint8_t bits = 0U;
    uint32_t j;

    for (j = 0U; j < AXIS_MAX_REGISTERS; ++j) {
        bits |= (uint8_t)(axis_constant_stream_bit(padded_bits, beat * 8ULL + j) << j);
    }
    return bits;
}
/* Function: axis_reupfull_constant_seq32. Builds thirty-two beats of per-register ReUpFull constant bits. */

static void axis_reupfull_constant_seq32(uint64_t padded_bits, uint64_t beat, uint32_t message_seq[AXIS_MAX_REGISTERS]) {
    uint64_t position = (padded_bits + beat * 8ULL) % (AXIS_MAX_REGISTERS * AXIS_REGISTER_BITS);
    uint32_t constant_index = (uint32_t)(position / AXIS_REGISTER_BITS);
    uint32_t bit_in_constant = (uint32_t)(position - (uint64_t)constant_index * AXIS_REGISTER_BITS);
    uint32_t k;
    uint32_t j;

    for (j = 0U; j < AXIS_MAX_REGISTERS; ++j) {
        message_seq[j] = 0U;
    }
    for (k = 0U; k < 32U; ++k) {
        for (j = 0U; j < AXIS_MAX_REGISTERS; ++j) {
            message_seq[j] |= (uint32_t)axis_get_bit(k_axis_init_words[constant_index], bit_in_constant) << k;
            ++bit_in_constant;
            if (bit_in_constant == AXIS_REGISTER_BITS) {
                bit_in_constant = 0U;
                constant_index = (constant_index + 1U) & 7U;
            }
        }
    }
}
/* Function: axis_reupfull. Applies one ReUpFull beat using the eight constant bits supplied for the registers. */

static void axis_reupfull(axis_core_t* ctx, uint8_t constant_bits) {
    const axis_variant_config_t* config = ctx->config;
    const uint32_t register_count = config->register_count;
    uint64_t old_reg[AXIS_REGISTER_WORDS];
    uint32_t j;

    for (j = 0U; j < register_count; ++j) {
        const uint32_t prev = (j == 0U) ? (register_count - 1U) : (j - 1U);
        const uint32_t next1 = (j + 1U) % register_count;
        const uint32_t next3 = (j + 3U) % register_count;
        const uint32_t next5 = (j + 5U) % register_count;
        const uint8_t message_bit = (uint8_t)((constant_bits >> j) & 1U);
        const uint8_t b = (uint8_t)(
            message_bit
            ^ axis_sbox6(
                axis_get_bit(ctx->regs[j], config->exl[5]),
                axis_get_bit(ctx->regs[j], config->exl[2]),
                axis_get_bit(ctx->regs[j], config->exl[4]),
                axis_get_bit(ctx->regs[j], config->exl[1]),
                axis_get_bit(ctx->regs[j], config->exl[3]),
                axis_get_bit(ctx->regs[j], config->exl[0]))
            ^ axis_sbox6(
                axis_get_bit(ctx->regs[next5], config->exl[3]),
                axis_get_bit(ctx->regs[next5], config->exl[2]),
                axis_get_bit(ctx->regs[next3], config->exl[5]),
                axis_get_bit(ctx->regs[next3], config->exl[1]),
                axis_get_bit(ctx->regs[next1], config->exl[4]),
                axis_get_bit(ctx->regs[next1], config->exl[0]))
            ^ axis_get_bit(ctx->regs[prev], config->exl_prev));
        const uint8_t t0 = (uint8_t)(
            axis_get_bit(ctx->regs[j], config->exl[0])
            ^ axis_get_bit(ctx->regs[j], config->exl[1])
            ^ axis_get_bit(ctx->regs[j], config->exl[2])
            ^ b);
        const uint8_t t1 = (uint8_t)(
            axis_get_bit(ctx->regs[j], config->exl[3])
            ^ axis_get_bit(ctx->regs[j], config->exl[4])
            ^ axis_get_bit(ctx->regs[j], config->exl[5])
            ^ b);

        memcpy(old_reg, ctx->regs[j], sizeof(old_reg));
        axis_flip_bit(old_reg, (uint32_t)config->upl[0], t1);
        axis_flip_bit(old_reg, (uint32_t)config->upl[1], t1);
        axis_flip_bit(old_reg, (uint32_t)config->upl[2], t1);
        axis_flip_bit(old_reg, (uint32_t)config->upl[3], t0);
        axis_flip_bit(old_reg, (uint32_t)config->upl[4], t0);
        axis_flip_bit(old_reg, (uint32_t)config->upl[5], t0);
        axis_shift_right_one(old_reg, ctx->regs[j]);
    }
}
/* Function: axis_absorb_stream_bit. Feeds one padded message bit into the pending AXIS message-beat scheduler. */

static void axis_absorb_stream_bit(axis_core_t* ctx, uint8_t bit) {
    bit = (uint8_t)(bit & 1U);
    if (ctx->config->variant == AXIS_VARIANT_1024) {
        axis_step(ctx, bit, bit);
        return;
    }

    ctx->pending_bits[ctx->pending_len++] = bit;
    if (ctx->pending_len == 2U) {
        axis_step(ctx, ctx->pending_bits[1], ctx->pending_bits[0]);
        ctx->pending_len = 0U;
    }
}
/* Function: axis_binary64_double_bits. Encodes the 64-bit length field with each source bit duplicated as required by the AXIS padding rule. */

static uint64_t axis_binary64_double_bits(uint64_t value) {
    return value;
}
/* Function: axis_768_one_bit_tail. Computes the AXIS-768 tail-mode threshold where the final pending bits switch to one-bit beats. */

static uint64_t axis_768_one_bit_tail(uint64_t padded_bits) {
    const uint64_t block_mod = (padded_bits >> 6) % 3ULL;

    if (block_mod == 1ULL) {
        return 64ULL;
    }
    if (block_mod == 2ULL) {
        return 32ULL;
    }
    return 0ULL;
}
/* Function: axis_constant_pair. Extracts the two constant-stream bits used by a scalar finalization beat. */

static AXIS_MAYBE_UNUSED void axis_constant_pair(uint64_t constant_bits, uint64_t beat, uint8_t* high, uint8_t* low) {
    const uint32_t high_pos = (uint32_t)((beat << 1) & 63ULL);
    const uint32_t low_pos = (high_pos + 1U) & 63U;
    const uint32_t high_shift = 63U - high_pos;
    const uint32_t low_shift = 63U - low_pos;

    *high = (uint8_t)((constant_bits >> high_shift) & 1ULL);
    *low = (uint8_t)((constant_bits >> low_shift) & 1ULL);
}
/* Function: axis_constant_pair_seq32. Packs thirty-two finalization constant pairs for block processing. */

static AXIS_MAYBE_UNUSED void axis_constant_pair_seq32(uint64_t constant_bits, uint32_t* m0_seq, uint32_t* m1_seq) {
    uint32_t i;
    uint32_t m0 = 0U;
    uint32_t m1 = 0U;

    for (i = 0U; i < 32U; ++i) {
        m0 |= (uint32_t)((constant_bits >> (63U - 2U * i)) & 1ULL) << i;
        m1 |= (uint32_t)((constant_bits >> (62U - 2U * i)) & 1ULL) << i;
    }
    *m0_seq = m0;
    *m1_seq = m1;
}
/* Function: axis_read_input_bit. Reads one input bit from a byte string in the external message-bit order. */

static uint8_t axis_read_input_bit(const uint8_t* bytes, uint64_t bit_index) {
    const uint8_t value = bytes[bit_index >> 3];
    const uint8_t shift = (uint8_t)(7U - (bit_index & 7U));
    return (uint8_t)((value >> shift) & 1U);
}
/* Function: axis_get_stored_message_bit. Reads one bit from the internally stored bit-addressed message buffer. */

static uint8_t axis_get_stored_message_bit(const axis_core_t* ctx, uint64_t bit_index) {
    const uint8_t value = ctx->message[bit_index >> 3];
    const uint8_t shift = (uint8_t)(7U - (bit_index & 7U));
    return (uint8_t)((value >> shift) & 1U);
}
/* Function: axis_set_stored_message_bit. Writes one bit into the internally stored bit-addressed message buffer. */

static void axis_set_stored_message_bit(axis_core_t* ctx, uint64_t bit_index, uint8_t bit) {
    const uint64_t byte_index = bit_index >> 3;
    const uint8_t mask = (uint8_t)(1U << (7U - (bit_index & 7U)));

    if (bit != 0U) {
        ctx->message[byte_index] |= mask;
    } else {
        ctx->message[byte_index] &= (uint8_t)~mask;
    }
}
/* Function: axis_reserve_message_bits. Ensures that the context owns enough storage for the requested number of message bits. */

static int axis_reserve_message_bits(axis_core_t* ctx, uint64_t required_bits) {
    const uint64_t required_bytes = (required_bits + 7ULL) >> 3;
    const uint64_t current_bytes = (ctx->message_capacity_bits + 7ULL) >> 3;
    uint64_t new_capacity_bits;
    uint64_t new_bytes;
    uint8_t* new_message;

    if (required_bits <= ctx->message_capacity_bits) {
        return 0;
    }

    new_capacity_bits = (ctx->message_capacity_bits == 0ULL) ? 1024ULL : ctx->message_capacity_bits;
    while (new_capacity_bits < required_bits) {
        if (new_capacity_bits > UINT64_MAX / 2ULL) {
            new_capacity_bits = required_bits;
            break;
        }
        new_capacity_bits *= 2ULL;
    }
    new_bytes = (new_capacity_bits + 7ULL) >> 3;
    if (new_bytes < required_bytes) {
        return -1;
    }

    if (ctx->message_owned != 0U) {
        new_message = (uint8_t*)realloc(ctx->message, (size_t)new_bytes);
    } else {
        new_message = (uint8_t*)malloc((size_t)new_bytes);
        if (new_message != 0 && current_bytes != 0ULL && ctx->message != 0) {
            memcpy(new_message, ctx->message, (size_t)current_bytes);
        }
    }
    if (new_message == 0) {
        return -1;
    }
    if (new_bytes > current_bytes) {
        memset(new_message + current_bytes, 0, (size_t)(new_bytes - current_bytes));
    }
    ctx->message = new_message;
    ctx->message_capacity_bits = new_capacity_bits;
    ctx->message_owned = 1U;
    return 0;
}
/* Function: axis_padded_bit_at. Returns one bit of the Pad10*1-plus-length padded message stream. */

static uint8_t axis_padded_bit_at(const axis_core_t* ctx, uint64_t padded_index, uint64_t zero_count, uint64_t length_bits) {
    const uint64_t message_bits = ctx->total_message_bits;

    if (padded_index < message_bits) {
        return axis_get_stored_message_bit(ctx, padded_index);
    }
    padded_index -= message_bits;
    if (padded_index == 0ULL) {
        return 1U;
    }
    --padded_index;
    if (padded_index < zero_count) {
        return 0U;
    }
    padded_index -= zero_count;
    if (padded_index == 0ULL) {
        return 1U;
    }
    --padded_index;
    return (uint8_t)((length_bits >> (63ULL - padded_index)) & 1ULL);
}
/* Function: axis_raw_padded_bit_at. Returns one padded-message bit directly from an external byte-aligned input buffer. */

static uint8_t axis_raw_padded_bit_at(
    const uint8_t* message,
    uint64_t message_bits,
    uint64_t padded_index,
    uint64_t zero_count,
    uint64_t length_bits
) {
    if (padded_index < message_bits) {
        const uint8_t value = message[padded_index >> 3];
        const uint8_t shift = (uint8_t)(7U - (padded_index & 7U));
        return (uint8_t)((value >> shift) & 1U);
    }
    padded_index -= message_bits;
    if (padded_index == 0ULL) {
        return 1U;
    }
    --padded_index;
    if (padded_index < zero_count) {
        return 0U;
    }
    padded_index -= zero_count;
    if (padded_index == 0ULL) {
        return 1U;
    }
    --padded_index;
    return (uint8_t)((length_bits >> (63ULL - padded_index)) & 1ULL);
}
/* Function: axis_pack_even_lsb_bits. Expands one byte into a 32-bit sequence for even beat positions. */

static inline uint32_t axis_pack_even_lsb_bits(uint8_t value) {
    return (uint32_t)(
        ((value >> 0) & 1U)
        | (((value >> 2) & 1U) << 1)
        | (((value >> 4) & 1U) << 2)
        | (((value >> 6) & 1U) << 3));
}
/* Function: axis_pack_odd_lsb_bits. Expands one byte into a 32-bit sequence for odd beat positions. */

static inline uint32_t axis_pack_odd_lsb_bits(uint8_t value) {
    return (uint32_t)(
        ((value >> 1) & 1U)
        | (((value >> 3) & 1U) << 1)
        | (((value >> 5) & 1U) << 2)
        | (((value >> 7) & 1U) << 3));
}

#if !AXIS_HAS_BMI2
static uint8_t axis_pack_even_lsb_table[256];
static uint8_t axis_pack_odd_lsb_table[256];
static uint8_t axis_pack_tables_ready = 0U;
/* Function: axis_init_pack_tables. Initializes lookup tables used to pack byte-aligned message bits into 32-beat sequences. */

static void axis_init_pack_tables(void) {
    uint32_t i;

    if (axis_pack_tables_ready != 0U) {
        return;
    }
    for (i = 0U; i < 256U; ++i) {
        axis_pack_even_lsb_table[i] = (uint8_t)axis_pack_even_lsb_bits((uint8_t)i);
        axis_pack_odd_lsb_table[i] = (uint8_t)axis_pack_odd_lsb_bits((uint8_t)i);
    }
    axis_pack_tables_ready = 1U;
}
#endif
/* Function: axis_pack_message64_reverse_block. Packs a 64-bit byte-aligned message block into the two 32-beat AXIS-512 sequences. */

static inline void axis_pack_message64_reverse_block(const uint8_t* block, uint32_t* m0_seq, uint32_t* m1_seq) {
#if AXIS_HAS_BMI2
    uint64_t word;

    memcpy(&word, block, sizeof(word));
    word = __builtin_bswap64(word);
    *m1_seq = (uint32_t)_pext_u64(word, 0x5555555555555555ULL);
    *m0_seq = (uint32_t)_pext_u64(word, 0xAAAAAAAAAAAAAAAAULL);
    return;
#else
    axis_init_pack_tables();
    *m1_seq =
        ((uint32_t)axis_pack_even_lsb_table[block[7U]])
        | ((uint32_t)axis_pack_even_lsb_table[block[6U]] << 4)
        | ((uint32_t)axis_pack_even_lsb_table[block[5U]] << 8)
        | ((uint32_t)axis_pack_even_lsb_table[block[4U]] << 12)
        | ((uint32_t)axis_pack_even_lsb_table[block[3U]] << 16)
        | ((uint32_t)axis_pack_even_lsb_table[block[2U]] << 20)
        | ((uint32_t)axis_pack_even_lsb_table[block[1U]] << 24)
        | ((uint32_t)axis_pack_even_lsb_table[block[0U]] << 28);
    *m0_seq =
        ((uint32_t)axis_pack_odd_lsb_table[block[7U]])
        | ((uint32_t)axis_pack_odd_lsb_table[block[6U]] << 4)
        | ((uint32_t)axis_pack_odd_lsb_table[block[5U]] << 8)
        | ((uint32_t)axis_pack_odd_lsb_table[block[4U]] << 12)
        | ((uint32_t)axis_pack_odd_lsb_table[block[3U]] << 16)
        | ((uint32_t)axis_pack_odd_lsb_table[block[2U]] << 20)
        | ((uint32_t)axis_pack_odd_lsb_table[block[1U]] << 24)
        | ((uint32_t)axis_pack_odd_lsb_table[block[0U]] << 28);
#endif
}
/* Function: axis_pack_message64_reverse. Packs up to 64 message bits ending at the selected top bit into AXIS-512 beat sequences. */

static inline void axis_pack_message64_reverse(const uint8_t* message, uint64_t top_bit, uint32_t* m0_seq, uint32_t* m1_seq) {
    const uint64_t top_byte = top_bit >> 3;

    axis_pack_message64_reverse_block(message + top_byte - 7ULL, m0_seq, m1_seq);
}

#if AXIS_HAS_AVX2
/* Function: axis_pack_message64_reverse4_avx2. Packs four 64-bit message blocks into AXIS-512 beat sequences using AVX2 operations. */
static AXIS_ALWAYS_INLINE void axis_pack_message64_reverse4_avx2(
    const uint8_t* block,
    uint32_t* m0_seq0,
    uint32_t* m1_seq0,
    uint32_t* m0_seq1,
    uint32_t* m1_seq1,
    uint32_t* m0_seq2,
    uint32_t* m1_seq2,
    uint32_t* m0_seq3,
    uint32_t* m1_seq3
) {
    uint8_t bytes[32] AXIS_ALIGN32;
    const __m256i packed = _mm256_loadu_si256((const __m256i*)(block - 24));

    _mm256_store_si256((__m256i*)bytes, packed);
    axis_pack_message64_reverse_block(bytes + 24, m0_seq0, m1_seq0);
    axis_pack_message64_reverse_block(bytes + 16, m0_seq1, m1_seq1);
    axis_pack_message64_reverse_block(bytes + 8, m0_seq2, m1_seq2);
    axis_pack_message64_reverse_block(bytes, m0_seq3, m1_seq3);
}
#endif
/* Function: axis_pack_message32_reverse_block. Packs a 32-bit byte-aligned message block into one 32-beat sequence. */

static inline uint32_t axis_pack_message32_reverse_block(const uint8_t* block) {
    uint32_t word;

    memcpy(&word, block, sizeof(word));
    return __builtin_bswap32(word);
}
/* Function: axis_pack_message48_axis768_reverse_block. Packs a byte-aligned 48-bit AXIS-768 message block into two 32-beat sequences. */

static inline void axis_pack_message48_axis768_reverse_block(const uint8_t* block, uint32_t* m0_seq, uint32_t* m1_seq) {
    const uint64_t stream =
        ((uint64_t)block[5U])
        | ((uint64_t)block[4U] << 8)
        | ((uint64_t)block[3U] << 16)
        | ((uint64_t)block[2U] << 24)
        | ((uint64_t)block[1U] << 32)
        | ((uint64_t)block[0U] << 40);
#if AXIS_HAS_BMI2
    *m0_seq = (uint32_t)_pext_u64(stream, 0xB6DB6DB6DB6DULL);
    *m1_seq = (uint32_t)_pext_u64(stream, 0x6DB6DB6DB6DBULL);
#else
    uint32_t m0 = 0U;
    uint32_t m1 = 0U;
    uint32_t r;

    for (r = 0U; r < 16U; ++r) {
        const uint32_t triplet = (uint32_t)((stream >> (3U * r)) & 7U);
        m0 |= ((triplet & 1U) != 0U ? 1U : 0U) << (2U * r);
        m0 |= ((triplet & 4U) != 0U ? 1U : 0U) << (2U * r + 1U);
        m1 |= ((triplet & 1U) != 0U ? 1U : 0U) << (2U * r);
        m1 |= ((triplet & 2U) != 0U ? 1U : 0U) << (2U * r + 1U);
    }
    *m0_seq = m0;
    *m1_seq = m1;
#endif
}
/* Function: axis_pack_message32_reverse. Packs up to 32 message bits ending at the selected top bit into one 32-beat sequence. */

static inline uint32_t axis_pack_message32_reverse(const uint8_t* message, uint64_t top_bit) {
    const uint64_t top_byte = top_bit >> 3;

    return axis_pack_message32_reverse_block(message + top_byte - 3ULL);
}
/* Function: axis_absorb_padded_message. Absorbs the fully padded message stream according to the selected AXIS instance rate rule. */

static void axis_absorb_padded_message(axis_core_t* ctx) {
    const uint64_t message_bits = ctx->total_message_bits;
    const uint64_t zero_count = (64ULL - ((message_bits + 2ULL) & 63ULL)) & 63ULL;
    const uint64_t padded_bits = message_bits + 2ULL + zero_count + AXIS_LENGTH_BITS;
    const uint64_t length_bits = axis_binary64_double_bits(message_bits);
    uint64_t offset;

    ctx->padded_message_bits = padded_bits;
    if (ctx->config->variant == AXIS_VARIANT_768) {
        const uint64_t mixed_bits = padded_bits - axis_768_one_bit_tail(padded_bits);
        uint64_t beat = 0ULL;

        for (offset = 0ULL; offset < mixed_bits; ++beat) {
            const uint64_t top_index = padded_bits - 1ULL - offset;

            if ((beat & 1ULL) == 0ULL) {
                const uint8_t bit = axis_padded_bit_at(ctx, top_index, zero_count, length_bits);
                axis_step(ctx, bit, bit);
                offset += 1ULL;
            } else {
                const uint8_t high = axis_padded_bit_at(ctx, top_index - 1ULL, zero_count, length_bits);
                const uint8_t low = axis_padded_bit_at(ctx, top_index, zero_count, length_bits);
                axis_step(ctx, high, low);
                offset += 2ULL;
            }
        }
        for (; offset < padded_bits; ++offset) {
            const uint64_t padded_index = padded_bits - 1ULL - offset;
            const uint8_t bit = axis_padded_bit_at(ctx, padded_index, zero_count, length_bits);
            axis_step(ctx, bit, bit);
        }
        return;
    }

    for (offset = 0ULL; offset < padded_bits; ++offset) {
        const uint64_t padded_index = padded_bits - 1ULL - offset;
        const uint8_t bit = axis_padded_bit_at(ctx, padded_index, zero_count, length_bits);

        if (ctx->config->variant == AXIS_VARIANT_1024) {
            axis_step(ctx, bit, bit);
        } else {
            axis_absorb_stream_bit(ctx, bit);
        }
    }
}
/* Function: axis_release_message. Releases any owned message buffer and clears ownership metadata. */

static void axis_release_message(axis_core_t* ctx) {
    if (ctx->message_owned != 0U) {
        free(ctx->message);
    }
    ctx->message = 0;
    ctx->message_capacity_bits = 0ULL;
    ctx->total_message_bits = 0ULL;
    ctx->message_owned = 0U;
}
/* Function: axis_write_digest_bit. Writes one digest bit to the external digest buffer in output bit order. */

static AXIS_MAYBE_UNUSED void axis_write_digest_bit(uint8_t* out, uint32_t bit_index, uint8_t bit) {
    const uint32_t byte_index = bit_index >> 3;
    const uint32_t bit_offset = 7U - (bit_index & 7U);

    if (bit != 0U) {
        out[byte_index] |= (uint8_t)(1U << bit_offset);
    }
}
/* Function: axis_reverse_byte. Reverses the bit order of one byte for optimized digest packing. */

static inline uint8_t axis_reverse_byte(uint32_t value) {
    value = ((value & 0x55U) << 1) | ((value >> 1) & 0x55U);
    value = ((value & 0x33U) << 2) | ((value >> 2) & 0x33U);
    return (uint8_t)(((value & 0x0FU) << 4) | ((value >> 4) & 0x0FU));
}
/* Function: axis_pack_digest512_byte. Packs four AXIS-512 h0/h1 beat pairs into one digest byte. */

static inline uint8_t axis_pack_digest512_byte(uint32_t h0_seq, uint32_t h1_seq, uint32_t group) {
    const uint32_t shift = group * 4U;
    const uint32_t h0 = (h0_seq >> shift) & 0x0FU;
    const uint32_t h1 = (h1_seq >> shift) & 0x0FU;

    return (uint8_t)(
        ((h0 & 1U) << 7)
        | ((h1 & 1U) << 6)
        | ((h0 & 2U) << 4)
        | ((h1 & 2U) << 3)
        | ((h0 & 4U) << 1)
        | (h1 & 4U)
        | ((h0 & 8U) >> 2)
        | ((h1 & 8U) >> 3));
}
/* Function: axis_pack_digest512_byte_scalar_order. Packs AXIS-512 digest bits in the scalar output order. */

static inline uint8_t axis_pack_digest512_byte_scalar_order(uint32_t h0_seq, uint32_t h1_seq, uint32_t group) {
    const uint32_t shift = group * 4U;
    const uint32_t h0 = (h0_seq >> shift) & 0x0FU;
    const uint32_t h1 = (h1_seq >> shift) & 0x0FU;

    return (uint8_t)(
        ((h0 & 8U) << 4)
        | ((h1 & 8U) << 3)
        | ((h0 & 4U) << 3)
        | ((h1 & 4U) << 2)
        | ((h0 & 2U) << 2)
        | ((h1 & 2U) << 1)
        | ((h0 & 1U) << 1)
        | (h1 & 1U));
}
/* Function: axis_write_digest512_seq32. Writes thirty-two AXIS-512 output beats into the digest buffer. */

static inline void axis_write_digest512_seq32(uint8_t* digest, uint32_t beat, uint32_t h0_seq, uint32_t h1_seq) {
    const uint32_t byte_index = (AXIS_512_RESULT_LENGTH - 64U - 2U * beat) >> 3;

    digest[byte_index + 0U] = axis_pack_digest512_byte_scalar_order(h0_seq, h1_seq, 7U);
    digest[byte_index + 1U] = axis_pack_digest512_byte_scalar_order(h0_seq, h1_seq, 6U);
    digest[byte_index + 2U] = axis_pack_digest512_byte_scalar_order(h0_seq, h1_seq, 5U);
    digest[byte_index + 3U] = axis_pack_digest512_byte_scalar_order(h0_seq, h1_seq, 4U);
    digest[byte_index + 4U] = axis_pack_digest512_byte_scalar_order(h0_seq, h1_seq, 3U);
    digest[byte_index + 5U] = axis_pack_digest512_byte_scalar_order(h0_seq, h1_seq, 2U);
    digest[byte_index + 6U] = axis_pack_digest512_byte_scalar_order(h0_seq, h1_seq, 1U);
    digest[byte_index + 7U] = axis_pack_digest512_byte_scalar_order(h0_seq, h1_seq, 0U);
}
/* Function: axis_write_digest1024_seq32_scalar_order. Writes thirty-two AXIS-1024 digest bits in scalar output order. */

static inline void axis_write_digest1024_seq32_scalar_order(uint8_t* out, uint32_t beat, uint32_t seq) {
    uint32_t k;

    for (k = 0U; k < 32U; ++k) {
        axis_write_digest_bit(out, AXIS_1024_RESULT_LENGTH - 1U - beat - k, (uint8_t)((seq >> k) & 1U));
    }
}
/* Function: axis_gen512_h0. Generates the first AXIS-512 digest bit for one digest beat. */

static AXIS_MAYBE_UNUSED uint8_t axis_gen512_h0(const axis_core_t* ctx) {
    return (uint8_t)(
        axis_get_bit(ctx->regs[6], AXIS_EXL_0)
        ^ axis_get_bit(ctx->regs[4], AXIS_EXL_0)
        ^ (axis_get_bit(ctx->regs[2], AXIS_EXL_0) & axis_get_bit(ctx->regs[0], AXIS_EXL_0)));
}
/* Function: axis_gen512_h1. Generates the second AXIS-512 digest bit for one digest beat. */

static AXIS_MAYBE_UNUSED uint8_t axis_gen512_h1(const axis_core_t* ctx) {
    return (uint8_t)(
        axis_get_bit(ctx->regs[7], AXIS_EXL_0)
        ^ axis_get_bit(ctx->regs[5], AXIS_EXL_0)
        ^ (axis_get_bit(ctx->regs[3], AXIS_EXL_0) & axis_get_bit(ctx->regs[1], AXIS_EXL_0)));
}
/* Function: axis_write_digest768_bitwise. Writes the AXIS-768 digest using the documented alternating 1-bit and 2-bit beat rule. */

static AXIS_MAYBE_UNUSED void axis_write_digest768_bitwise(axis_core_t* ctx, uint8_t* digest) {
    uint32_t bit_index = 0U;
    uint64_t beat = ctx->config->blank_rounds;

    while (bit_index < AXIS_768_RESULT_LENGTH) {
        if ((beat & 1ULL) == 0ULL) {
            axis_write_digest_bit(digest, AXIS_768_RESULT_LENGTH - 1U - bit_index, axis_gen512_h0(ctx));
            bit_index += 1U;
        } else {
            axis_write_digest_bit(digest, AXIS_768_RESULT_LENGTH - 2U - bit_index, axis_gen512_h0(ctx));
            axis_write_digest_bit(digest, AXIS_768_RESULT_LENGTH - 1U - bit_index, axis_gen512_h1(ctx));
            bit_index += 2U;
        }
        axis_reupfull(ctx, axis_reupfull_constant_byte(ctx->padded_message_bits, beat));
        ++beat;
    }
}
/* Function: axis_gen512_h0_seq32. Generates thirty-two AXIS-512 h0 digest bits from packed registers. */

static AXIS_MAYBE_UNUSED uint32_t axis_gen512_h0_seq32(const axis_core_t* ctx) {
    return (uint32_t)(
        axis_extract_seq32(ctx->regs[6], AXIS_EXL_0)
        ^ axis_extract_seq32(ctx->regs[4], AXIS_EXL_0)
        ^ (axis_extract_seq32(ctx->regs[2], AXIS_EXL_0) & axis_extract_seq32(ctx->regs[0], AXIS_EXL_0)));
}
/* Function: axis_gen512_h1_seq32. Generates thirty-two AXIS-512 h1 digest bits from packed registers. */

static AXIS_MAYBE_UNUSED uint32_t axis_gen512_h1_seq32(const axis_core_t* ctx) {
    return (uint32_t)(
        axis_extract_seq32(ctx->regs[7], AXIS_EXL_0)
        ^ axis_extract_seq32(ctx->regs[5], AXIS_EXL_0)
        ^ (axis_extract_seq32(ctx->regs[3], AXIS_EXL_0) & axis_extract_seq32(ctx->regs[1], AXIS_EXL_0)));
}

#if AXIS_HAS_AVX2
/* Function: axis_soa32_init_default. Initializes the AVX2 SoA32 state with the document initial constants. */
static AXIS_ALWAYS_INLINE void axis_soa32_init_default(axis_soa32_state_t* AXIS_RESTRICT soa) {
    const __m256i w0 = _mm256_setr_epi32(
        (int)(uint32_t)k_axis_init_words[0][0], (int)(uint32_t)k_axis_init_words[1][0],
        (int)(uint32_t)k_axis_init_words[2][0], (int)(uint32_t)k_axis_init_words[3][0],
        (int)(uint32_t)k_axis_init_words[4][0], (int)(uint32_t)k_axis_init_words[5][0],
        (int)(uint32_t)k_axis_init_words[6][0], (int)(uint32_t)k_axis_init_words[7][0]);
    const __m256i w0h = _mm256_setr_epi32(
        (int)(uint32_t)(k_axis_init_words[0][0] >> 32), (int)(uint32_t)(k_axis_init_words[1][0] >> 32),
        (int)(uint32_t)(k_axis_init_words[2][0] >> 32), (int)(uint32_t)(k_axis_init_words[3][0] >> 32),
        (int)(uint32_t)(k_axis_init_words[4][0] >> 32), (int)(uint32_t)(k_axis_init_words[5][0] >> 32),
        (int)(uint32_t)(k_axis_init_words[6][0] >> 32), (int)(uint32_t)(k_axis_init_words[7][0] >> 32));
    const __m256i w1 = _mm256_setr_epi32(
        (int)(uint32_t)k_axis_init_words[0][1], (int)(uint32_t)k_axis_init_words[1][1],
        (int)(uint32_t)k_axis_init_words[2][1], (int)(uint32_t)k_axis_init_words[3][1],
        (int)(uint32_t)k_axis_init_words[4][1], (int)(uint32_t)k_axis_init_words[5][1],
        (int)(uint32_t)k_axis_init_words[6][1], (int)(uint32_t)k_axis_init_words[7][1]);
    const __m256i w1h = _mm256_setr_epi32(
        (int)(uint32_t)(k_axis_init_words[0][1] >> 32), (int)(uint32_t)(k_axis_init_words[1][1] >> 32),
        (int)(uint32_t)(k_axis_init_words[2][1] >> 32), (int)(uint32_t)(k_axis_init_words[3][1] >> 32),
        (int)(uint32_t)(k_axis_init_words[4][1] >> 32), (int)(uint32_t)(k_axis_init_words[5][1] >> 32),
        (int)(uint32_t)(k_axis_init_words[6][1] >> 32), (int)(uint32_t)(k_axis_init_words[7][1] >> 32));
    const __m256i w2 = _mm256_setr_epi32(
        (int)(uint32_t)k_axis_init_words[0][2], (int)(uint32_t)k_axis_init_words[1][2],
        (int)(uint32_t)k_axis_init_words[2][2], (int)(uint32_t)k_axis_init_words[3][2],
        (int)(uint32_t)k_axis_init_words[4][2], (int)(uint32_t)k_axis_init_words[5][2],
        (int)(uint32_t)k_axis_init_words[6][2], (int)(uint32_t)k_axis_init_words[7][2]);
    const __m256i w2h = _mm256_setr_epi32(
        (int)(uint32_t)(k_axis_init_words[0][2] >> 32), (int)(uint32_t)(k_axis_init_words[1][2] >> 32),
        (int)(uint32_t)(k_axis_init_words[2][2] >> 32), (int)(uint32_t)(k_axis_init_words[3][2] >> 32),
        (int)(uint32_t)(k_axis_init_words[4][2] >> 32), (int)(uint32_t)(k_axis_init_words[5][2] >> 32),
        (int)(uint32_t)(k_axis_init_words[6][2] >> 32), (int)(uint32_t)(k_axis_init_words[7][2] >> 32));

    _mm256_store_si256((__m256i*)soa->w0l, w0);
    _mm256_store_si256((__m256i*)soa->w0h, w0h);
    _mm256_store_si256((__m256i*)soa->w1l, w1);
    _mm256_store_si256((__m256i*)soa->w1h, w1h);
    _mm256_store_si256((__m256i*)soa->w2l, w2);
    _mm256_store_si256((__m256i*)soa->w2h, w2h);
}
/* Function: axis_soa32_to_regs_doc. Converts the AVX2 SoA32 state back to document-order packed registers. */

static void axis_soa32_to_regs_doc(const axis_soa32_state_t* AXIS_RESTRICT soa, uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS]) {
    uint32_t i;

    for (i = 0U; i < AXIS_MAX_REGISTERS; ++i) {
        regs[i][0] = (uint64_t)soa->w0l[i] | ((uint64_t)soa->w0h[i] << 32);
        regs[i][1] = (uint64_t)soa->w1l[i] | ((uint64_t)soa->w1h[i] << 32);
        regs[i][2] = (uint64_t)soa->w2l[i] | ((uint64_t)soa->w2h[i] << 32);
    }
}
/* Function: axis_soa32_from_regs_doc. Converts document-order packed registers into the AVX2 SoA32 state. */

static void axis_soa32_from_regs_doc(axis_soa32_state_t* AXIS_RESTRICT soa, uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS]) {
    uint32_t i;

    for (i = 0U; i < AXIS_MAX_REGISTERS; ++i) {
        soa->w0l[i] = (uint32_t)regs[i][0];
        soa->w0h[i] = (uint32_t)(regs[i][0] >> 32);
        soa->w1l[i] = (uint32_t)regs[i][1];
        soa->w1h[i] = (uint32_t)(regs[i][1] >> 32);
        soa->w2l[i] = (uint32_t)regs[i][2];
        soa->w2h[i] = (uint32_t)(regs[i][2] >> 32);
    }
}
/* Function: axis_soa32_zero32_doc_step. Applies one generated 32-beat zero update directly in AVX2 SoA32 form. */

static AXIS_MAYBE_UNUSED void axis_soa32_zero32_doc_step(axis_soa32_state_t* AXIS_RESTRICT soa) {
    uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS];

    axis_soa32_to_regs_doc(soa, regs);
    axis_zero32_expr_word_step(regs);
    axis_soa32_from_regs_doc(soa, regs);
}
/* Function: axis_soa32_sbox_a1_vec. Evaluates the one-beat advanced SBox term on AVX2 vector lanes. */

static AXIS_ALWAYS_INLINE AXIS_MAYBE_UNUSED __m256i axis_soa32_sbox_a1_vec(
    __m256i w0l,
    __m256i w0h,
    __m256i w1l,
    __m256i w1h,
    __m256i w2l
) {
    const __m256i x0 = _mm256_or_si256(_mm256_srli_epi32(w0l, 1), _mm256_slli_epi32(w0h, 31));
    const __m256i x1 = _mm256_or_si256(_mm256_srli_epi32(w0l, 13), _mm256_slli_epi32(w0h, 19));
    const __m256i x2 = _mm256_or_si256(_mm256_srli_epi32(w0h, 1), _mm256_slli_epi32(w1l, 31));
    const __m256i x3 = _mm256_or_si256(_mm256_srli_epi32(w1h, 1), _mm256_slli_epi32(w2l, 31));
    const __m256i x4 = _mm256_or_si256(_mm256_srli_epi32(w1h, 12), _mm256_slli_epi32(w2l, 20));
    const __m256i x5 = _mm256_or_si256(_mm256_srli_epi32(w1h, 30), _mm256_slli_epi32(w2l, 2));

    return _mm256_xor_si256(
        _mm256_xor_si256(_mm256_and_si256(x0, x1), _mm256_and_si256(x2, x3)),
        _mm256_and_si256(x4, x5));
}
/* Function: axis_soa32_extract_seq_vec. Extracts a vectorized thirty-two-beat sequence from AVX2 register words. */

static AXIS_ALWAYS_INLINE __m256i axis_soa32_extract_seq_vec(
    __m256i w0l,
    __m256i w0h,
    __m256i w1l,
    __m256i w1h,
    __m256i w2l,
    __m256i w2h,
    uint32_t bit_index
) {
    __m256i lo;
    __m256i hi;
    uint32_t shift;

    if (bit_index < 32U) {
        lo = w0l;
        hi = w0h;
        shift = bit_index;
    } else if (bit_index < 64U) {
        lo = w0h;
        hi = w1l;
        shift = bit_index - 32U;
    } else if (bit_index < 96U) {
        lo = w1l;
        hi = w1h;
        shift = bit_index - 64U;
    } else if (bit_index < 128U) {
        lo = w1h;
        hi = w2l;
        shift = bit_index - 96U;
    } else {
        lo = w2l;
        hi = w2h;
        shift = bit_index - 128U;
    }

    return _mm256_or_si256(
        _mm256_srlv_epi32(lo, _mm256_set1_epi32((int)shift)),
        _mm256_sllv_epi32(hi, _mm256_set1_epi32((int)(32U - shift))));
}
/* Function: axis_soa32_step_vecs_block32_common. Executes the common AVX2 32-beat UpFull kernel for supplied message vectors. */

static AXIS_ALWAYS_INLINE void axis_soa32_step_vecs_block32_common(
    __m256i* AXIS_RESTRICT w0l_io,
    __m256i* AXIS_RESTRICT w0h_io,
    __m256i* AXIS_RESTRICT w1l_io,
    __m256i* AXIS_RESTRICT w1h_io,
    __m256i* AXIS_RESTRICT w2l_io,
    __m256i* AXIS_RESTRICT w2h_io,
    const __m256i m
) {
    const __m256i w0l = *w0l_io;
    const __m256i w0h = *w0h_io;
    const __m256i w1l = *w1l_io;
    const __m256i w1h = *w1h_io;
    const __m256i w2l = *w2l_io;
    const __m256i w2h = *w2h_io;
    const __m256i e0 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_0);
    const __m256i e1 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_1);
    const __m256i e2 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_2);
    const __m256i e3 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_3);
    const __m256i e4 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_4);
    const __m256i e5 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_5);
    const __m256i e0a1 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_0 + 1U);
    const __m256i e1a1 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_1 + 1U);
    const __m256i e2a1 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_2 + 1U);
    const __m256i e3a1 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_3 + 1U);
    const __m256i e4a1 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_4 + 1U);
    const __m256i e5a1 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_5 + 1U);
    const __m256i prev0 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_PREV);
    const __m256i prev1 = axis_soa32_extract_seq_vec(w0l, w0h, w1l, w1h, w2l, w2h, AXIS_EXL_PREV + 1U);
    const __m256i perm_next1 = _mm256_setr_epi32(1, 2, 3, 4, 5, 6, 7, 0);
    const __m256i perm_next3 = _mm256_setr_epi32(3, 4, 5, 6, 7, 0, 1, 2);
    const __m256i perm_next5 = _mm256_setr_epi32(5, 6, 7, 0, 1, 2, 3, 4);
    const __m256i perm_prev = _mm256_setr_epi32(7, 0, 1, 2, 3, 4, 5, 6);
    __m256i n5e3;
    __m256i n5e2;
    __m256i n3e5;
    __m256i n3e1;
    __m256i n1e4;
    __m256i n1e0;
    __m256i sbox_current;
    __m256i sbox_new;
    __m256i prev;
    const __m256i t0v = _mm256_xor_si256(_mm256_xor_si256(e0, e1), e2);
    const __m256i t1v = _mm256_xor_si256(_mm256_xor_si256(e3, e4), e5);
    __m256i bvec;
    __m256i t0;
    __m256i t1;

    n5e3 = _mm256_blend_epi32(
        _mm256_permutevar8x32_epi32(e3, perm_next5),
        _mm256_permutevar8x32_epi32(e3a1, perm_next5),
        0xF8);
    n5e2 = _mm256_blend_epi32(
        _mm256_permutevar8x32_epi32(e2, perm_next5),
        _mm256_permutevar8x32_epi32(e2a1, perm_next5),
        0xF8);
    n3e5 = _mm256_blend_epi32(
        _mm256_permutevar8x32_epi32(e5, perm_next3),
        _mm256_permutevar8x32_epi32(e5a1, perm_next3),
        0xE0);
    n3e1 = _mm256_blend_epi32(
        _mm256_permutevar8x32_epi32(e1, perm_next3),
        _mm256_permutevar8x32_epi32(e1a1, perm_next3),
        0xE0);
    n1e4 = _mm256_blend_epi32(
        _mm256_permutevar8x32_epi32(e4, perm_next1),
        _mm256_permutevar8x32_epi32(e4a1, perm_next1),
        0x80);
    n1e0 = _mm256_blend_epi32(
        _mm256_permutevar8x32_epi32(e0, perm_next1),
        _mm256_permutevar8x32_epi32(e0a1, perm_next1),
        0x80);
    sbox_current = _mm256_xor_si256(
        _mm256_xor_si256(_mm256_and_si256(e5, e2), _mm256_and_si256(e4, e1)),
        _mm256_and_si256(e3, e0));
    sbox_new = _mm256_xor_si256(
        _mm256_xor_si256(_mm256_and_si256(n5e3, n5e2), _mm256_and_si256(n3e5, n3e1)),
        _mm256_and_si256(n1e4, n1e0));
    prev = _mm256_permutevar8x32_epi32(prev0, perm_prev);
    prev = _mm256_blend_epi32(
        prev,
        _mm256_permutevar8x32_epi32(prev1, perm_prev),
        0xFE);
    bvec = _mm256_xor_si256(
        _mm256_xor_si256(m, sbox_current),
        _mm256_xor_si256(sbox_new, prev));
    t0 = _mm256_xor_si256(t0v, bvec);
    t1 = _mm256_xor_si256(t1v, bvec);

    *w0l_io = w0h;
    *w0h_io = _mm256_xor_si256(
        w1l,
        _mm256_xor_si256(
            _mm256_xor_si256(_mm256_slli_epi32(t1, 26), _mm256_slli_epi32(t1, 11)),
            _mm256_slli_epi32(t1, 2)));
    *w1l_io = _mm256_xor_si256(
        w1h,
        _mm256_xor_si256(
            _mm256_xor_si256(_mm256_srli_epi32(t1, 6), _mm256_srli_epi32(t1, 21)),
            _mm256_srli_epi32(t1, 30)));
    *w1h_io = w2l;
    *w2l_io = _mm256_xor_si256(
        w2h,
        _mm256_xor_si256(
            _mm256_xor_si256(_mm256_slli_epi32(t0, 28), _mm256_slli_epi32(t0, 18)),
            _mm256_slli_epi32(t0, 2)));
    *w2h_io = _mm256_xor_si256(
        w0l,
        _mm256_xor_si256(
            _mm256_xor_si256(_mm256_srli_epi32(t0, 4), _mm256_srli_epi32(t0, 14)),
            _mm256_srli_epi32(t0, 30)));
}
/* Function: axis_soa32_step_vecs_block32. Executes the AVX2 32-beat UpFull kernel for alternating AXIS-512 style message bits. */

static AXIS_ALWAYS_INLINE void axis_soa32_step_vecs_block32(
    __m256i* AXIS_RESTRICT w0l_io,
    __m256i* AXIS_RESTRICT w0h_io,
    __m256i* AXIS_RESTRICT w1l_io,
    __m256i* AXIS_RESTRICT w1h_io,
    __m256i* AXIS_RESTRICT w2l_io,
    __m256i* AXIS_RESTRICT w2h_io,
    uint32_t m0_seq,
    uint32_t m1_seq
) {
    const __m256i m = _mm256_setr_epi32((int)m0_seq, (int)m1_seq, (int)m0_seq, (int)m1_seq, (int)m0_seq, (int)m1_seq, (int)m0_seq, (int)m1_seq);

    axis_soa32_step_vecs_block32_common(w0l_io, w0h_io, w1l_io, w1h_io, w2l_io, w2h_io, m);
}
/* Function: axis_soa32_step_vecs_block32_same. Executes the AVX2 32-beat UpFull kernel when both message inputs are identical. */

static AXIS_ALWAYS_INLINE void axis_soa32_step_vecs_block32_same(
    __m256i* AXIS_RESTRICT w0l_io,
    __m256i* AXIS_RESTRICT w0h_io,
    __m256i* AXIS_RESTRICT w1l_io,
    __m256i* AXIS_RESTRICT w1h_io,
    __m256i* AXIS_RESTRICT w2l_io,
    __m256i* AXIS_RESTRICT w2h_io,
    uint32_t seq
) {
    const __m256i m = _mm256_set1_epi32((int)seq);

    axis_soa32_step_vecs_block32_common(w0l_io, w0h_io, w1l_io, w1h_io, w2l_io, w2h_io, m);
}
/* Function: axis_soa32_step_vecs_block32_register_seqs. Executes the AVX2 32-beat UpFull kernel with per-register message sequences. */

static AXIS_ALWAYS_INLINE void axis_soa32_step_vecs_block32_register_seqs(
    __m256i* AXIS_RESTRICT w0l_io,
    __m256i* AXIS_RESTRICT w0h_io,
    __m256i* AXIS_RESTRICT w1l_io,
    __m256i* AXIS_RESTRICT w1h_io,
    __m256i* AXIS_RESTRICT w2l_io,
    __m256i* AXIS_RESTRICT w2h_io,
    const uint32_t message_seq[AXIS_MAX_REGISTERS]
) {
    const __m256i m = _mm256_setr_epi32(
        (int)message_seq[0], (int)message_seq[1], (int)message_seq[2], (int)message_seq[3],
        (int)message_seq[4], (int)message_seq[5], (int)message_seq[6], (int)message_seq[7]);

    axis_soa32_step_vecs_block32_common(w0l_io, w0h_io, w1l_io, w1h_io, w2l_io, w2h_io, m);
}
/* Function: axis_soa32_step_default_block32. Applies one 32-beat AVX2 SoA32 update block with alternating message sequences. */

static AXIS_ALWAYS_INLINE void axis_soa32_step_default_block32(axis_soa32_state_t* AXIS_RESTRICT soa, uint32_t m0_seq, uint32_t m1_seq) {
    __m256i w0l = _mm256_load_si256((const __m256i*)soa->w0l);
    __m256i w0h = _mm256_load_si256((const __m256i*)soa->w0h);
    __m256i w1l = _mm256_load_si256((const __m256i*)soa->w1l);
    __m256i w1h = _mm256_load_si256((const __m256i*)soa->w1h);
    __m256i w2l = _mm256_load_si256((const __m256i*)soa->w2l);
    __m256i w2h = _mm256_load_si256((const __m256i*)soa->w2h);

    axis_soa32_step_vecs_block32(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, m0_seq, m1_seq);

    _mm256_store_si256((__m256i*)soa->w0l, w0l);
    _mm256_store_si256((__m256i*)soa->w0h, w0h);
    _mm256_store_si256((__m256i*)soa->w1l, w1l);
    _mm256_store_si256((__m256i*)soa->w1h, w1h);
    _mm256_store_si256((__m256i*)soa->w2l, w2l);
    _mm256_store_si256((__m256i*)soa->w2h, w2h);
}
/* Function: axis_soa32_step_default_block32_register_seqs. Applies one 32-beat AVX2 SoA32 update block with per-register message sequences. */

static AXIS_ALWAYS_INLINE void axis_soa32_step_default_block32_register_seqs(axis_soa32_state_t* AXIS_RESTRICT soa, const uint32_t message_seq[AXIS_MAX_REGISTERS]) {
    __m256i w0l = _mm256_load_si256((const __m256i*)soa->w0l);
    __m256i w0h = _mm256_load_si256((const __m256i*)soa->w0h);
    __m256i w1l = _mm256_load_si256((const __m256i*)soa->w1l);
    __m256i w1h = _mm256_load_si256((const __m256i*)soa->w1h);
    __m256i w2l = _mm256_load_si256((const __m256i*)soa->w2l);
    __m256i w2h = _mm256_load_si256((const __m256i*)soa->w2h);

    axis_soa32_step_vecs_block32_register_seqs(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, message_seq);

    _mm256_store_si256((__m256i*)soa->w0l, w0l);
    _mm256_store_si256((__m256i*)soa->w0h, w0h);
    _mm256_store_si256((__m256i*)soa->w1l, w1l);
    _mm256_store_si256((__m256i*)soa->w1h, w1h);
    _mm256_store_si256((__m256i*)soa->w2l, w2l);
    _mm256_store_si256((__m256i*)soa->w2h, w2h);
}
/* Function: axis_soa32_extract_w0_seq32. Extracts thirty-two digest-related bits from the low AVX2 SoA32 word for one register. */

static AXIS_ALWAYS_INLINE uint32_t axis_soa32_extract_w0_seq32(const axis_soa32_state_t* AXIS_RESTRICT soa, uint32_t reg_index, uint32_t start_bit) {
    if (start_bit < 32U) {
        return (soa->w0l[reg_index] >> start_bit) | (soa->w0h[reg_index] << (32U - start_bit));
    }
    return soa->w0h[reg_index] >> (start_bit - 32U);
}
/* Function: axis_soa32_gen512_h0_seq32. Generates thirty-two AXIS-512 h0 digest bits from AVX2 SoA32 state. */

static inline uint32_t axis_soa32_gen512_h0_seq32(const axis_soa32_state_t* AXIS_RESTRICT soa) {
    const uint32_t r6 = axis_soa32_extract_w0_seq32(soa, 6U, AXIS_EXL_0);
    const uint32_t r4 = axis_soa32_extract_w0_seq32(soa, 4U, AXIS_EXL_0);
    const uint32_t r2 = axis_soa32_extract_w0_seq32(soa, 2U, AXIS_EXL_0);
    const uint32_t r0 = axis_soa32_extract_w0_seq32(soa, 0U, AXIS_EXL_0);

    return r6 ^ r4 ^ (r2 & r0);
}
/* Function: axis_soa32_gen512_h0_seq32_vec. Generates AXIS-512 h0 digest bits from AVX2 vector registers without storing state. */

static AXIS_ALWAYS_INLINE uint32_t axis_soa32_gen512_h0_seq32_vec(__m256i w0l, __m256i w0h) {
    axis_soa32_state_t soa;

    _mm256_store_si256((__m256i*)soa.w0l, w0l);
    _mm256_store_si256((__m256i*)soa.w0h, w0h);
    return axis_soa32_gen512_h0_seq32(&soa);
}
/* Function: axis_soa32_gen512_h1_seq32. Generates thirty-two AXIS-512 h1 digest bits from AVX2 SoA32 state. */

static inline uint32_t axis_soa32_gen512_h1_seq32(const axis_soa32_state_t* AXIS_RESTRICT soa) {
    const uint32_t r7 = axis_soa32_extract_w0_seq32(soa, 7U, AXIS_EXL_0);
    const uint32_t r5 = axis_soa32_extract_w0_seq32(soa, 5U, AXIS_EXL_0);
    const uint32_t r3 = axis_soa32_extract_w0_seq32(soa, 3U, AXIS_EXL_0);
    const uint32_t r1 = axis_soa32_extract_w0_seq32(soa, 1U, AXIS_EXL_0);

    return r7 ^ r5 ^ (r3 & r1);
}
/* Function: axis_soa32_gen512_h1_seq32_vec. Generates AXIS-512 h1 digest bits from AVX2 vector registers without storing state. */

static AXIS_ALWAYS_INLINE uint32_t axis_soa32_gen512_h1_seq32_vec(__m256i w0l, __m256i w0h) {
    axis_soa32_state_t soa;

    _mm256_store_si256((__m256i*)soa.w0l, w0l);
    _mm256_store_si256((__m256i*)soa.w0h, w0h);
    return axis_soa32_gen512_h1_seq32(&soa);
}
#endif
/* Function: axis_soa_gen512_h0_seq32. Generates thirty-two AXIS-512 h0 digest bits from scalar SoA state. */

static inline uint32_t axis_soa_gen512_h0_seq32(const axis_soa_state_t* AXIS_RESTRICT soa) {
    return (uint32_t)(
        ((uint32_t)(soa->w0[6] >> AXIS_EXL_0))
        ^ ((uint32_t)(soa->w0[4] >> AXIS_EXL_0))
        ^ (((uint32_t)(soa->w0[2] >> AXIS_EXL_0)) & ((uint32_t)(soa->w0[0] >> AXIS_EXL_0))));
}
/* Function: axis_soa_gen512_h1_seq32. Generates thirty-two AXIS-512 h1 digest bits from scalar SoA state. */

static inline uint32_t axis_soa_gen512_h1_seq32(const axis_soa_state_t* AXIS_RESTRICT soa) {
    return (uint32_t)(
        ((uint32_t)(soa->w0[7] >> AXIS_EXL_0))
        ^ ((uint32_t)(soa->w0[5] >> AXIS_EXL_0))
        ^ (((uint32_t)(soa->w0[3] >> AXIS_EXL_0)) & ((uint32_t)(soa->w0[1] >> AXIS_EXL_0))));
}
/* Function: axis_soa_init_default. Initializes the scalar SoA state with the document initial constants. */

static AXIS_ALWAYS_INLINE void axis_soa_init_default(axis_soa_state_t* AXIS_RESTRICT soa) {
    uint32_t i;

    for (i = 0U; i < AXIS_MAX_REGISTERS; ++i) {
        soa->w0[i] = k_axis_init_words[i][0];
        soa->w1[i] = k_axis_init_words[i][1];
        soa->w2[i] = k_axis_init_words[i][2];
    }
}

#if AXIS_HAS_AVX2
#define axis_fast_soa_state_t axis_soa32_state_t
#define axis_fast_soa_init_default axis_soa32_init_default
#define axis_fast_soa_step_default_block32 axis_soa32_step_default_block32
#define axis_fast_soa_step_default_block32_register_seqs axis_soa32_step_default_block32_register_seqs
#define axis_fast_soa_gen512_h0_seq32 axis_soa32_gen512_h0_seq32
#define axis_fast_soa_gen512_h1_seq32 axis_soa32_gen512_h1_seq32
#else
#define axis_fast_soa_state_t axis_soa_state_t
#define axis_fast_soa_init_default axis_soa_init_default
#define axis_fast_soa_step_default_block32 axis_soa_step_default_block32
#define axis_fast_soa_step_default_block32_register_seqs axis_soa_step_default_block32_register_seqs
#define axis_fast_soa_gen512_h0_seq32 axis_soa_gen512_h0_seq32
#define axis_fast_soa_gen512_h1_seq32 axis_soa_gen512_h1_seq32
#endif
/* Function: axis_soa_hash512_byte_aligned. Computes AXIS-512 for byte-aligned input using the portable SoA fast path. */

static AXIS_MAYBE_UNUSED void axis_soa_hash512_byte_aligned(const uint8_t* message, uint64_t message_bits, uint8_t* digest) {
    axis_fast_soa_state_t soa;
    const uint64_t zero_count = (64ULL - ((message_bits + 2ULL) & 63ULL)) & 63ULL;
    const uint64_t padded_bits = message_bits + 2ULL + zero_count + AXIS_LENGTH_BITS;
    const uint64_t length_bits = axis_binary64_double_bits((uint64_t)message_bits);
    uint64_t beat;
    uint64_t offset;

    axis_fast_soa_init_default(&soa);
    for (offset = 0ULL; offset + 64ULL <= padded_bits; offset += 64ULL) {
        const uint64_t top_index = padded_bits - 1ULL - offset;
        uint32_t m0_seq = 0U;
        uint32_t m1_seq = 0U;
        uint32_t k;

        if (top_index < message_bits) {
            break;
        }
        for (k = 0U; k < 32U; ++k) {
            const uint64_t low_index = top_index - (uint64_t)(2U * k);
            const uint64_t high_index = low_index - 1ULL;
            m1_seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, low_index, zero_count, length_bits) << k;
            m0_seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, high_index, zero_count, length_bits) << k;
        }
        axis_fast_soa_step_default_block32(&soa, m0_seq, m1_seq);
    }
    if (offset + 64ULL <= padded_bits) {
        const uint64_t top_index = padded_bits - 1ULL - offset;
        uint64_t pure_blocks = (top_index + 1ULL) >> 6;

        if (pure_blocks != 0ULL) {
            const uint64_t consumed_bits = pure_blocks << 6;
            const uint8_t* block = message + (top_index >> 3) - 7ULL;
#if AXIS_HAS_AVX2
            __m256i w0l = _mm256_load_si256((const __m256i*)soa.w0l);
            __m256i w0h = _mm256_load_si256((const __m256i*)soa.w0h);
            __m256i w1l = _mm256_load_si256((const __m256i*)soa.w1l);
            __m256i w1h = _mm256_load_si256((const __m256i*)soa.w1h);
            __m256i w2l = _mm256_load_si256((const __m256i*)soa.w2l);
            __m256i w2h = _mm256_load_si256((const __m256i*)soa.w2h);

            while (pure_blocks >= 4ULL) {
                uint32_t m0_seq0;
                uint32_t m1_seq0;
                uint32_t m0_seq1;
                uint32_t m1_seq1;
                uint32_t m0_seq2;
                uint32_t m1_seq2;
                uint32_t m0_seq3;
                uint32_t m1_seq3;

                axis_pack_message64_reverse4_avx2(
                    block,
                    &m0_seq0,
                    &m1_seq0,
                    &m0_seq1,
                    &m1_seq1,
                    &m0_seq2,
                    &m1_seq2,
                    &m0_seq3,
                    &m1_seq3);
                axis_soa32_step_vecs_block32(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, m0_seq0, m1_seq0);
                axis_soa32_step_vecs_block32(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, m0_seq1, m1_seq1);
                axis_soa32_step_vecs_block32(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, m0_seq2, m1_seq2);
                axis_soa32_step_vecs_block32(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, m0_seq3, m1_seq3);
                block -= 32;
                pure_blocks -= 4ULL;
            }
            if (pure_blocks != 0ULL) {
                do {
                    uint32_t m0_seq;
                    uint32_t m1_seq;

                    axis_pack_message64_reverse_block(block, &m0_seq, &m1_seq);
                    axis_soa32_step_vecs_block32(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, m0_seq, m1_seq);
                    block -= 8;
                    --pure_blocks;
                } while (pure_blocks != 0ULL);
            }
            _mm256_store_si256((__m256i*)soa.w0l, w0l);
            _mm256_store_si256((__m256i*)soa.w0h, w0h);
            _mm256_store_si256((__m256i*)soa.w1l, w1l);
            _mm256_store_si256((__m256i*)soa.w1h, w1h);
            _mm256_store_si256((__m256i*)soa.w2l, w2l);
            _mm256_store_si256((__m256i*)soa.w2h, w2h);
#else
            do {
                uint32_t m0_seq;
                uint32_t m1_seq;

                axis_pack_message64_reverse_block(block, &m0_seq, &m1_seq);
                axis_fast_soa_step_default_block32(&soa, m0_seq, m1_seq);
                block -= 8;
                --pure_blocks;
            } while (pure_blocks != 0ULL);
#endif
            offset += consumed_bits;
        }
    }
    for (; offset + 64ULL <= padded_bits; offset += 64ULL) {
        const uint64_t top_index = padded_bits - 1ULL - offset;
        uint32_t m0_seq = 0U;
        uint32_t m1_seq = 0U;
        uint32_t k;

        for (k = 0U; k < 32U; ++k) {
            const uint64_t low_index = top_index - (uint64_t)(2U * k);
            const uint64_t high_index = low_index - 1ULL;
            m1_seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, low_index, zero_count, length_bits) << k;
            m0_seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, high_index, zero_count, length_bits) << k;
        }
        axis_fast_soa_step_default_block32(&soa, m0_seq, m1_seq);
    }

    for (beat = 0ULL; beat < AXIS512_BLANK_ROUNDS; beat += 32ULL) {
        uint32_t constant_seq[AXIS_MAX_REGISTERS];

        axis_reupfull_constant_seq32(padded_bits, beat, constant_seq);
        axis_fast_soa_step_default_block32_register_seqs(&soa, constant_seq);
    }

    for (beat = 0ULL; beat < 256ULL; beat += 32ULL) {
        uint32_t constant_seq[AXIS_MAX_REGISTERS];
        const uint32_t h0_seq = axis_fast_soa_gen512_h0_seq32(&soa);
        const uint32_t h1_seq = axis_fast_soa_gen512_h1_seq32(&soa);

        axis_write_digest512_seq32(digest, (uint32_t)beat, h0_seq, h1_seq);
        axis_reupfull_constant_seq32(padded_bits, AXIS512_BLANK_ROUNDS + beat, constant_seq);
        axis_fast_soa_step_default_block32_register_seqs(&soa, constant_seq);
    }
}
/* Function: axis_write_digest768_seq32. Appends AXIS-768 digest bits from a thirty-two-beat h0/h1 sequence. */

static AXIS_MAYBE_UNUSED void axis_write_digest768_seq32(uint8_t* digest, uint32_t* bit_index, uint32_t h0_seq, uint32_t h1_seq) {
    uint32_t k;

    for (k = 0U; k < 32U; ++k) {
        if ((k & 1U) == 0U) {
            axis_write_digest_bit(digest, AXIS_768_RESULT_LENGTH - 1U - *bit_index, (uint8_t)((h0_seq >> k) & 1U));
            *bit_index += 1U;
        } else {
            axis_write_digest_bit(digest, AXIS_768_RESULT_LENGTH - 2U - *bit_index, (uint8_t)((h0_seq >> k) & 1U));
            axis_write_digest_bit(digest, AXIS_768_RESULT_LENGTH - 1U - *bit_index, (uint8_t)((h1_seq >> k) & 1U));
            *bit_index += 2U;
        }
    }
}
/* Function: axis_soa_hash768_byte_aligned. Computes AXIS-768 for byte-aligned input using the SoA fast path. */

static AXIS_MAYBE_UNUSED void axis_soa_hash768_byte_aligned(const uint8_t* message, uint64_t message_bits, uint8_t* digest) {
#if AXIS_HAS_AVX2
    axis_soa32_state_t soa;
#else
    axis_soa_state_t soa;
#endif
    const uint64_t zero_count = (64ULL - ((message_bits + 2ULL) & 63ULL)) & 63ULL;
    const uint64_t padded_bits = message_bits + 2ULL + zero_count + AXIS_LENGTH_BITS;
    const uint64_t mixed_bits = padded_bits - axis_768_one_bit_tail(padded_bits);
    const uint64_t length_bits = axis_binary64_double_bits((uint64_t)message_bits);
    uint64_t offset;
    uint64_t beat;
    uint32_t digest_bit_index = 0U;

    memset(digest, 0, AXIS_768_RESULT_LENGTH / 8U);
#if AXIS_HAS_AVX2
    axis_soa32_init_default(&soa);
    {
        __m256i w0l = _mm256_load_si256((const __m256i*)soa.w0l);
        __m256i w0h = _mm256_load_si256((const __m256i*)soa.w0h);
        __m256i w1l = _mm256_load_si256((const __m256i*)soa.w1l);
        __m256i w1h = _mm256_load_si256((const __m256i*)soa.w1h);
        __m256i w2l = _mm256_load_si256((const __m256i*)soa.w2l);
        __m256i w2h = _mm256_load_si256((const __m256i*)soa.w2h);

        for (offset = 0ULL; offset + 48ULL <= mixed_bits; offset += 48ULL) {
            const uint64_t current_top_index = padded_bits - 1ULL - offset;
            uint64_t block_offset = offset;
            uint32_t m0_seq = 0U;
            uint32_t m1_seq = 0U;
            uint32_t k;

            if (current_top_index < message_bits) {
                break;
            }
            for (k = 0U; k < 32U; ++k) {
                const uint64_t top_index = padded_bits - 1ULL - block_offset;

                if ((k & 1U) == 0U) {
                    const uint32_t bit = axis_raw_padded_bit_at(message, message_bits, top_index, zero_count, length_bits);
                    m0_seq |= bit << k;
                    m1_seq |= bit << k;
                    block_offset += 1ULL;
                } else {
                    m0_seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, top_index - 1ULL, zero_count, length_bits) << k;
                    m1_seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, top_index, zero_count, length_bits) << k;
                    block_offset += 2ULL;
                }
            }
            axis_soa32_step_vecs_block32(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, m0_seq, m1_seq);
        }
        for (; offset + 48ULL <= mixed_bits; offset += 48ULL) {
            const uint64_t top_index = padded_bits - 1ULL - offset;
            const uint8_t* block = message + (top_index >> 3) - 5ULL;
            uint32_t m0_seq;
            uint32_t m1_seq;

            axis_pack_message48_axis768_reverse_block(block, &m0_seq, &m1_seq);
            axis_soa32_step_vecs_block32(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, m0_seq, m1_seq);
        }
        _mm256_store_si256((__m256i*)soa.w0l, w0l);
        _mm256_store_si256((__m256i*)soa.w0h, w0h);
        _mm256_store_si256((__m256i*)soa.w1l, w1l);
        _mm256_store_si256((__m256i*)soa.w1h, w1h);
        _mm256_store_si256((__m256i*)soa.w2l, w2l);
        _mm256_store_si256((__m256i*)soa.w2h, w2h);
    }
#else
    axis_soa_init_default(&soa);
    for (offset = 0ULL; offset + 48ULL <= mixed_bits; offset += 48ULL) {
        const uint64_t current_top_index = padded_bits - 1ULL - offset;
        uint64_t block_offset = offset;
        uint32_t m0_seq = 0U;
        uint32_t m1_seq = 0U;
        uint32_t k;

        if (current_top_index < message_bits) {
            break;
        }
        for (k = 0U; k < 32U; ++k) {
            const uint64_t top_index = padded_bits - 1ULL - block_offset;

            if ((k & 1U) == 0U) {
                const uint32_t bit = axis_raw_padded_bit_at(message, message_bits, top_index, zero_count, length_bits);
                m0_seq |= bit << k;
                m1_seq |= bit << k;
                block_offset += 1ULL;
            } else {
                m0_seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, top_index - 1ULL, zero_count, length_bits) << k;
                m1_seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, top_index, zero_count, length_bits) << k;
                block_offset += 2ULL;
            }
        }
        axis_soa_step_default_block32(&soa, m0_seq, m1_seq);
    }
    for (; offset + 48ULL <= mixed_bits; offset += 48ULL) {
        const uint64_t top_index = padded_bits - 1ULL - offset;
        const uint8_t* block = message + (top_index >> 3) - 5ULL;
        uint32_t m0_seq;
        uint32_t m1_seq;

        axis_pack_message48_axis768_reverse_block(block, &m0_seq, &m1_seq);
        axis_soa_step_default_block32(&soa, m0_seq, m1_seq);
    }
#endif
    for (; offset + 32ULL <= padded_bits; offset += 32ULL) {
        const uint64_t top_index = padded_bits - 1ULL - offset;
        uint32_t seq = 0U;
        uint32_t k;

        for (k = 0U; k < 32U; ++k) {
            seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, top_index - k, zero_count, length_bits) << k;
        }
        axis_fast_soa_step_default_block32(&soa, seq, seq);
    }

    for (beat = 0ULL; beat < AXIS768_BLANK_ROUNDS; beat += 32ULL) {
        uint32_t constant_seq[AXIS_MAX_REGISTERS];

        axis_reupfull_constant_seq32(padded_bits, beat, constant_seq);
        axis_fast_soa_step_default_block32_register_seqs(&soa, constant_seq);
    }

    for (beat = 0ULL; beat < 512ULL; beat += 32ULL) {
        uint32_t constant_seq[AXIS_MAX_REGISTERS];
        const uint32_t h0_seq = axis_fast_soa_gen512_h0_seq32(&soa);
        const uint32_t h1_seq = axis_fast_soa_gen512_h1_seq32(&soa);

        axis_write_digest768_seq32(digest, &digest_bit_index, h0_seq, h1_seq);
        axis_reupfull_constant_seq32(padded_bits, AXIS768_BLANK_ROUNDS + beat, constant_seq);
        axis_fast_soa_step_default_block32_register_seqs(&soa, constant_seq);
    }
}
/* Function: axis_soa_hash1024_byte_aligned. Computes AXIS-1024 for byte-aligned input using the SoA fast path. */

static AXIS_MAYBE_UNUSED void axis_soa_hash1024_byte_aligned(const uint8_t* message, uint64_t message_bits, uint8_t* digest) {
#if AXIS_HAS_AVX2
    axis_soa32_state_t soa;
#else
    axis_soa_state_t soa;
#endif
    const uint64_t zero_count = (64ULL - ((message_bits + 2ULL) & 63ULL)) & 63ULL;
    const uint64_t padded_bits = message_bits + 2ULL + zero_count + AXIS_LENGTH_BITS;
    const uint64_t length_bits = axis_binary64_double_bits((uint64_t)message_bits);
    uint64_t offset;

    memset(digest, 0, AXIS_1024_RESULT_LENGTH / 8U);
#if AXIS_HAS_AVX2
    axis_soa32_init_default(&soa);
#else
    axis_soa_init_default(&soa);
#endif
    for (offset = 0ULL; offset + 32ULL <= padded_bits; offset += 32ULL) {
        const uint64_t top_index = padded_bits - 1ULL - offset;
        uint32_t seq = 0U;
        uint32_t k;

        if (top_index < message_bits) {
            break;
        }
        for (k = 0U; k < 32U; ++k) {
            const uint64_t padded_index = top_index - k;
            seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, padded_index, zero_count, length_bits) << k;
        }
#if AXIS_HAS_AVX2
        axis_soa32_step_default_block32(&soa, seq, seq);
#else
        axis_soa_step_default_block32(&soa, seq, seq);
#endif
    }
    if (offset + 32ULL <= padded_bits) {
        const uint64_t top_index = padded_bits - 1ULL - offset;
        uint64_t pure_blocks = (top_index + 1ULL) >> 5;

        if (pure_blocks != 0ULL) {
            const uint64_t consumed_bits = pure_blocks << 5;
            const uint8_t* block = message + (top_index >> 3) - 3ULL;
#if AXIS_HAS_AVX2
            __m256i w0l = _mm256_load_si256((const __m256i*)soa.w0l);
            __m256i w0h = _mm256_load_si256((const __m256i*)soa.w0h);
            __m256i w1l = _mm256_load_si256((const __m256i*)soa.w1l);
            __m256i w1h = _mm256_load_si256((const __m256i*)soa.w1h);
            __m256i w2l = _mm256_load_si256((const __m256i*)soa.w2l);
            __m256i w2h = _mm256_load_si256((const __m256i*)soa.w2h);

            while (pure_blocks >= 4ULL) {
                const uint32_t seq0 = axis_pack_message32_reverse_block(block);
                const uint32_t seq1 = axis_pack_message32_reverse_block(block - 4);
                const uint32_t seq2 = axis_pack_message32_reverse_block(block - 8);
                const uint32_t seq3 = axis_pack_message32_reverse_block(block - 12);

                axis_soa32_step_vecs_block32_same(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, seq0);
                axis_soa32_step_vecs_block32_same(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, seq1);
                axis_soa32_step_vecs_block32_same(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, seq2);
                axis_soa32_step_vecs_block32_same(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, seq3);
                block -= 16;
                pure_blocks -= 4ULL;
            }
            if (pure_blocks != 0ULL) {
                do {
                    const uint32_t seq = axis_pack_message32_reverse_block(block);

                    axis_soa32_step_vecs_block32_same(&w0l, &w0h, &w1l, &w1h, &w2l, &w2h, seq);
                    block -= 4;
                    --pure_blocks;
                } while (pure_blocks != 0ULL);
            }
            _mm256_store_si256((__m256i*)soa.w0l, w0l);
            _mm256_store_si256((__m256i*)soa.w0h, w0h);
            _mm256_store_si256((__m256i*)soa.w1l, w1l);
            _mm256_store_si256((__m256i*)soa.w1h, w1h);
            _mm256_store_si256((__m256i*)soa.w2l, w2l);
            _mm256_store_si256((__m256i*)soa.w2h, w2h);
#else
            while (pure_blocks >= 4ULL) {
                const uint32_t seq0 = axis_pack_message32_reverse_block(block);
                const uint32_t seq1 = axis_pack_message32_reverse_block(block - 4);
                const uint32_t seq2 = axis_pack_message32_reverse_block(block - 8);
                const uint32_t seq3 = axis_pack_message32_reverse_block(block - 12);

                axis_fast_soa_step_default_block32(&soa, seq0, seq0);
                axis_fast_soa_step_default_block32(&soa, seq1, seq1);
                axis_fast_soa_step_default_block32(&soa, seq2, seq2);
                axis_fast_soa_step_default_block32(&soa, seq3, seq3);
                block -= 16;
                pure_blocks -= 4ULL;
            }
            if (pure_blocks != 0ULL) {
                do {
                    const uint32_t seq = axis_pack_message32_reverse_block(block);

                    axis_fast_soa_step_default_block32(&soa, seq, seq);
                    block -= 4;
                    --pure_blocks;
                } while (pure_blocks != 0ULL);
            }
#endif
            offset += consumed_bits;
        }
    }
    for (; offset + 32ULL <= padded_bits; offset += 32ULL) {
        const uint64_t top_index = padded_bits - 1ULL - offset;
        uint32_t seq = 0U;
        uint32_t k;

        for (k = 0U; k < 32U; ++k) {
            const uint64_t padded_index = top_index - k;
            seq |= (uint32_t)axis_raw_padded_bit_at(message, message_bits, padded_index, zero_count, length_bits) << k;
        }
#if AXIS_HAS_AVX2
        axis_soa32_step_default_block32(&soa, seq, seq);
#else
        axis_soa_step_default_block32(&soa, seq, seq);
#endif
    }

    for (offset = 0ULL; offset < AXIS1024_BLANK_ROUNDS; offset += 32ULL) {
        uint32_t constant_seq[AXIS_MAX_REGISTERS];

        axis_reupfull_constant_seq32(padded_bits, offset, constant_seq);
        axis_fast_soa_step_default_block32_register_seqs(&soa, constant_seq);
    }

    for (offset = 0ULL; offset < 1024ULL; offset += 32ULL) {
        uint32_t constant_seq[AXIS_MAX_REGISTERS];
        const uint32_t h0_seq = axis_fast_soa_gen512_h0_seq32(&soa);

        axis_write_digest1024_seq32_scalar_order(digest, (uint32_t)offset, h0_seq);
        axis_reupfull_constant_seq32(padded_bits, AXIS1024_BLANK_ROUNDS + offset, constant_seq);
        axis_fast_soa_step_default_block32_register_seqs(&soa, constant_seq);
    }
}
/* Function: axis_gen1024. Generates one AXIS-1024 digest bit from the current state. */

static AXIS_MAYBE_UNUSED uint8_t axis_gen1024(const axis_core_t* ctx) {
    return axis_gen512_h0(ctx);
}
/* Function: axis_emit_digest. Runs digest generation beats and writes the complete digest for the selected AXIS instance. */

static void axis_emit_digest(axis_core_t* ctx, uint8_t* digest) {
    uint64_t i;

    if (ctx->config->variant == AXIS_VARIANT_512) {
        for (i = 0ULL; i < ctx->config->blank_rounds; ++i) {
            axis_reupfull(ctx, axis_reupfull_constant_byte(ctx->padded_message_bits, i));
        }
        for (i = 0ULL; i < 256ULL; ++i) {
            const uint32_t bit_index = (uint32_t)(2ULL * i);

            axis_write_digest_bit(digest, AXIS_512_RESULT_LENGTH - 2U - bit_index, axis_gen512_h0(ctx));
            axis_write_digest_bit(digest, AXIS_512_RESULT_LENGTH - 1U - bit_index, axis_gen512_h1(ctx));
            axis_reupfull(ctx, axis_reupfull_constant_byte(ctx->padded_message_bits, ctx->config->blank_rounds + i));
        }
        return;
    }

    if (ctx->config->variant == AXIS_VARIANT_768) {
        for (i = 0ULL; i < ctx->config->blank_rounds; ++i) {
            axis_reupfull(ctx, axis_reupfull_constant_byte(ctx->padded_message_bits, i));
        }
        axis_write_digest768_bitwise(ctx, digest);
        return;
    }

    for (i = 0ULL; i < ctx->config->blank_rounds; ++i) {
        axis_reupfull(ctx, axis_reupfull_constant_byte(ctx->padded_message_bits, i));
    }
    for (i = 0ULL; i < 1024ULL; ++i) {
        axis_write_digest_bit(digest, AXIS_1024_RESULT_LENGTH - 1U - (uint32_t)i, axis_gen1024(ctx));
        axis_reupfull(ctx, axis_reupfull_constant_byte(ctx->padded_message_bits, ctx->config->blank_rounds + i));
    }
}
/* Function: axis_core_init. Initializes an AXIS context with the default configuration for a variant. */

void axis_core_init(axis_core_t* ctx, axis_variant_t variant) {
    axis_core_init_with_config(ctx, axis_default_config(variant));
}
/* Function: axis_core_init_with_config. Initializes an AXIS context from an explicit variant configuration. */

void axis_core_init_with_config(axis_core_t* ctx, const axis_variant_config_t* config) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->config = config;
    memcpy(ctx->regs, k_axis_init_words, sizeof(k_axis_init_words));
}
/* Function: axis_core_update. Appends a whole-byte message fragment to the AXIS context. */

void axis_core_update(axis_core_t* ctx, const void* msg, uint64_t msg_len) {
    axis_core_update_bits(ctx, msg, msg_len * 8ULL);
}
/* Function: axis_core_update_bits. Appends an arbitrary bit-length message fragment to the AXIS context. */

void axis_core_update_bits(axis_core_t* ctx, const void* msg, uint64_t msg_bit_len) {
    const uint8_t* bytes = (const uint8_t*)msg;
    uint64_t bit_index;
    const uint64_t start_bit = ctx->total_message_bits;
    const uint64_t end_bit = ctx->total_message_bits + msg_bit_len;

    if (msg_bit_len == 0ULL) {
        return;
    }
    if (bytes == 0) {
        return;
    }

    if (start_bit == 0ULL && (msg_bit_len & 7ULL) == 0ULL) {
        ctx->message = (uint8_t*)bytes;
        ctx->message_capacity_bits = msg_bit_len;
        ctx->total_message_bits = msg_bit_len;
        ctx->message_owned = 0U;
        return;
    }

    if (axis_reserve_message_bits(ctx, end_bit) != 0) {
        ctx->allocation_failed = 1U;
        return;
    }
    if (((start_bit | msg_bit_len) & 7ULL) == 0ULL) {
        memcpy(ctx->message + (start_bit >> 3), bytes, (size_t)(msg_bit_len >> 3));
        ctx->total_message_bits = end_bit;
        return;
    }
    for (bit_index = 0ULL; bit_index < msg_bit_len; ++bit_index) {
        axis_set_stored_message_bit(ctx, start_bit + bit_index, axis_read_input_bit(bytes, bit_index));
    }
    ctx->total_message_bits = end_bit;
}
/* Function: axis_core_final. Pads, absorbs, finalizes, emits the digest, and resets transient message storage. */

void axis_core_final(axis_core_t* ctx, void* out) {
    uint8_t* digest = (uint8_t*)out;
    const uint32_t digest_bits = ctx->config->result_bits;
    const uint32_t digest_bytes = digest_bits / 8U;

    if (ctx->allocation_failed != 0U) {
        memset(digest, 0, digest_bytes);
        axis_release_message(ctx);
        return;
    }

    memset(digest, 0, digest_bytes);
    if (ctx->config == axis_default_config(ctx->config->variant) && ctx->message_owned == 0U && ctx->message != 0 && (ctx->total_message_bits & 7ULL) == 0ULL) {
        if (ctx->config->variant == AXIS_VARIANT_512) {
            axis_soa_hash512_byte_aligned(ctx->message, ctx->total_message_bits, digest);
            axis_release_message(ctx);
            return;
        }
        if (ctx->config->variant == AXIS_VARIANT_768) {
            axis_soa_hash768_byte_aligned(ctx->message, ctx->total_message_bits, digest);
            axis_release_message(ctx);
            return;
        }
        if (ctx->config->variant == AXIS_VARIANT_1024) {
            axis_soa_hash1024_byte_aligned(ctx->message, ctx->total_message_bits, digest);
            axis_release_message(ctx);
            return;
        }
    }

    axis_absorb_padded_message(ctx);
    axis_release_message(ctx);
    axis_emit_digest(ctx, digest);
}
/* Function: axis_core_hash_bits. Computes a complete one-shot AXIS hash for an arbitrary bit-length message. */

void axis_core_hash_bits(axis_variant_t variant, const void* msg, uint64_t msg_bit_len, void* out) {
    axis_core_t ctx;
    uint8_t* digest = (uint8_t*)out;
    const uint32_t digest_bytes = axis_result_bits(variant) / 8U;

    if (digest == 0) {
        return;
    }
    if (msg_bit_len > 0ULL && msg == 0) {
        memset(digest, 0, digest_bytes);
        return;
    }

    axis_core_init(&ctx, variant);
    if ((msg_bit_len & 7ULL) == 0ULL) {
        if (variant == AXIS_VARIANT_512) {
            axis_soa_hash512_byte_aligned((const uint8_t*)msg, msg_bit_len, digest);
        } else if (variant == AXIS_VARIANT_768) {
            axis_soa_hash768_byte_aligned((const uint8_t*)msg, msg_bit_len, digest);
        } else if (variant == AXIS_VARIANT_1024) {
            axis_soa_hash1024_byte_aligned((const uint8_t*)msg, msg_bit_len, digest);
        } else {
            axis_core_update_bits(&ctx, msg, msg_bit_len);
            axis_core_final(&ctx, digest);
        }
        return;
    }

    axis_core_update_bits(&ctx, msg, msg_bit_len);
    axis_core_final(&ctx, digest);
}
