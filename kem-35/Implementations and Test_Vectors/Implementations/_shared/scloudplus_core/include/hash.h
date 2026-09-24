/**
 * @file hash.h
 * @brief Common hash/XOF interface for all Scloud+ families.
 *
 * This header is intentionally stable across AES, SHAKE, and SM3 builds.  The
 * selected source files and compile definitions decide which family-specific
 * implementation is linked; callers do not include different hash headers for
 * different instances.
 */

#ifndef SCLOUDPLUS_HASH_H
#define SCLOUDPLUS_HASH_H

#include <stddef.h>
#include <stdint.h>

#define SHAKE128_RATE 168
#define SHAKE256_RATE 136
#define SHA3_256_RATE 136
#define SHA3_512_RATE 72
#define SM3_BOUNDED_XOF_RATE 32
#define SM3_BOUNDED_XOF_CACHE_BYTES 2048

typedef struct
{
	uint64_t s[25];
	unsigned int pos;
} keccak_state;

typedef struct
{
	uint8_t msg[128];
	size_t msg_len;
	size_t generated;
#if defined(SCLOUDPLUS_AVX2_FAMILY_SM3)
	unsigned int base_digest[8];
	uint8_t tail[64];
	size_t tail_len;
	unsigned long long total_bitlen;
	unsigned int precomputed_ready;
#endif
	uint8_t cache[SM3_BOUNDED_XOF_CACHE_BYTES];
	size_t cache_pos;
	size_t cache_len;
	int status;
} sm3_bounded_xof_state;

/** Initialize an incremental SHAKE128 context. */
void shake128_init(keccak_state *state);
/** Absorb bytes into an incremental SHAKE128 context. */
void shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
/** Finalize an incremental SHAKE128 absorb phase. */
void shake128_finalize(keccak_state *state);
/** Squeeze bytes from an incremental SHAKE128 context. */
void shake128_squeeze(uint8_t *out, size_t outlen, keccak_state *state);
/** Absorb and finalize one complete SHAKE128 input. */
void shake128_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen);
/** Squeeze full SHAKE128 rate blocks from an incremental context. */
void shake128_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state);
/** Initialize an incremental SHAKE256 context. */
void shake256_init(keccak_state *state);
/** Absorb bytes into an incremental SHAKE256 context. */
void shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
/** Finalize an incremental SHAKE256 absorb phase. */
void shake256_finalize(keccak_state *state);
/** Squeeze bytes from an incremental SHAKE256 context. */
void shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state);
/** Absorb and finalize one complete SHAKE256 input. */
void shake256_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen);
/** Squeeze full SHAKE256 rate blocks from an incremental context. */
void shake256_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state);
/** One-shot SHAKE128 XOF. */
void shake128(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);
/** One-shot SHAKE256 XOF. */
void shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/** Initialize an incremental bounded SM3 expansion context. */
void sm3_bounded_xof_init(sm3_bounded_xof_state *state);
/** Absorb bytes into an incremental bounded SM3 expansion context. */
int sm3_bounded_xof_absorb(sm3_bounded_xof_state *state, const uint8_t *in, size_t inlen);
/** Finalize an incremental bounded SM3 expansion absorb phase. */
void sm3_bounded_xof_finalize(sm3_bounded_xof_state *state);
/** Squeeze bytes from an incremental bounded SM3 expansion context. */
int sm3_bounded_xof_squeeze(uint8_t *out, size_t outlen, sm3_bounded_xof_state *state);
/** Absorb and finalize one complete bounded SM3 expansion input. */
int sm3_bounded_xof_absorb_once(sm3_bounded_xof_state *state, const uint8_t *in, size_t inlen);
/** Squeeze full bounded SM3 expansion rate blocks from an incremental context. */
int sm3_bounded_xof_squeezeblocks(uint8_t *out, size_t nblocks,
								  sm3_bounded_xof_state *state);
/** One-shot bounded SM3 expansion. */
int sm3_bounded_xof(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);
/** Bounded SM3 expansion specialized for A-row input SeedA || LE32(row). */
int sm3_bounded_xof_seed_row(uint8_t *out, size_t outlen, const uint8_t *seed,
							  uint32_t row);
/** Bounded SM3 expansion specialized for eight consecutive A rows. */
int sm3_bounded_xof8_a_rows(uint8_t *out[8], size_t outlen,
							const uint8_t *seed, uint32_t row_start);
/** One-shot SM3-256 hash. */
void sm3_256(uint8_t out[32], const uint8_t *in, size_t inlen);

/**
 * @brief Family-selected expansion primitive F.
 *
 * Used to expand externally sampled coins into `SeedA`, `r1`, and `r2`.
 * `SeedA` is always 128 bits; the remaining derived strings use the public
 * level-selected security material length from `parameters.h`.
 */
void scloudplus_F(unsigned char *output, unsigned long long outlen,
				  const unsigned char *input, unsigned long long inlen);
/**
 * @brief Family-selected KDF primitive K for deriving the final shared secret.
 */
void scloudplus_K(unsigned char *output, unsigned long long outlen,
				  const unsigned char *input, unsigned long long inlen);
/**
 * @brief Family-selected hash H used on public keys and transcript material.
 *
 * The output length is `scloudplus_hash_bytes`: 512 bits for levels up to 256,
 * 768 bits for level 384, and 1024 bits for level 512.  SM3 builds call the
 * submitted `pseudohash` interface for this function.
 */
void scloudplus_H(unsigned char *output, const unsigned char *input,
				  unsigned long long inlen);
/**
 * @brief Family-selected expansion primitive G used by the FO transform.
 *
 * The output is parsed as `r || k`, where both halves have length
 * `scloudplus_hash_bytes`.
 */
void scloudplus_G(unsigned char *output, const unsigned char *input,
				  unsigned long long inlen);

#endif
