#ifndef SM3_INTERNAL_H
#define SM3_INTERNAL_H

#include "sm3.h"

void sm3x4_neon(uint8_t out0[SM3_DIGEST_BYTES],
                uint8_t out1[SM3_DIGEST_BYTES],
                uint8_t out2[SM3_DIGEST_BYTES],
                uint8_t out3[SM3_DIGEST_BYTES],
                const uint8_t *in0,
                const uint8_t *in1,
                const uint8_t *in2,
                const uint8_t *in3,
                size_t inlen);

void sm3x8_avx2(uint8_t out0[SM3_DIGEST_BYTES],
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
                size_t inlen);

#endif
