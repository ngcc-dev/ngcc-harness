/* Phoenix-SHAKE-128f FIPS202 interface
 *
 * This header defines the SHAKE256 and SHA3 hash function interfaces
 * optimized for Phoenix signature scheme using 4-way AVX2 parallelism.
 *
 * Key features:
 * - SHAKE128/SHAKE256 extendable-output functions (XOF)
 * - SHA3-256/SHA3-512 hash functions
 * - Incremental and block-based operation modes
 * - Optimized for KeccakP1600 4-lane parallel permutation
 *
 * Rate constants (in bytes):
 *   SHAKE128_RATE: 168
 *   SHAKE256_RATE: 136
 *   SHA3_256_RATE:  136
 *   SHA3_512_RATE:  72
 */

#ifndef PH_FIPS202_H
#define PH_FIPS202_H

#include <stddef.h>
#include <stdint.h>

/* SHAKE128 rate (1600 - 256)/8 = 168 bytes */
#define SPX_SHAKE128_RATE 168
/* SHAKE256 rate (1600 - 512)/8 = 136 bytes */
#define SPX_SHAKE256_RATE 136
/* SHA3-256 rate (1600 - 512)/8 = 136 bytes */
#define PH_SHA3_256_RATE 136
/* SHA3-512 rate (1600 - 1024)/8 = 72 bytes */
#define PH_SHA3_512_RATE 72

/* For compatibility, keep original macro names as aliases */
#define SHAKE128_RATE SPX_SHAKE128_RATE
#define SHAKE256_RATE SPX_SHAKE256_RATE
#define SHA3_256_RATE PH_SHA3_256_RATE
#define SHA3_512_RATE PH_SHA3_512_RATE


/* ===== SHAKE128 XOF ===== */

/**
 * @brief Absorb input data into SHAKE128 state (non-incremental)
 *
 * @param state Keccak state array pointer
 * @param input Input data pointer
 * @param inlen Length of input in bytes
 */
void shake128_absorb(uint64_t *state, const uint8_t *input, size_t inlen);

/**
 * @brief Squeeze output blocks from SHAKE128 state
 *
 * @param output Output buffer pointer
 * @param nblocks Number of SHAKE128_RATE-sized blocks to squeeze
 * @param state Keccak state array pointer
 */
void shake128_squeezeblocks(uint8_t *output, size_t nblocks, uint64_t *state);

/**
 * @brief Initialize incremental SHAKE128 state
 *
 * @param s_inc Incremental state pointer (26-element array)
 */
void shake128_inc_init(uint64_t *s_inc);

/**
 * @brief Incrementally absorb bytes into SHAKE128 state
 *
 * @param s_inc Incremental state pointer
 * @param input Input data pointer
 * @param inlen Number of bytes to absorb
 */
void shake128_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen);

/**
 * @brief Finalize SHAKE128 incremental absorption
 *
 * @param s_inc Incremental state pointer
 */
void shake128_inc_finalize(uint64_t *s_inc);

/**
 * @brief Incrementally squeeze bytes from SHAKE128 state
 *
 * @param output Output buffer pointer
 * @param outlen Number of bytes to squeeze
 * @param s_inc Incremental state pointer
 */
void shake128_inc_squeeze(uint8_t *output, size_t outlen, uint64_t *s_inc);

/**
 * @brief SHAKE128 extendable-output function (non-incremental)
 *
 * @param output Output buffer pointer
 * @param outlen Desired output length in bytes
 * @param input Input data pointer
 * @param inlen Length of input in bytes
 */
void shake128(uint8_t *output, size_t outlen,
              const uint8_t *input, size_t inlen);


/* ===== SHAKE256 XOF ===== */

/**
 * @brief Absorb input data into SHAKE256 state (non-incremental)
 *
 * @param state Keccak state array pointer
 * @param input Input data pointer
 * @param inlen Length of input in bytes
 */
void shake256_absorb(uint64_t *state, const uint8_t *input, size_t inlen);

/**
 * @brief Squeeze output blocks from SHAKE256 state
 *
 * @param output Output buffer pointer
 * @param nblocks Number of SHAKE256_RATE-sized blocks to squeeze
 * @param state Keccak state array pointer
 */
void shake256_squeezeblocks(uint8_t *output, size_t nblocks, uint64_t *state);

/**
 * @brief Initialize incremental SHAKE256 state
 *
 * @param s_inc Incremental state pointer (26-element array)
 */
void shake256_inc_init(uint64_t *s_inc);

/**
 * @brief Incrementally absorb bytes into SHAKE256 state
 *
 * @param s_inc Incremental state pointer
 * @param input Input data pointer
 * @param inlen Number of bytes to absorb
 */
void shake256_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen);

/**
 * @brief Finalize SHAKE256 incremental absorption
 *
 * @param s_inc Incremental state pointer
 */
void shake256_inc_finalize(uint64_t *s_inc);

/**
 * @brief Incrementally squeeze bytes from SHAKE256 state
 *
 * @param output Output buffer pointer
 * @param outlen Number of bytes to squeeze
 * @param s_inc Incremental state pointer
 */
void shake256_inc_squeeze(uint8_t *output, size_t outlen, uint64_t *s_inc);

/**
 * @brief SHAKE256 extendable-output function (non-incremental)
 *
 * @param output Output buffer pointer
 * @param outlen Desired output length in bytes
 * @param input Input data pointer
 * @param inlen Length of input in bytes
 */
void shake256(uint8_t *output, size_t outlen,
              const uint8_t *input, size_t inlen);


/* ===== SHA3-256 Hash ===== */

/**
 * @brief Initialize incremental SHA3-256 state
 *
 * @param s_inc Incremental state pointer (26-element array)
 */
void sha3_256_inc_init(uint64_t *s_inc);

/**
 * @brief Incrementally absorb bytes into SHA3-256 state
 *
 * @param s_inc Incremental state pointer
 * @param input Input data pointer
 * @param inlen Number of bytes to absorb
 */
void sha3_256_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen);

/**
 * @brief Finalize SHA3-256 and produce hash output
 *
 * @param output Output buffer (32 bytes)
 * @param s_inc Incremental state pointer
 */
void sha3_256_inc_finalize(uint8_t *output, uint64_t *s_inc);

/**
 * @brief SHA3-256 hash function (non-incremental)
 *
 * @param output Output buffer (32 bytes)
 * @param input Input data pointer
 * @param inlen Length of input in bytes
 */
void sha3_256(uint8_t *output, const uint8_t *input, size_t inlen);


/* ===== SHA3-512 Hash ===== */

/**
 * @brief Initialize incremental SHA3-512 state
 *
 * @param s_inc Incremental state pointer (26-element array)
 */
void sha3_512_inc_init(uint64_t *s_inc);

/**
 * @brief Incrementally absorb bytes into SHA3-512 state
 *
 * @param s_inc Incremental state pointer
 * @param input Input data pointer
 * @param inlen Number of bytes to absorb
 */
void sha3_512_inc_absorb(uint64_t *s_inc, const uint8_t *input, size_t inlen);

/**
 * @brief Finalize SHA3-512 and produce hash output
 *
 * @param output Output buffer (64 bytes)
 * @param s_inc Incremental state pointer
 */
void sha3_512_inc_finalize(uint8_t *output, uint64_t *s_inc);

/**
 * @brief SHA3-512 hash function (non-incremental)
 *
 * @param output Output buffer (64 bytes)
 * @param input Input data pointer
 * @param inlen Length of input in bytes
 */
void sha3_512(uint8_t *output, const uint8_t *input, size_t inlen);

#endif /* PH_FIPS202_H */
