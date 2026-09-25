/*
 * fips202.h -- scalar FIPS-202 Keccak/SHAKE128/SHAKE256 for SHUTTLE
 * SHA3_MODE.
 *
 * Vendored, self-contained inside SHUTTLE/.  Byte-identical FIPS-202; the
 * permutation, padding (0x1F SHAKE domain byte) and rates match the
 * Dilithium/Lithium reference (and thus any standard SHAKE test vector).
 *
 * Source: public-domain Keccak (Van Keer / "TweetFips202" by Van Assche,
 * Bernstein, Schwabe), as packaged by the Dilithium reference
 * (/home/jipengzhang/code/dilithium/ref/fips202.{c,h}).  The only edit is
 * the namespace: FIPS202_NAMESPACE is bound to the SHUTTLE
 * per-set/per-backend SHUTTLE_NAMESPACE (config.h) so all sets x backends
 * co-link without clash.
 *
 * This translation unit is compiled into the build ONLY when SHA3_MODE is
 * defined; the default NGCC_MODE build never links it (and never includes
 * this header), keeping the SM3-only default warning-clean and free of
 * Keccak code.
 */
#ifndef SHUTTLE_FIPS202_H
#define SHUTTLE_FIPS202_H

#include <stddef.h>
#include <stdint.h>

#include "config.h" /* SHUTTLE_NAMESPACE (per set x backend) */

#define SHAKE128_RATE 168
#define SHAKE256_RATE 136
#define SHA3_256_RATE 136
#define SHA3_512_RATE 72

/* Bind the FIPS-202 namespace to the SHUTTLE per-set/per-backend namespace
 * so shuttle128_ref_*, shuttle256_avx2_*, ... all co-link.  Under
 * -DDISABLE_NAMESPACE=1, SHUTTLE_NAMESPACE collapses to the bare name. */
#ifndef SHUTTLE_NAMESPACE
#    define SHUTTLE_NAMESPACE(s) s
#endif
#define FIPS202_NAMESPACE(s) SHUTTLE_NAMESPACE(fips202_##s)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t s[25];
    unsigned int pos;
} keccak_state;

#define KeccakF_RoundConstants FIPS202_NAMESPACE(KeccakF_RoundConstants)
extern const uint64_t KeccakF_RoundConstants[];

#define shake128_init FIPS202_NAMESPACE(shake128_init)
void shake128_init(keccak_state *state);
#define shake128_absorb FIPS202_NAMESPACE(shake128_absorb)
void shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
#define shake128_finalize FIPS202_NAMESPACE(shake128_finalize)
void shake128_finalize(keccak_state *state);
#define shake128_squeeze FIPS202_NAMESPACE(shake128_squeeze)
void shake128_squeeze(uint8_t *out, size_t outlen, keccak_state *state);
#define shake128_absorb_once FIPS202_NAMESPACE(shake128_absorb_once)
void shake128_absorb_once(keccak_state *state, const uint8_t *in,
                          size_t inlen);
#define shake128_squeezeblocks FIPS202_NAMESPACE(shake128_squeezeblocks)
void shake128_squeezeblocks(uint8_t *out, size_t nblocks,
                            keccak_state *state);

#define shake256_init FIPS202_NAMESPACE(shake256_init)
void shake256_init(keccak_state *state);
#define shake256_absorb FIPS202_NAMESPACE(shake256_absorb)
void shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen);
#define shake256_finalize FIPS202_NAMESPACE(shake256_finalize)
void shake256_finalize(keccak_state *state);
#define shake256_squeeze FIPS202_NAMESPACE(shake256_squeeze)
void shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state);
#define shake256_absorb_once FIPS202_NAMESPACE(shake256_absorb_once)
void shake256_absorb_once(keccak_state *state, const uint8_t *in,
                          size_t inlen);
#define shake256_squeezeblocks FIPS202_NAMESPACE(shake256_squeezeblocks)
void shake256_squeezeblocks(uint8_t *out, size_t nblocks,
                            keccak_state *state);

#define shake128 FIPS202_NAMESPACE(shake128)
void shake128(uint8_t *out, size_t outlen, const uint8_t *in,
              size_t inlen);
#define shake256 FIPS202_NAMESPACE(shake256)
void shake256(uint8_t *out, size_t outlen, const uint8_t *in,
              size_t inlen);
#define sha3_256 FIPS202_NAMESPACE(sha3_256)
void sha3_256(uint8_t h[32], const uint8_t *in, size_t inlen);
#define sha3_512 FIPS202_NAMESPACE(sha3_512)
void sha3_512(uint8_t h[64], const uint8_t *in, size_t inlen);

#ifdef __cplusplus
}
#endif

#endif /* SHUTTLE_FIPS202_H */
