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
 * 16-way (AVX512) SM3 Hash-DRBG.
 *
 * Same DRBG as SHUTTLE/ref/drng.c, run for 16 independent instances at
 * once on top of the 16-way sm3hash_avx512() core. Output is
 * byte-identical to running the scalar
 * init_random_number()/get_random_number() once per lane.
 *
 * seed_len_bytes / random_number_len_bits are SHARED across the 16 lanes;
 * seed[] / random_number[] are per-lane.
 *
 * Caller contract (NOTHING is validated): all 16 lane pointers must be
 * non-NULL; seed[k] holds >= seed_len_bytes bytes; random_number[k] holds
 * >= (random_number_len_bits + 7) / 8 bytes.
 */

#ifndef DRNG_AVX512_H
#define DRNG_AVX512_H

#include "drng.h" /* SEEDLEN, scalar DRNG_ctx (resolved via -I../ref) */

#define DRNG_WAY_AVX512 16

#ifdef __cplusplus
extern "C" {
#endif

/// 16-lane DRBG working state (one independent instance per lane).
typedef struct {
    unsigned char V[DRNG_WAY_AVX512][SEEDLEN];
    unsigned char C[DRNG_WAY_AVX512][SEEDLEN];
    unsigned char reseed_counter[DRNG_WAY_AVX512][SEEDLEN];
} DRNG_ctx_avx512;

/// @brief Instantiate 16 DRBG instances from per-lane seeds.
int init_random_number_avx512(
    DRNG_ctx_avx512 *drng,
    const unsigned char *const seed[DRNG_WAY_AVX512],
    unsigned long long seed_len_bytes);

/// @brief Generate pseudo-random bytes for 16 instances and advance state.
int get_random_number_avx512(
    DRNG_ctx_avx512 *drng,
    unsigned char *const random_number[DRNG_WAY_AVX512],
    unsigned long long random_number_len_bits);

#ifdef __cplusplus
}
#endif

#endif /* DRNG_AVX512_H */
