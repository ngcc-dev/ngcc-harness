#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "compression.h"
#include "parameters.h"

static void fill_pattern(uint64_t *v, unsigned round) {
    for (int i = 0; i < VEC_N_SIZE_64; ++i) {
        uint64_t x = 0x9e3779b97f4a7c15ULL ^ ((uint64_t)round << 32) ^ (uint64_t)i;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        v[i] = x;
    }
    if ((PARAM_N & 63) != 0) {
        v[VEC_N_SIZE_64 - 1] &= ((1ULL << (PARAM_N & 63)) - 1ULL);
    }
}

int main(void) {
    uint64_t v[VEC_N_SIZE_64];
    uint64_t mt1[VEC_NM_SIZE_64];
    uint64_t mt2[VEC_NM_SIZE_64];
    uint64_t tail1[VEC_NV2_SIZE_64];
    uint64_t tail2[VEC_NV2_SIZE_64];
    uint64_t dec1[VEC_N_SIZE_64];
    uint64_t dec2[VEC_N_SIZE_64];

    if (PARAM_COMP_N != 51 || PARAM_COMP_K != 41 || PARAM_COMP_R != 2) {
        printf("kr_projection_parameter_mismatch\n");
        return 1;
    }

    for (unsigned round = 0; round < 32; ++round) {
        fill_pattern(v, round);
        memset(mt1, 0, sizeof mt1);
        memset(mt2, 0, sizeof mt2);
        memset(tail1, 0, sizeof tail1);
        memset(tail2, 0, sizeof tail2);
        memset(dec1, 0, sizeof dec1);
        memset(dec2, 0, sizeof dec2);

        ciphertext_compress(mt1, tail1, v);
        ciphertext_decompress(dec1, mt1, tail1);
        ciphertext_compress(mt2, tail2, dec1);
        ciphertext_decompress(dec2, mt2, tail2);

        if (memcmp(mt1, mt2, sizeof mt1) != 0 || memcmp(tail1, tail2, sizeof tail1) != 0) {
            printf("kr_projection_compress_not_idempotent round=%u\n", round);
            return 2;
        }
        if (memcmp(dec1, dec2, sizeof dec1) != 0) {
            printf("kr_projection_decompress_not_idempotent round=%u\n", round);
            return 3;
        }
        for (int bit = PARAM_L1; bit < PARAM_N; ++bit) {
            if ((dec1[bit >> 6] >> (bit & 63)) & 1ULL) {
                printf("kr_projection_tail_not_zero round=%u bit=%d\n", round, bit);
                return 4;
            }
        }
    }

    puts("kr_projection_pass");
    return 0;
}
