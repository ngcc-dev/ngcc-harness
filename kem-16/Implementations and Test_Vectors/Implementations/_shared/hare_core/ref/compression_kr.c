/**
 * @file compression_kr.c
 * @brief KR [51,41] compression backend (reference path main backend).
 */

#include "compression.h"

#include <stdint.h>
#include <string.h>
#include "parameters.h"
#include "kr_syndrome_table_generated.h"

#define KR_N 51
#define KR_K 41
#define KR_R 10

/*
 * Systematic parity-check matrix H_KR = [I_10 | M_KR].
 * Row i stores the 41-bit mask for M_KR[i][*].
 */
static const uint64_t KR_M_ROWS[KR_R] = {
    0xA8FE70FC2AULL,
    0x52C05A8E7BULL,
    0x17DDD5D677DULL,
    0x1D2E904544ULL,
    0x1A4D94F4333ULL,
    0xCFA303A571ULL,
    0x1F3506F20F4ULL,
    0x14BD402923FULL,
    0xB17A40F8DFULL,
    0xD75EC73E00ULL,
};

static uint8_t parity64(uint64_t x) {
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x &= 0xFULL;
    return (uint8_t)((0x6996u >> x) & 1u);
}

static uint16_t ct_mask_u16_eq(uint16_t a, uint16_t b) {
    uint16_t x = (uint16_t)(a ^ b);
    x |= (uint16_t)(0u - x);
    x = (uint16_t)(x >> 15);
    return (uint16_t)(0u - (uint16_t)(1u ^ x));
}

static uint64_t ct_lookup_leader(uint16_t syndrome) {
    uint64_t leader = 0;
    for (uint16_t i = 0; i < (1u << KR_R); ++i) {
        const uint16_t mask = ct_mask_u16_eq(i, syndrome);
        leader |= KR_SYNDROME_LEADER[i] & (uint64_t)(-(int64_t)(mask & 1u));
    }
    return leader;
}

static uint8_t get_bit64(const uint64_t *v, int bitpos) {
    return (uint8_t)((v[bitpos >> 6] >> (bitpos & 63)) & 1ULL);
}

static void set_bit64(uint64_t *v, int bitpos, uint8_t bit) {
    const uint64_t mask = 1ULL << (bitpos & 63);
    const uint64_t bit_mask = 0ULL - (uint64_t)(bit & 1u);
    const int word = bitpos >> 6;

    /* Branchless set/clear so compression can be used in FO re-encryption without
     * data-dependent control flow on compressed payload bits. */
    v[word] = (v[word] & ~mask) | (bit_mask & mask);
}

static uint64_t kr_encode_41_to_51(uint64_t msg41) {
    uint64_t codeword = (msg41 & ((1ULL << KR_K) - 1ULL)) << KR_R;
    for (int r = 0; r < KR_R; r++) {
        uint8_t p = parity64(msg41 & KR_M_ROWS[r]);
        codeword |= ((uint64_t)p) << r;
    }
    return codeword;
}

static uint64_t kr_decode_51_to_41(uint64_t rx51) {
    uint64_t msg41 = (rx51 >> KR_R) & ((1ULL << KR_K) - 1ULL);
    uint16_t syndrome = 0;

    for (int r = 0; r < KR_R; r++) {
        uint8_t parity_part = (uint8_t)((rx51 >> r) & 1ULL);
        uint8_t check_part = parity64(msg41 & KR_M_ROWS[r]);
        syndrome |= (uint16_t)((parity_part ^ check_part) << r);
    }

    rx51 ^= ct_lookup_leader(syndrome);
    return (rx51 >> KR_R) & ((1ULL << KR_K) - 1ULL);
}

void ciphertext_compress(uint64_t *m_tilde, uint64_t *v2, const uint64_t *v_vec) {
    const int blocks = PARAM_COMP_Q;
    const int tail_bits = PARAM_NV2;

    memset(m_tilde, 0, VEC_NM_SIZE_64 * sizeof(uint64_t));
    memset(v2, 0, VEC_NV2_SIZE_64 * sizeof(uint64_t));

    for (int i = 0; i < blocks; i++) {
        uint64_t block = 0;
        int src_base = i * KR_N;
        int dst_base = i * KR_K;

        for (int j = 0; j < KR_N; j++) {
            block |= ((uint64_t)get_bit64(v_vec, src_base + j)) << j;
        }

        uint64_t msg = kr_decode_51_to_41(block);
        for (int j = 0; j < KR_K; j++) {
            set_bit64(m_tilde, dst_base + j, (uint8_t)((msg >> j) & 1ULL));
        }
    }

    for (int j = 0; j < tail_bits; j++) {
        set_bit64(v2, j, get_bit64(v_vec, PARAM_L2 + j));
    }
}

void ciphertext_decompress(uint64_t *v_vec, const uint64_t *m_tilde, const uint64_t *v2) {
    const int blocks = PARAM_COMP_Q;
    const int tail_bits = PARAM_NV2;

    memset(v_vec, 0, VEC_N_SIZE_64 * sizeof(uint64_t));

    for (int i = 0; i < blocks; i++) {
        uint64_t msg = 0;
        int src_base = i * KR_K;
        int dst_base = i * KR_N;

        for (int j = 0; j < KR_K; j++) {
            msg |= ((uint64_t)get_bit64(m_tilde, src_base + j)) << j;
        }

        uint64_t codeword = kr_encode_41_to_51(msg);
        for (int j = 0; j < KR_N; j++) {
            set_bit64(v_vec, dst_base + j, (uint8_t)((codeword >> j) & 1ULL));
        }
    }

    for (int j = 0; j < tail_bits; j++) {
        set_bit64(v_vec, PARAM_L2 + j, get_bit64(v2, j));
    }
    /* [PARAM_L1, PARAM_N) remains zero by memset above. */
}
