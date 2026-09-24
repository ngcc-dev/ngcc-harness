#include "../reduce_neon.h"
#include <stdint.h>
#include <stdio.h>

static int check_equal_i32(const char *name, int32_t got, int32_t want) {
    if (got != want) {
        printf("%s mismatch: got %d, want %d\n", name, got, want);
        return 1;
    }
    return 0;
}

int main(void) {
    const int32_t a[8] = {0, 1, -1, Q - 1, Q, -Q, 12345, -54321};
    const int32_t b[8] = {1, -2, 3, Q - 2, -Q + 3, 7, -11, 19};
    int32_t out[8];
    int errors = 0;

    for (unsigned int i = 0; i < 8; i += 4) {
        int32x4_t va = vld1q_s32(&a[i]);
        int32x4_t vb = vld1q_s32(&b[i]);

        vst1q_s32(&out[i], darts_neon_freeze(va));
        for (unsigned int j = 0; j < 4; j++) {
            errors += check_equal_i32("freeze", out[i + j],
                                      freeze(a[i + j]));
        }

        vst1q_s32(&out[i], darts_neon_fqmul(va, vb));
        for (unsigned int j = 0; j < 4; j++) {
            errors += check_equal_i32("fqmul", out[i + j],
                                      montgomery_reduce((int64_t)a[i + j] * b[i + j]));
        }
    }

    if (errors) {
        return 1;
    }

    puts("NEON finite-field helper tests passed!");
    return 0;
}
