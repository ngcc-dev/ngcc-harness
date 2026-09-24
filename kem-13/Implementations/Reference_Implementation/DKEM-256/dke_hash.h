/*
 * DKE Hash Abstraction Layer.
 *
 * DKE_HASH=0: SM3 + pseudoXOF (original DKE spec, Chinese standard)
 * DKE_HASH=2: SHAKE128 XOF + SHA3-256 H(pk) + SHA3-512 G + SHAKE256 KDF
 *             (full ML-KEM hash suite, protocol-compatible with ML-KEM)
 *
 * Function routing (HASH=0 / HASH=2):
 *   dke_xof        -- XOF for matrix/noise generation (SM3-iter / SHAKE128)
 *   dke_hash_pk    -- H(pk): fixed 32B hash of public key (pseudoXOF / SHA3-256)
 *   dke_hash_g     -- G(seed): seed expansion to 64B (pseudoXOF / SHA3-512)
 *   dke_xof_kdf    -- KDF for CCA kr derivation (pseudoXOF / SHAKE256)
 */
#ifndef DKE_HASH_H
#define DKE_HASH_H

#include <stdint.h>



#ifdef __cplusplus
extern "C" {
#endif

/* XOF: variable-length output. Used for matrix generation and noise sampling. */
int dke_xof(unsigned long long output_len_bits,
            const unsigned char *msg,
            unsigned long long msg_len_bits,
            unsigned char *output);

/* Fixed 256-bit hash (bit-length interface). */
void dke_hash256(const unsigned char *msg,
                 unsigned long long msg_len_bits,
                 unsigned char *digest);

/* Fixed 256-bit hash (byte-length interface). */
void dke_hash256_bytes(const unsigned char *msg,
                       unsigned long long msg_bytes,
                       unsigned char *digest);

/*
 * H(pk): hash public key → 32 bytes.
 *   DKE_HASH=0: pseudoXOF(pk) → 32B
 *   DKE_HASH=2: SHA3-256(pk)  → 32B  (ML-KEM hash_h)
 * Must be used consistently in keygen, enc, and dec.
 */
void dke_hash_pk(const unsigned char *pk,
                 unsigned long long pk_bytes,
                 unsigned char *digest);

/*
 * G(seed): expand seed → 64 bytes (rho || sigma for keygen).
 *   DKE_HASH=0: pseudoXOF(seed) → 64B
 *   DKE_HASH=2: SHA3-512(seed)  → 64B  (ML-KEM hash_g)
 * Declared noinline to prevent MSVC from inlining sha3_512's large stack
 * frame into callers that use AVX2 locals requiring 32-byte alignment.
 */
#if defined(_MSC_VER)
__declspec(noinline)
#endif
void dke_hash_g(const unsigned char *seed,
                unsigned long long seed_bytes,
                unsigned char *out64);

/*
 * KDF for CCA kr derivation: expand (coins || H(pk)) → kr.
 *   DKE_HASH=0: pseudoXOF(in) → out_bytes
 *   DKE_HASH=2: SHAKE256(in)  → out_bytes  (ML-KEM KDF)
 */
void dke_xof_kdf(unsigned char *out,
                 unsigned long long out_bytes,
                 const unsigned char *in,
                 unsigned long long in_bytes);

/* Extended hash: 512/768/1024-bit output. */
int dke_hash_extended(int digest_len_bits,
                      const unsigned char *msg,
                      unsigned long long msg_len_bits,
                      unsigned char *digest);

/* HMAC with the DKE fixed key. */
int dke_hmac_fixed_key(const unsigned char *msg,
                       unsigned long long msg_len_bits,
                       unsigned char *mac);

/* Normalize bit-string (clear unused bits in last byte). */
void dke_normalize(unsigned char *input, unsigned long long total_bits);

#ifdef __cplusplus
}
#endif

#endif /* DKE_HASH_H */
