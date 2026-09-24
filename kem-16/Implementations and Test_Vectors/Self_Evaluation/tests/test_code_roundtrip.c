#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "code.h"
#include "parameters.h"

/**
 * Exercise code_encode/code_decode roundtrips for the configured instance.
 */
int main(void) {
    uint8_t m[PARAM_K];
    uint64_t em[VEC_N1N2_SIZE_64 + 8];
    uint8_t m2[PARAM_K];
    memset(m, 0, sizeof(m));
    for (int i = 0; i < 256; ++i) {
        memset(m, 0, sizeof(m));
        memset(em, 0, sizeof(em));
        memset(m2, 0, sizeof(m2));
        m[0] = (uint8_t)i;
        code_encode(em, m);
        code_decode(m2, em);
        if (memcmp(m, m2, PARAM_K) != 0) {
            printf("code_roundtrip_fail i=%d\n", i);
            return 1;
        }
    }
    puts("code_roundtrip_pass");
    return 0;
}

