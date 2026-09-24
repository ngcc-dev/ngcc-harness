#include "sm3.h"

#if SM3_ENABLE_NEON
#include "sm3_internal.h"
#endif

int sm3_x4_is_accelerated(void) {
#if SM3_ENABLE_NEON
    if (sm3_cpu_supports_neon()) {
        return 1;
    }
#endif
    return 0;
}

void sm3x4(uint8_t out0[SM3_DIGEST_BYTES],
           uint8_t out1[SM3_DIGEST_BYTES],
           uint8_t out2[SM3_DIGEST_BYTES],
           uint8_t out3[SM3_DIGEST_BYTES],
           const uint8_t *in0,
           const uint8_t *in1,
           const uint8_t *in2,
           const uint8_t *in3,
           size_t inlen) {
#if SM3_ENABLE_NEON
    if (sm3_cpu_supports_neon()) {
        sm3x4_neon(out0, out1, out2, out3, in0, in1, in2, in3, inlen);
        return;
    }
#endif

    sm3(out0, in0, inlen);
    sm3(out1, in1, inlen);
    sm3(out2, in2, inlen);
    sm3(out3, in3, inlen);
}
