#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>

#include "../avx2_ntt.h"

#define TESTS 100
__m256i t;
static int compare_mod32(const int32_t *a, const int32_t *b, int n, int32_t Q, const char *label) {
    int mismatches = 0;
    for (int i = 0; i < n; ++i) {
        int64_t diff = (int64_t)a[i] - (int64_t)b[i];
        if ((diff % Q) != 0) {
            if (mismatches == 0) {
                printf("%s TEST FAILED! Mismatches:\n", label);
            }
            ++mismatches;
            printf("  - idx %d: ref=%d, avx2=%d (Q=%d)\n", i, a[i], b[i], Q);
        }
    }
    if (mismatches == 0) {
        printf("%s TEST PASSED!\n", label);
    }
    return mismatches;
}

static int test_avx2_ntt_intrinsic() {
    nttpoly_n1087 ref, avx;
    for (int i = 0; i < N_N1087; ++i) {
        int32_t x = (int32_t)(rand() % DTRU_Q);
        ref.coeffs[i] = x;
        avx.coeffs[i] = x;
    }

    poly_ntt(&ref);
    ntt_avx2_intrinsic(avx.vec);
    for (int i = 0; i < N_N1087/(24*2); i++) {
        for (int j = 0;j < 3; j++){
            shuffle8(ref.vec[j+i*2*3], ref.vec[j+i*2*3+3]);
        }
    }
    //可能是溢出了，需要加伪梅森约减
    for (int i = 0; i < N_N1087/(24*2); i++) {
        shuffle4(ref.vec[0+i*2*3], ref.vec[4+i*2*3]);
        shuffle4(ref.vec[3+i*2*3], ref.vec[2+i*2*3]);
        shuffle4(ref.vec[1+i*2*3], ref.vec[5+i*2*3]);
    }
    for (int i = 0; i < N_N1087/(24*2); i++) {
        shuffle2(ref.vec[0+i*2*3], ref.vec[2+i*2*3]);
        shuffle2(ref.vec[4+i*2*3], ref.vec[1+i*2*3]);
        shuffle2(ref.vec[3+i*2*3], ref.vec[5+i*2*3]);
    }

    return compare_mod32(ref.coeffs, avx.coeffs, N_N1087, Q_N1087, "avx2_ntt_intrinsic");
}

static int test_avx2_invntt_intrinsic() {
    nttpoly_n1087 ref, avx;
    for (int i = 0; i < N_N1087; ++i) {
        int32_t x = (int32_t)(rand() % DTRU_Q);
        ref.coeffs[i] = x;
        avx.coeffs[i] = x;
    }
    poly_ntt(&ref);
    ntt_avx2_intrinsic(avx.vec);

    poly_invntt(&ref);
    invntt_avx2_intrinsic(avx.vec);
    
    return compare_mod32(ref.coeffs, avx.coeffs, N_N1087, Q_N1087, "avx2_invntt_intrinsic");
}

static int test_avx2_basemul3x3_intrinsic() {
    nttpoly_n1087 refa, refb,refc ,avxa,avxb,avxc;
    for (int i = 0; i < N_N1087; ++i) {
        int32_t a = (int32_t)(rand() % DTRU_Q);
        int32_t b = (int32_t)(rand() % DTRU_Q);
        refa.coeffs[i] = a;
        refb.coeffs[i] = b;
        avxa.coeffs[i] = a;
        avxb.coeffs[i] = b;
    }
    poly_ntt(&refa);
    poly_ntt(&refb);
    ntt_avx2_intrinsic(avxa.vec);
    ntt_avx2_intrinsic(avxb.vec);

    poly_basemul(&refc,&refa,&refb);
    basemul3x3_avx2_intrinsic(avxc.vec,avxa.vec,avxb.vec);

    poly_invntt(&refc);
    invntt_avx2_intrinsic(avxc.vec);
    
    return compare_mod32(refc.coeffs, avxc.coeffs, N_N1087, Q_N1087, "avx2_basemul3x3_intrinsic");
}
int main() {
    srand((unsigned)time(NULL));
    int errorCount = 0;
    for (int i = 0; i < TESTS; ++i) {
        int status = test_avx2_ntt_intrinsic();
        status |= test_avx2_invntt_intrinsic();
        status |= test_avx2_basemul3x3_intrinsic();

        if (status != 0) {
            errorCount++;
        }
    }
    if (errorCount != 0) {
        printf("\nOverall Status: %d TESTS FAILED!\n", errorCount);
    } else {
        printf("\nOverall Status: ALL TESTS PASSED!\n");
    }
    return errorCount;
}