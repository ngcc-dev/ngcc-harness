/*
 * DKE Hash Abstraction Layer implementation.
 *
 * DKE_HASH=0: SM3 + pseudoXOF (original DKE spec)
 * DKE_HASH=1: SHAKE128 for all operations
 * DKE_HASH=2: SHAKE128 XOF + SHA3-256 H(pk) + SHA3-512 G + SHAKE256 KDF
 */
#include "dke_hash.h"
#include "parameters.h"

/* ================================================================
 * DKE_HASH=0: SM3 backend (original DKE spec, unchanged)
 * ================================================================ */
#if DKE_HASH == 0
#include "sm3.h"
#include "dke_sm3.h"
#include "auxfunc.h"

int dke_xof(unsigned long long output_len_bits,
            const unsigned char *msg,
            unsigned long long msg_len_bits,
            unsigned char *output) {
    return pseudoXOF(output_len_bits, msg, msg_len_bits, output);
}

void dke_hash256(const unsigned char *msg,
                 unsigned long long msg_len_bits,
                 unsigned char *digest) {
    /* req 3.4(5): use the official ICCS auxiliary function sm3hash (256-bit SM3). */
    sm3hash(256, msg, msg_len_bits, digest);
}

void dke_hash256_bytes(const unsigned char *msg,
                       unsigned long long msg_bytes,
                       unsigned char *digest) {
    sm3hash(256, msg, msg_bytes * 8, digest);
}

void dke_hash_pk(const unsigned char *pk,
                 unsigned long long pk_bytes,
                 unsigned char *digest) {
    /* SM3 mode: pseudoXOF → DKE_SSBYTES */
    pseudoXOF(DKE_SSBYTES * 8, pk, pk_bytes * 8, digest);
}

void dke_hash_g(const unsigned char *seed,
                unsigned long long seed_bytes,
                unsigned char *out64) {
    /* SM3 mode: pseudoXOF(64B, seed) — same as original dke_xof(2*SEEDBYTES*8, ...) */
    pseudoXOF(seed_bytes * 8 * 2, seed, seed_bytes * 8, out64);
}

void dke_xof_kdf(unsigned char *out,
                 unsigned long long out_bytes,
                 const unsigned char *in,
                 unsigned long long in_bytes) {
    /* SM3 mode: pseudoXOF — same as original dke_xof */
    pseudoXOF(out_bytes * 8, in, in_bytes * 8, out);
}

int dke_hash_extended(int digest_len_bits,
                      const unsigned char *msg,
                      unsigned long long msg_len_bits,
                      unsigned char *digest) {
    return pseudohash(digest_len_bits, msg, msg_len_bits, digest);
}

int dke_hmac_fixed_key(const unsigned char *msg,
                       unsigned long long msg_len_bits,
                       unsigned char *mac) {
    return dke_hmac_with_fixed_key(msg, msg_len_bits, mac);
}

void dke_normalize(unsigned char *input, unsigned long long total_bits) {
    dke_sm3_normalize(input, total_bits);
}

/* ================================================================
 * DKE_HASH=1: SHAKE128 for all operations (fast benchmark)
 * ================================================================ */
#elif DKE_HASH == 1
#include "fips202.h"
#include <string.h>

int dke_xof(unsigned long long output_len_bits,
            const unsigned char *msg,
            unsigned long long msg_len_bits,
            unsigned char *output) {
    unsigned long long msg_bytes = (msg_len_bits + 7) / 8;
    unsigned long long out_bytes = (output_len_bits + 7) / 8;
    shake128(output, out_bytes, msg, msg_bytes);
    if (output_len_bits % 8 != 0) {
        unsigned long long last = output_len_bits / 8;
        unsigned long long rem  = output_len_bits % 8;
        output[last] &= ~((1 << (8 - rem)) - 1);
    }
    return 0;
}

void dke_hash256(const unsigned char *msg,
                 unsigned long long msg_len_bits,
                 unsigned char *digest) {
    sha3_256(digest, msg, (msg_len_bits + 7) / 8);
}

void dke_hash256_bytes(const unsigned char *msg,
                       unsigned long long msg_bytes,
                       unsigned char *digest) {
    sha3_256(digest, msg, msg_bytes);
}

void dke_hash_pk(const unsigned char *pk,
                 unsigned long long pk_bytes,
                 unsigned char *digest) {
    /* HASH=1: SHAKE128(pk) → DKE_SSBYTES */
    shake128(digest, DKE_SSBYTES, pk, pk_bytes);
}

void dke_hash_g(const unsigned char *seed,
                unsigned long long seed_bytes,
                unsigned char *out64) {
    /* HASH=1: SHAKE128(seed) → 2*seed_bytes.
     * Copy input to avoid potential aliasing when in==out. */
    uint8_t tmp[64];  /* max seed_bytes is 64 (DKE-512) */
    memcpy(tmp, seed, seed_bytes);
    shake128(out64, seed_bytes * 2, tmp, seed_bytes);
}

void dke_xof_kdf(unsigned char *out,
                 unsigned long long out_bytes,
                 const unsigned char *in,
                 unsigned long long in_bytes) {
    /* HASH=1: SHAKE128 (same as original dke_xof) */
    shake128(out, out_bytes, in, in_bytes);
}

int dke_hash_extended(int digest_len_bits,
                      const unsigned char *msg,
                      unsigned long long msg_len_bits,
                      unsigned char *digest) {
    shake256(digest, (unsigned long long)digest_len_bits / 8,
             msg, (msg_len_bits + 7) / 8);
    return 0;
}

int dke_hmac_fixed_key(const unsigned char *msg,
                       unsigned long long msg_len_bits,
                       unsigned char *mac) {
    sha3_256(mac, msg, (msg_len_bits + 7) / 8);
    return 0;
}

void dke_normalize(unsigned char *input, unsigned long long total_bits) {
    unsigned long long full_bytes = total_bits / 8;
    unsigned long long rem = total_bits % 8;
    if (rem > 0)
        input[full_bytes] &= ~((1 << (8 - rem)) - 1);
}

/* ================================================================
 * DKE_HASH=2: Full ML-KEM hash suite
 *   XOF (matrix/noise): SHAKE128
 *   H(pk):              SHA3-256
 *   G(seed):            SHA3-512
 *   KDF (kr):           SHAKE256
 * ================================================================ */
#elif DKE_HASH == 2
#include "fips202.h"
#include <string.h>

int dke_xof(unsigned long long output_len_bits,
            const unsigned char *msg,
            unsigned long long msg_len_bits,
            unsigned char *output) {
    /* Matrix/noise XOF: SHAKE128 (same as HASH=1) */
    unsigned long long msg_bytes = (msg_len_bits + 7) / 8;
    unsigned long long out_bytes = (output_len_bits + 7) / 8;
    shake128(output, out_bytes, msg, msg_bytes);
    if (output_len_bits % 8 != 0) {
        unsigned long long last = output_len_bits / 8;
        unsigned long long rem  = output_len_bits % 8;
        output[last] &= ~((1 << (8 - rem)) - 1);
    }
    return 0;
}

void dke_hash256(const unsigned char *msg,
                 unsigned long long msg_len_bits,
                 unsigned char *digest) {
    sha3_256(digest, msg, (msg_len_bits + 7) / 8);
}

void dke_hash256_bytes(const unsigned char *msg,
                       unsigned long long msg_bytes,
                       unsigned char *digest) {
    sha3_256(digest, msg, msg_bytes);
}

void dke_hash_pk(const unsigned char *pk,
                 unsigned long long pk_bytes,
                 unsigned char *digest) {
    /* ML-KEM hash_h: SHA3-256 for DKE-128/256, SHAKE256 for DKE-512 */
    if (DKE_SSBYTES == 32) {
        sha3_256(digest, pk, (size_t)pk_bytes);
    } else {
        shake256(digest, DKE_SSBYTES, pk, (size_t)pk_bytes);
    }
}

#if defined(_MSC_VER)
__declspec(noinline)
#endif
void dke_hash_g(const unsigned char *seed,
                unsigned long long seed_bytes,
                unsigned char *out64) {
    /* ML-KEM hash_g: SHA3-512(seed) → 64B for DKE-128/256 (seed_bytes=32).
     * DKE-512 (seed_bytes=64): SHAKE128(seed) → 128B.
     * Uses stack buffers (no malloc) so this works on bare-metal Cortex-M4.
     * Max stack: 192 bytes (DKE-512 path: 64 + 128). */
    if (seed_bytes == 32) {
        uint8_t tmp[32 + 64];
        memcpy(tmp, seed, 32);
        sha3_512(tmp + 32, tmp, 32);
        memcpy(out64, tmp + 32, 64);
    } else {
        /* seed_bytes == 64 (DKE-512): output is 128 bytes */
        uint8_t tmp[64 + 128];
        memcpy(tmp, seed, (size_t)seed_bytes);
        shake128(tmp + seed_bytes, seed_bytes * 2, tmp, (size_t)seed_bytes);
        memcpy(out64, tmp + seed_bytes, (size_t)(seed_bytes * 2));
    }
}

void dke_xof_kdf(unsigned char *out,
                 unsigned long long out_bytes,
                 const unsigned char *in,
                 unsigned long long in_bytes) {
    /* ML-KEM KDF: SHAKE256 */
    shake256(out, (size_t)out_bytes, in, (size_t)in_bytes);
}

int dke_hash_extended(int digest_len_bits,
                      const unsigned char *msg,
                      unsigned long long msg_len_bits,
                      unsigned char *digest) {
    shake256(digest, (unsigned long long)digest_len_bits / 8,
             msg, (msg_len_bits + 7) / 8);
    return 0;
}

int dke_hmac_fixed_key(const unsigned char *msg,
                       unsigned long long msg_len_bits,
                       unsigned char *mac) {
    sha3_256(mac, msg, (msg_len_bits + 7) / 8);
    return 0;
}

void dke_normalize(unsigned char *input, unsigned long long total_bits) {
    unsigned long long full_bytes = total_bits / 8;
    unsigned long long rem = total_bits % 8;
    if (rem > 0)
        input[full_bytes] &= ~((1 << (8 - rem)) - 1);
}

#else
#error "DKE_HASH must be 0, 1, or 2"
#endif /* DKE_HASH */
