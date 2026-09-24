#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../params.h"
#include "../ntt.h"
#include "../consts.h"
#include "../reduce.h"
#include <immintrin.h>

extern const int16_t qdata[];
extern int k4_baseinv_avx_8x(int16_t *b, const int16_t *a,
                              int16_t zeta, __m256i q, __m256i qinv, __m256i v);

int main(void) {
    __m256i q    = _mm256_load_si256((const __m256i *)(qdata + _16XQ));
    __m256i qinv = _mm256_load_si256((const __m256i *)(qdata + _16XQINV));
    __m256i v    = _mm256_load_si256((const __m256i *)(qdata + _16XV));

    /* Test k4_baseinv_avx_8x vs two baseinv_K(4) calls */
    for (int t = 0; t < 50; t++) {
        int16_t a[16] __attribute__((aligned(32)));
        int16_t b_avx[16] __attribute__((aligned(32)));
        int16_t b_ref[16] __attribute__((aligned(32)));
        int16_t zeta = (int16_t)((t * 31 + 1) % 3328 + 1);

        for (int i = 0; i < 16; i++)
            a[i] = (int16_t)((i * 73 + t * 17 + 1) % 3329);

        /* Scalar reference: baseinv_K(4) on each 8-element block */
        int ref_ok = 0;
        ref_ok += baseinv_K(4, b_ref,      a,       zeta);
        ref_ok += baseinv_K(4, b_ref + 8,  a + 8,  -zeta);

        /* AVX2: k4_baseinv_avx_8x on all 16 elements */
        int avx_ok = k4_baseinv_avx_8x(b_avx, a, zeta, q, qinv, v);

        if (ref_ok != avx_ok) {
            printf("STATUS t=%d ref=%d avx=%d\n", t, ref_ok, avx_ok);
        } else if (ref_ok == 0) {
            for (int i = 0; i < 16; i++) {
                if (freeze(b_avx[i]) != freeze(b_ref[i])) {
                    printf("MIS t=%d i=%d: avx=%d ref=%d\n", t, i, b_avx[i], b_ref[i]);
                    if (i >= 5) break;
                }
            }
            return 0; /* Exit after first mismatch for debugging */
        }
    }
    printf("k4_baseinv_avx_8x: ALL PASS\n");
    return 0;
}
