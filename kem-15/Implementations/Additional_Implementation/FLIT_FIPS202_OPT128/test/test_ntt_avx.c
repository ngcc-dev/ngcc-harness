#include <stdio.h>
#include <string.h>
#include "../params.h"
#include "../ntt.h"
#include "../ntt_avx.h"
#include "../consts.h"
#include "../reduce.h"

extern const int16_t qdata[];

#if KEM_MODE == 128 || KEM_MODE == 256
int main(void) {
    int16_t a[N] __attribute__((aligned(32)));
    int16_t a_ref[N] __attribute__((aligned(32)));
    int16_t orig[N] __attribute__((aligned(32)));
    int errors = 0;

    /* Test 1: NTT forward matches reference */
    for (int t = 0; t < 1000; t++) {
        for (int i = 0; i < N; i++) {
            a[i] = (int16_t)((i * 123 + t * 7) % Q);
            a_ref[i] = a[i];
        }

        ntt(a_ref);
        ntt_avx(a, qdata);

        for (int i = 0; i < N; i++) {
            if (freeze(a[i]) != freeze(a_ref[i])) {
                errors++;
                if (errors <= 3)
                    printf("ntt mismatch at i=%d: avx=%d ref=%d\n", i, a[i], a_ref[i]);
            }
        }
    }

    /* Test 2: invNTT matches reference */
    for (int t = 0; t < 1000; t++) {
        for (int i = 0; i < N; i++) {
            a[i] = (int16_t)((i * 47 + t * 13) % Q);
            a_ref[i] = a[i];
        }

        invntt_tomont(a_ref);
        invntt_avx(a, qdata);

        for (int i = 0; i < N; i++) {
            if (freeze(a[i]) != freeze(a_ref[i])) {
                errors++;
                if (errors <= 3)
                    printf("invntt mismatch at i=%d: avx=%d ref=%d\n", i, a[i], a_ref[i]);
            }
        }
    }

    /* Test 3: NTT + invNTT roundtrip */
    int rt_ok = 1;
    for (int t = 0; t < 100 && rt_ok; t++) {
        for (int i = 0; i < N; i++) {
            a[i] = (int16_t)((i * 77 + t * 19) % Q);
            orig[i] = a[i];
        }

        ntt_avx(a, qdata);
        invntt_avx(a, qdata);

        for (int i = 0; i < N; i++) {
            /* After NTT+invNTT we get mont(f) = f*R mod Q.
             * montgomery_reduce strips R to recover f. */
            int16_t val = freeze(montgomery_reduce((int32_t)a[i]));
            if (val != freeze(orig[i])) {
                errors++;
                if (errors <= 3)
                    printf("roundtrip mismatch at i=%d: got=%d orig=%d\n", i, val, freeze(orig[i]));
                rt_ok = 0;
            }
        }
    }

    if (errors == 0) {
        printf("ntt_avx / invntt_avx: ALL TESTS PASSED\n");
    } else {
        printf("ntt_avx / invntt_avx: %d ERRORS\n", errors);
        return 1;
    }

    return 0;
}
#endif
