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
 * 8-way (AVX2) SM3 Hash-DRBG.
 *
 * Same DRBG as SHUTTLE/ref/drng.c (instantiate + generate, SM3 kernel),
 * but run for 8 independent instances at once: the SM3 calls inside every
 * step hash per-lane data of identical length, so they map directly onto
 * the 8-way sm3hash_avx2() core. Output is byte-identical to running the
 * scalar init_random_number()/get_random_number() once per lane.
 *
 * By the N-way contract: seed_len_bytes / random_number_len_bits are
 * SHARED across the 8 lanes; seed[] / random_number[] are per-lane.
 *
 * Caller contract (NOTHING is validated): all 8 lane pointers must be
 * non-NULL; seed[k] holds >= seed_len_bytes bytes; random_number[k] holds
 * >= (random_number_len_bits + 7) / 8 bytes.
 */

#ifndef DRNG_AVX2_H
#define DRNG_AVX2_H

#include "drng.h" /* SEEDLEN, scalar DRNG_ctx (resolved via -I../ref) */

#define DRNG_WAY_AVX2 8

#ifdef __cplusplus
extern "C" {
#endif

/// 8-lane DRBG working state (one independent instance per lane).
typedef struct {
    unsigned char V[DRNG_WAY_AVX2][SEEDLEN];
    unsigned char C[DRNG_WAY_AVX2][SEEDLEN];
    unsigned char reseed_counter[DRNG_WAY_AVX2][SEEDLEN];
} DRNG_ctx_avx2;

/// @brief Instantiate 8 DRBG instances. drng->{V,C,reseed_counter}[k] are
///        derived from seed[k] exactly as the scalar init_random_number().
/// @param[out] drng           8-lane DRBG context
/// @param[in]  seed           8 seed base addresses (one per lane)
/// @param[in]  seed_len_bytes Valid bytes of each seed (shared)
/// @return 0 for success, others for error
int init_random_number_avx2(DRNG_ctx_avx2 *drng,
                            const unsigned char *const seed[DRNG_WAY_AVX2],
                            unsigned long long seed_len_bytes);

/// @brief Generate pseudo-random bytes for 8 instances and advance state.
/// @param[in,out] drng                   Instantiated 8-lane DRBG
/// @param[out]    random_number          8 output base addresses (per
/// lane)
/// @param[in]     random_number_len_bits Valid bits of each output
/// (shared)
/// @return 0 for success, others for error
int get_random_number_avx2(
    DRNG_ctx_avx2 *drng, unsigned char *const random_number[DRNG_WAY_AVX2],
    unsigned long long random_number_len_bits);

#ifdef __cplusplus
}
#endif

#endif /* DRNG_AVX2_H */
