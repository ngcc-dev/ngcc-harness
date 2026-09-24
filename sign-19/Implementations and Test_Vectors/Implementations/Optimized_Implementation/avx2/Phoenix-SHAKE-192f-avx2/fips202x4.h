/* Phoenix-SHAKE-128f 4-way parallel SHAKE interface
 *
 * This header defines AVX2 optimized 4-lane parallel SHAKE128/256
 * extendable-output functions for Phoenix signature scheme.
 *
 * Key features:
 * - 4-way parallel SHAKE128/SHAKE256 XOF
 * - Optimized for KeccakP1600times4_PermuteAll_24rounds
 * - High-throughput signing operations support
 */

#ifndef PH_FIPS202X4_H
#define PH_FIPS202X4_H

#include <immintrin.h>

/*************************************************
 * @brief SHAKE128 XOF with 4-way parallel processing
 *
 * Processes 4 independent SHAKE128 computations in parallel,
 * each with identical input length but different messages.
 *
 * @param[out] out0..3  Output buffer pointers (outlen bytes each)
 * @param[in]  outlen   Output length per lane in bytes
 * @param[in]  in0..3   Input message pointers
 * @param[in]  inlen    Input length (identical for all 4 lanes)
 **************************************************/
void shake128x4(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3, unsigned long long outlen,
                unsigned char *in0,
                unsigned char *in1,
                unsigned char *in2,
                unsigned char *in3, unsigned long long inlen);

/*************************************************
 * @brief SHAKE256 XOF with 4-way parallel processing
 *
 * Processes 4 independent SHAKE256 computations in parallel,
 * each with identical input length but different messages.
 *
 * @param[out] out0..3  Output buffer pointers (outlen bytes each)
 * @param[in]  outlen   Output length per lane in bytes
 * @param[in]  in0..3   Input message pointers
 * @param[in]  inlen    Input length (identical for all 4 lanes)
 **************************************************/
void shake256x4(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3, unsigned long long outlen,
                unsigned char *in0,
                unsigned char *in1,
                unsigned char *in2,
                unsigned char *in3, unsigned long long inlen);

#endif /* PH_FIPS202X4_H */
