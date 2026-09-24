#ifdef _MSC_VER
#pragma warning(disable: 4819)
#endif
#ifdef _MSC_VER
#pragma warning(disable: 4819)
#endif
#include "parameters.h"
#include "poly.h"
#include "dke_utils.h"
#include "verify.h"
#include <stdint.h>
#include <stddef.h>

#if defined(DKE_USE_AVX2)

#include <immintrin.h>
#include <string.h>

/* AVX2: expand each bit of coins to 0 or -1 in 16-bit coefficients.
 * Process 2 bytes per iteration → 16 coefficients per __m256i. */
static void DKE_rand_to_poly(poly *b, const uint8_t coins[DKE_N/8]) {
    unsigned int i;
    /* Bit masks for each of 8 bit positions within a byte */
    const __m128i bitmask = _mm_set_epi16(128, 64, 32, 16, 8, 4, 2, 1);
    const __m256i bitmask256 = _mm256_broadcastsi128_si256(bitmask);
    const __m256i zero = _mm256_setzero_si256();

    for (i = 0; i < DKE_N / 16; i++) {
        /* Broadcast byte 0 to low 128 bits, byte 1 to high 128 bits */
        __m128i lo = _mm_set1_epi16((int16_t)coins[2*i]);
        __m128i hi = _mm_set1_epi16((int16_t)coins[2*i + 1]);
        __m256i v = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);

        /* AND with bitmask: non-zero where bit is set */
        v = _mm256_and_si256(v, bitmask256);
        /* Compare equal to zero: -1 where bit=0, 0 where bit=1 */
        v = _mm256_cmpeq_epi16(v, zero);
        /* We want bit=1 → -1, bit=0 → 0. cmpeq gives opposite. XOR with -1: */
        /* Actually: cmpeq gives -1 (0xFFFF) where zero, 0 where non-zero.
         * We want -1 where non-zero, 0 where zero. So NOT: */
        v = _mm256_xor_si256(v, _mm256_set1_epi16(-1));
        /* Now v = -1 where bit=1, 0 where bit=0 */

        _mm256_store_si256((__m256i *)&b->coeffs[16 * i], v);
    }
}

#elif defined(DKE_USE_AARCH64)

#include <arm_neon.h>

/* NEON: expand each bit of coins to 0 or -1 in 16-bit coefficients.
 * Process 1 byte per iteration → 8 coefficients per int16x8_t. */
static void DKE_rand_to_poly(poly *b, const uint8_t coins[DKE_N/8]) {
    unsigned int i;
    const int16x8_t bitmask = {1, 2, 4, 8, 16, 32, 64, 128};
    const int16x8_t zero = vdupq_n_s16(0);

    for (i = 0; i < DKE_N / 8; i++) {
        int16x8_t v = vdupq_n_s16((int16_t)coins[i]);
        v = vandq_s16(v, bitmask);
        /* v != 0 → -1, v == 0 → 0 */
        uint16x8_t cmp = vceqq_s16(v, zero);
        v = vmvnq_s16(vreinterpretq_s16_u16(cmp)); /* NOT: 0→-1, -1→0 */
        vst1q_s16(&b->coeffs[8 * i], v);
    }
}

#elif defined(DKE_USE_OPT_C)

/* Optimized: direct bit extraction, no function call per coefficient */
static void DKE_rand_to_poly(poly *b, const uint8_t coins[DKE_N/8]) {
    size_t i, j;
    for (i = 0; i < DKE_N / 8; i++) {
        uint8_t byte = coins[i];
        for (j = 0; j < 8; j++) {
            /* -((byte >> j) & 1) gives 0 or -1 */
            b->coeffs[8*i+j] = -(int16_t)((byte >> j) & 1);
        }
    }
}

#else

static void DKE_rand_to_poly(poly *b, const uint8_t coins[DKE_N/8]) {
    size_t i, j;
    for (i = 0; i < DKE_N / 8; i++) {
        for (j = 0; j < 8; j++) {
            b->coeffs[8*i+j] = 0;
            DKE_cmov_int16(b->coeffs + 8*i+j, -1, (coins[i] >> j) & 1);
        }
    }
}

#endif /* DKE_USE_OPT_C */

void DKE_signal(uint8_t sig[DKE_SIGNALBYTES],
                const poly *k,
                const uint8_t coins[DKE_N/8]) {
    poly b;
    DKE_rand_to_poly(&b, coins);
    DKE_poly_add(&b, &b, k);
    DKE_poly_reduce(&b);
    DKE_getsignal(sig, &b);
}

static void DKE_apply_signal(poly *k, const uint8_t sig[DKE_SIGNALBYTES]) {
    poly wL;
    DKE_poly_fromsignal(&wL, sig);
    DKE_poly_sub(k, k, &wL);
    DKE_poly_reduce_center(k);   /* maps to {-(q-1)/2, ..., (q-1)/2} (ref reduce_center) */
}

static void DKE_mod2(uint8_t ss[DKE_SSBYTES], poly *k) {
#if defined(DKE_USE_AVX2)
    unsigned int i;
    const __m256i mask1 = _mm256_set1_epi16(1);

    for (i = 0; i < DKE_SSBYTES / 4; i++) {
        /* Load 32 coefficients (2 x __m256i) */
        __m256i v0 = _mm256_load_si256((__m256i *)&k->coeffs[32 * i]);
        __m256i v1 = _mm256_load_si256((__m256i *)&k->coeffs[32 * i + 16]);
        /* Extract bit 0 */
        v0 = _mm256_and_si256(v0, mask1);
        v1 = _mm256_and_si256(v1, mask1);
        /* Shift bit 0 to the SIGN bit (bit 15). packs_epi16 does signed
         * saturation, so 0x8000 -> 0x80 (bit7 set), 0 -> 0; movemask then reads
         * bit7. (Shifting to bit 7 = 0x0080 = +128 would saturate to 0x7F.) */
        v0 = _mm256_slli_epi16(v0, 15);
        v1 = _mm256_slli_epi16(v1, 15);
        /* Pack 16-bit to 8-bit (sign bit -> bit 7) */
        __m256i packed = _mm256_packs_epi16(v0, v1);
        packed = _mm256_permute4x64_epi64(packed, 0xD8);
        /* movemask extracts bit 7 of each byte → 32 bits = 4 bytes */
        uint32_t bits = (uint32_t)_mm256_movemask_epi8(packed);
        memcpy(&ss[4 * i], &bits, 4);
    }
    /* Handle remaining bytes if SSBYTES not multiple of 4 */
    for (i = (DKE_SSBYTES / 4) * 4; i < DKE_SSBYTES; i++) {
        unsigned int j;
        ss[i] = 0;
        for (j = 0; j < 8; j++) {
            ss[i] |= (k->coeffs[8*i+j] & 1) << j;
        }
    }
#elif defined(DKE_USE_AARCH64)
    unsigned int i, j;
    uint16_t t;
    /* NEON doesn't have movemask, use scalar extraction */
    for (i = 0; i < DKE_SSBYTES; i++) {
        ss[i] = 0;
        for (j = 0; j < 8; j++) {
            t = k->coeffs[8*i+j];
            t &= 1;
            ss[i] |= t << j;
        }
    }
#else
    unsigned int i, j;
    uint16_t t;
    for (i = 0; i < DKE_SSBYTES; i++) {
        ss[i] = 0;
        for (j = 0; j < 8; j++) {
            t = k->coeffs[8*i+j];
            t &= 1;
            ss[i] |= t << j;
        }
    }
#endif
}

void DKE_derive_ss(uint8_t ss[DKE_SSBYTES],
                   poly *k,
                   const uint8_t sig[DKE_SIGNALBYTES]) {
    DKE_apply_signal(k, sig);
    DKE_mod2(ss, k);
}
