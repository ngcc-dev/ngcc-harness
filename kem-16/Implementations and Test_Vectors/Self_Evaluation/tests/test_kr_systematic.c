#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "compression.h"
#include "parameters.h"

#define KR_N 51
#define KR_K 41
#define KR_R 10

static const uint64_t KR_M_ROWS[KR_R] = {
    UINT64_C(0x0A8FE70FC2A), UINT64_C(0x052C05A8E7B),
    UINT64_C(0x17DDD5D677D), UINT64_C(0x1D2E904544),
    UINT64_C(0x1A4D94F4333), UINT64_C(0x0CFA303A571),
    UINT64_C(0x1F3506F20F4), UINT64_C(0x14BD402923F),
    UINT64_C(0x0B17A40F8DF), UINT64_C(0x0D75EC73E00),
};

static uint8_t parity64(uint64_t x) {
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x &= UINT64_C(0xF);
    return (uint8_t)((UINT16_C(0x6996) >> x) & 1U);
}

static uint8_t get_bit(const uint64_t *v, unsigned bit) {
    return (uint8_t)((v[bit >> 6] >> (bit & 63U)) & UINT64_C(1));
}

static void set_bit(uint64_t *v, unsigned bit, uint8_t value) {
    const uint64_t mask = UINT64_C(1) << (bit & 63U);
    if (value != 0U) v[bit >> 6] |= mask;
    else v[bit >> 6] &= ~mask;
}

static int check_message(uint64_t msg) {
    uint64_t m_tilde[VEC_NM_SIZE_64];
    uint64_t m_roundtrip[VEC_NM_SIZE_64];
    uint64_t v2[VEC_NV2_SIZE_64];
    uint64_t v2_roundtrip[VEC_NV2_SIZE_64];
    uint64_t v[VEC_N_SIZE_64];

    memset(m_tilde, 0, sizeof m_tilde);
    memset(m_roundtrip, 0, sizeof m_roundtrip);
    memset(v2, 0, sizeof v2);
    memset(v2_roundtrip, 0, sizeof v2_roundtrip);
    memset(v, 0, sizeof v);

    msg &= (UINT64_C(1) << KR_K) - UINT64_C(1);
    for (unsigned j = 0; j < KR_K; ++j) set_bit(m_tilde, j, (uint8_t)((msg >> j) & 1U));
    ciphertext_decompress(v, m_tilde, v2);

    /* G_KR=[M_KR^T|I_41]: parity bits c[0:10], message bits c[10:51]. */
    for (unsigned j = 0; j < KR_K; ++j) {
        if (get_bit(v, KR_R + j) != ((msg >> j) & 1U)) return 1;
    }
    for (unsigned r = 0; r < KR_R; ++r) {
        if (get_bit(v, r) != parity64(msg & KR_M_ROWS[r])) return 2;
    }

    ciphertext_compress(m_roundtrip, v2_roundtrip, v);
    for (unsigned j = 0; j < KR_K; ++j) {
        if (get_bit(m_roundtrip, j) != ((msg >> j) & 1U)) return 3;
    }
    return 0;
}

int main(void) {
    if (PARAM_COMP_N != KR_N || PARAM_COMP_K != KR_K || PARAM_COMP_R != 2) {
        puts("kr_systematic_parameter_fail");
        return 1;
    }
    if (check_message(0) != 0 || check_message((UINT64_C(1) << KR_K) - 1U) != 0 ||
        check_message(UINT64_C(0x15555555555)) != 0) {
        puts("kr_systematic_pattern_fail");
        return 2;
    }
    for (unsigned j = 0; j < KR_K; ++j) {
        if (check_message(UINT64_C(1) << j) != 0) {
            printf("kr_systematic_basis_fail bit=%u\n", j);
            return 3;
        }
    }
    puts("kr_systematic_pass");
    return 0;
}
