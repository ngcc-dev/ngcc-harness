/*
The software is provided by the Institute of Commercial Cryptography
Standards (ICCS), and is used for algorithm submissions in the
Next-generation Commercial Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will
be uninterrupted or error-free in all cases. ICCS will take no
responsibility for the use of the software or the results thereof, if the
software is used for any other purposes.
*/

/*
 * 16-way (AVX512) vectorised SM3 auxiliary functions.
 *
 * Identical in spirit to the AVX2 8-way code, but packs one 32-bit SM3
 * word per lane of a 512-bit register, evaluating 16 independent instances
 * at once. The output is byte-identical to running the scalar reference 16
 * times.
 *
 * Caller contract (NOTHING is validated):
 *   - All 16 lane pointers in msg[] / output[] / digest[] must be non-NULL
 *     and independently readable/writable. To process fewer than 16 real
 *     instances, point the spare lanes at any valid scratch buffer.
 *   - msg[k] must hold at least (msg_len_bits + 7) / 8 bytes; output[k] at
 *     least (output_len_bits + 7) / 8 bytes; digest[k] exactly 32 bytes.
 *   - msg_len_bits has no upper bound; output_len_bits must stay below the
 *     reference's documented 2^40 - 2^8 limit.
 */

#ifndef AUXFUNC_AVX512_H
#define AUXFUNC_AVX512_H

#define SM3_WAY_AVX512 16

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 16-way SM3 hash. digest[k] = SM3(msg[k]) for k in 0..15. All 16
///        messages share msg_len_bits; only the bytes differ per lane.
/// @param[in]  msg          16 input message base addresses (one per lane)
/// @param[in]  msg_len_bits Total bits of each input message (shared)
/// @param[out] digest       16 output digest base addresses (32 bytes
/// each)
/// @return 0 for success, others for error
int sm3hash_avx512(const unsigned char *const msg[SM3_WAY_AVX512],
                   unsigned long long msg_len_bits,
                   unsigned char *const digest[SM3_WAY_AVX512]);

/// @brief 16-way XOF (KDF-SM3). For k in 0..15,
///        output[k] = pseudoXOF(output_len_bits, msg[k], msg_len_bits).
///        output_len_bits and msg_len_bits are shared; msg[]/output[] are
///        per-lane. Byte-identical to the scalar reference pseudoXOF().
/// @param[in]  output_len_bits Total bits (< 2^40 - 2^8) of each output
/// (shared)
/// @param[in]  msg             16 input message base addresses (one per
/// lane)
/// @param[in]  msg_len_bits    Total bits of each input message (shared)
/// @param[out] output          16 output base addresses (one per lane)
/// @return 0 for success, others for error
int pseudoXOF_avx512(unsigned long long output_len_bits,
                     const unsigned char *const msg[SM3_WAY_AVX512],
                     unsigned long long msg_len_bits,
                     unsigned char *const output[SM3_WAY_AVX512]);

#ifdef __cplusplus
}
#endif

#endif /* AUXFUNC_AVX512_H */
