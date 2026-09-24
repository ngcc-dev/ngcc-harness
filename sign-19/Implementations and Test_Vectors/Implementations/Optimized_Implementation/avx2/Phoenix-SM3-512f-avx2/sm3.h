#ifndef SM3_H
#define SM3_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SM3_BLOCK_BYTES 64
#define SM3_DIGEST_BYTES 32

void sm3(uint8_t out[SM3_DIGEST_BYTES], const uint8_t *in, size_t inlen);

void sm3x4(uint8_t out0[SM3_DIGEST_BYTES],
           uint8_t out1[SM3_DIGEST_BYTES],
           uint8_t out2[SM3_DIGEST_BYTES],
           uint8_t out3[SM3_DIGEST_BYTES],
           const uint8_t *in0,
           const uint8_t *in1,
           const uint8_t *in2,
           const uint8_t *in3,
           size_t inlen);

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
           size_t inlen);

int sm3_cpu_supports_avx2(void);
int sm3_cpu_supports_neon(void);
int sm3_x4_is_accelerated(void);
int sm3_x8_is_accelerated(void);

#ifdef __cplusplus
}
#endif

#endif
