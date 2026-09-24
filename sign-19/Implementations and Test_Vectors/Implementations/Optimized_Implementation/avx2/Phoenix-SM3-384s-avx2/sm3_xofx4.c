/**
 * SM3 XOF with 4-way parallel processing
 * 
 * This implementation uses sm3x4 internally (which may use NEON on ARM
 * or fall back to scalar on x86), then truncates each output to
 * the requested outlen bytes.
 * 
 * For true 4-way x86 AVX2 parallel XOF, a dedicated sm3x4_avx2
 * using 4-lane state manipulation would be needed.
 */

#include <string.h>

#include "sm3.h"
#include "sm3_xofx4.h"
#include "params.h"

void sm3_xofx4(uint8_t *out0,
               uint8_t *out1,
               uint8_t *out2,
               uint8_t *out3,
               size_t outlen,
               const uint8_t *in0,
               const uint8_t *in1,
               const uint8_t *in2,
               const uint8_t *in3,
               size_t inlen)
{
    /* SM3 produces 32-byte output, but we typically only need SPX_N bytes */
    uint8_t temp0[SM3_DIGEST_BYTES];
    uint8_t temp1[SM3_DIGEST_BYTES];
    uint8_t temp2[SM3_DIGEST_BYTES];
    uint8_t temp3[SM3_DIGEST_BYTES];

    /* Compute full SM3 hash for each lane */
    sm3x4(temp0, temp1, temp2, temp3, in0, in1, in2, in3, inlen);

    /* Truncate to requested output length */
    memcpy(out0, temp0, outlen);
    memcpy(out1, temp1, outlen);
    memcpy(out2, temp2, outlen);
    memcpy(out3, temp3, outlen);
}