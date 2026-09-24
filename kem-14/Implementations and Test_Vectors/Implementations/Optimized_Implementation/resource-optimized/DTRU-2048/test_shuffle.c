#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>
#include "shuffle.h"

static void print_vec16(const char *name, __m256i v) {
    uint16_t x[16];
    _mm256_storeu_si256(x, v);
    printf("%s:", name);
    for (int i = 0; i < 16; i++) {
        printf(" %u", x[i]);
    }
    printf("\n");
}

static int test_shuffle_basemul16x16(void) {
    __m256i input[16];
    __m256i orig[16];
    uint16_t seed = 1000;

    for (int i = 0; i < 16; i++) {
        uint16_t tmp[16];
        for (int j = 0; j < 16; j++)
            tmp[j] = (uint16_t)(seed + i*100 + j);
        input[i] = _mm256_loadu_si256(tmp);
    }

    for (int i = 0; i < 16; i++) {
        orig[i] = input[i];
    }

    printf("\n--- shuffle_to_basemul16x16 input (first 16) ---\n");
    for (int i = 0; i < 16; i++) { char name[16]; snprintf(name, sizeof(name), "in16[%d]", i); print_vec16(name, orig[i]); }

    shuffle_to_basemul16x16(input);

    printf("\n--- after shuffle_to_basemul16x16 (first 16) ---\n");
    for (int i = 0; i < 16; i++) { char name[16]; snprintf(name, sizeof(name), "shuf16[%d]", i); print_vec16(name, input[i]); }

    shuffle_from_basemul16x16(input);

    printf("\n--- after shuffle_from_basemul16x16 (first 16) ---\n");
    for (int i = 0; i < 16; i++) { char name[16]; snprintf(name, sizeof(name), "rest16[%d]", i); print_vec16(name, input[i]); }

    printf("shuffle_to_basemul16x16 print done\n");
    return 0;
}

int main(void) {
    test_shuffle_basemul16x16();
    return 0;
}