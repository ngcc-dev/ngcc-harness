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
 * 8-way (AVX2) vectorised SM3 auxiliary functions.
 *
 * These produce byte-identical output to the scalar reference in
 * SHUTTLE/ref/auxfunc.c, but evaluate 8 *independent* instances at once by
 * packing one 32-bit SM3 word per lane of a 256-bit register.  This is the
 * shape the future digital-signature code needs: it expands 8 polynomials
 * / seeds in parallel while remaining KAT-compatible with the reference.
 *
 * Caller contract (NOTHING is validated):
 *   - All 8 lane pointers in msg[] / output[] / digest[] must be non-NULL
 * and independently readable/writable (no aliasing assumptions are made or
 *     required). To process fewer than 8 real instances, point the spare
 * lanes at any valid scratch buffer.
 *   - msg[k] must hold at least (msg_len_bits + 7) / 8 bytes; output[k] at
 *     least (output_len_bits + 7) / 8 bytes; digest[k] exactly 32 bytes.
 *   - msg_len_bits has no upper bound; output_len_bits must stay below the
 *     reference's documented 2^40 - 2^8 limit.
 */

#ifndef AUXFUNC_AVX2_H
#define AUXFUNC_AVX2_H

#define SM3_WAY_AVX2 8

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 8-way SM3 hash. Computes digest[k] = SM3(msg[k]) for k in 0..7.
///        All 8 messages share the same bit length (msg_len_bits); only
///        the message bytes differ per lane. Each digest is 32 bytes (256
///        bits).
/// @param[in]  msg          8 input message base addresses (one per lane)
/// @param[in]  msg_len_bits Total bits of each input message (shared)
/// @param[out] digest       8 output digest base addresses (32 bytes each)
/// @return 0 for success, others for error
int sm3hash_avx2(const unsigned char *const msg[SM3_WAY_AVX2],
                 unsigned long long msg_len_bits,
                 unsigned char *const digest[SM3_WAY_AVX2]);

/// @brief 8-way XOF (KDF-SM3). Computes, for k in 0..7,
///        output[k] = pseudoXOF(output_len_bits, msg[k], msg_len_bits).
///        output_len_bits and msg_len_bits are shared across the 8 lanes;
///        msg[] and output[] are per-lane. Byte-identical to running the
///        scalar reference pseudoXOF() 8 times.
/// @param[in]  output_len_bits Total bits (< 2^40 - 2^8) of each output
/// (shared)
/// @param[in]  msg             8 input message base addresses (one per
/// lane)
/// @param[in]  msg_len_bits    Total bits of each input message (shared)
/// @param[out] output          8 output base addresses (one per lane)
/// @return 0 for success, others for error
int pseudoXOF_avx2(unsigned long long output_len_bits,
                   const unsigned char *const msg[SM3_WAY_AVX2],
                   unsigned long long msg_len_bits,
                   unsigned char *const output[SM3_WAY_AVX2]);

#ifdef __cplusplus
}
#endif

#endif /* AUXFUNC_AVX2_H */
