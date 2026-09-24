#ifndef SPX_SM3_XOFX4_H
#define SPX_SM3_XOFX4_H

#include <stddef.h>
#include <stdint.h>

/**
 * 4-way parallel SM3 XOF (eXtendable Output Function)
 * 
 * Processes 4 messages in parallel using sm3x4, then truncates each
 * output to outlen bytes (typically SPX_N = 16 for Phoenix).
 * 
 * This is a convenience wrapper around sm3x4 for XOF use cases where
 * the output length is less than SM3_DIGEST_BYTES (32).
 * 
 * Note: On x86 AVX2, this uses scalar sm3x4 (4x scalar sm3).
 *       For true 4-way AVX2 parallel XOF, sm3x4_avx2 would need to be created.
 *       On ARM NEON, sm3x4 uses sm3x4_neon which is truly 4-way parallel.
 *
 * @param out0..3     Output buffers (at least outlen bytes each)
 * @param outlen      Output length per lane (typically SPX_N = 16)
 * @param in0..3     Input messages
 * @param inlen      Input length (identical for all 4 lanes)
 */
void sm3_xofx4(uint8_t *out0,
               uint8_t *out1,
               uint8_t *out2,
               uint8_t *out3,
               size_t outlen,
               const uint8_t *in0,
               const uint8_t *in1,
               const uint8_t *in2,
               const uint8_t *in3,
               size_t inlen);

#endif