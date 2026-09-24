#include "sm3.h"

#if SM3_ENABLE_AVX2
#include "sm3_internal.h"
#endif

int sm3_x8_is_accelerated(void) {
#if SM3_ENABLE_AVX2
    return 1;
#endif
    return 0;
}

void sm3x8(uint8_t out0[SM3_DIGEST_BYTES],
           uint8_t out1[SM3_DIGEST_BYTES],
           uint8_t out2[SM3_DIGEST_BYTES],
           uint8_t out3[SM3_DIGEST_BYTES],
           uint8_t out4[SM3_DIGEST_BYTES],
           uint8_t out5[SM3_DIGEST_BYTES],
           uint8_t out6[SM3_DIGEST_BYTES],
           uint8_t out7[SM3_DIGEST_BYTES],
           const uint8_t *in0,
           const uint8_t *in1,
           const uint8_t *in2,
           const uint8_t *in3,
           const uint8_t *in4,
           const uint8_t *in5,
           const uint8_t *in6,
           const uint8_t *in7,
           size_t inlen) {
#if SM3_ENABLE_AVX2
    sm3x8_avx2(out0, out1, out2, out3, out4, out5, out6, out7,
               in0, in1, in2, in3, in4, in5, in6, in7, inlen);
    return;
#endif

    sm3(out0, in0, inlen);
    sm3(out1, in1, inlen);
    sm3(out2, in2, inlen);
    sm3(out3, in3, inlen);
    sm3(out4, in4, inlen);
    sm3(out5, in5, inlen);
    sm3(out6, in6, inlen);
    sm3(out7, in7, inlen);
}