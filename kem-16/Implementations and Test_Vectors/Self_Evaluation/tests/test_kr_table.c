#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "parameters.h"

#define KR_N 51
#define KR_K 41
#define KR_R 10

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

static unsigned popcount64(uint64_t x) {
    unsigned c = 0;
    while (x != 0) {
        c += (unsigned)(x & 1ULL);
        x >>= 1;
    }
    return c;
}

int main(void) {
    uint16_t col[KR_N];
    uint8_t best[1u << KR_R];

    if (PARAM_COMP_N != KR_N || PARAM_COMP_K != KR_K || PARAM_COMP_R != 2) {
        printf("kr_parameter_mismatch comp_n=%d comp_k=%d comp_r=%d\n",
               PARAM_COMP_N, PARAM_COMP_K, PARAM_COMP_R);
        return 1;
    }

    for (unsigned s = 0; s < (1u << KR_R); ++s) {
        best[s] = 0xffu;
    }

    for (int i = 0; i < KR_R; ++i) {
        col[i] = (uint16_t)(1u << i);
    }
    for (int j = 0; j < KR_K; ++j) {
        uint16_t c = 0;
        for (int r = 0; r < KR_R; ++r) {
            c |= (uint16_t)(((KR_M_ROWS[r] >> j) & 1ULL) << r);
        }
        col[KR_R + j] = c;
    }

    best[0] = 0;
    for (int i = 0; i < KR_N; ++i) {
        if (best[col[i]] > 1u) best[col[i]] = 1u;
    }
    for (int i = 0; i < KR_N; ++i) {
        for (int j = i + 1; j < KR_N; ++j) {
            uint16_t s = (uint16_t)(col[i] ^ col[j]);
            if (best[s] > 2u) best[s] = 2u;
        }
    }

    for (unsigned s = 0; s < (1u << KR_R); ++s) {
        if (best[s] > 2u) {
            printf("kr_uncovered_syndrome s=%u\n", s);
            return 2;
        }
    }

    for (int r = 0; r < KR_R; ++r) {
        if ((KR_M_ROWS[r] >> KR_K) != 0ULL) {
            printf("kr_row_mask_too_wide row=%d mask=%llx\n", r, (unsigned long long)KR_M_ROWS[r]);
            return 3;
        }
        (void)popcount64(KR_M_ROWS[r]);
    }

    puts("kr_table_pass");
    return 0;
}
