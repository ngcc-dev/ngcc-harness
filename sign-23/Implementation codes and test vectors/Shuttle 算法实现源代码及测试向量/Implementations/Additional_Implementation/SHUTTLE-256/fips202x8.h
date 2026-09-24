/*
 * fips202x8.h -- 8-way (AVX-512) SHAKE128/SHAKE256 for SHUTTLE SHA3_MODE.
 *
 * Vendored from an AVX-512 reference (fips202x8.c / keccakf1600x8.c);
 * self-contained inside SHUTTLE/.  The namespace macro
 * FIPS202X8_NAMESPACE is bound to the SHUTTLE per-set/per-backend
 * SHUTTLE_NAMESPACE (config.h) so all sets co-link.  Compiled ONLY under
 * SHA3_MODE; the default NGCC_MODE avx512 build never links it.
 *
 * 8-way Keccak (XOF_LANES_AVX512 for SHA3): packs one 64-bit lane per
 * __m512i qword.  Derived from the XKCP 8-way Keccak-f[1600] path.  The
 * scalar KeccakF_RoundConstants table (ref/fips202.c) is reused by
 * keccakf1600x8.c.
 */
#ifndef SHUTTLE_FIPS202X8_H
#define SHUTTLE_FIPS202X8_H

#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h" /* SHUTTLE_NAMESPACE (per set x backend) */

#ifndef SHUTTLE_NAMESPACE
#    define SHUTTLE_NAMESPACE(s) s
#endif
#define FIPS202X8_NAMESPACE(s) SHUTTLE_NAMESPACE(fips202x8_##s)

typedef struct {
    __m512i s[25];
} keccakx8_state;

#define shake128x8_absorb_once FIPS202X8_NAMESPACE(shake128x8_absorb_once)
void shake128x8_absorb_once(keccakx8_state *state, const uint8_t *in0,
                            const uint8_t *in1, const uint8_t *in2,
                            const uint8_t *in3, const uint8_t *in4,
                            const uint8_t *in5, const uint8_t *in6,
                            const uint8_t *in7, size_t inlen);

#define shake128x8_squeezeblocks \
    FIPS202X8_NAMESPACE(shake128x8_squeezeblocks)
void shake128x8_squeezeblocks(uint8_t *out0, uint8_t *out1, uint8_t *out2,
                              uint8_t *out3, uint8_t *out4, uint8_t *out5,
                              uint8_t *out6, uint8_t *out7, size_t nblocks,
                              keccakx8_state *state);

#define shake256x8_absorb_once FIPS202X8_NAMESPACE(shake256x8_absorb_once)
void shake256x8_absorb_once(keccakx8_state *state, const uint8_t *in0,
                            const uint8_t *in1, const uint8_t *in2,
                            const uint8_t *in3, const uint8_t *in4,
                            const uint8_t *in5, const uint8_t *in6,
                            const uint8_t *in7, size_t inlen);

#define shake256x8_squeezeblocks \
    FIPS202X8_NAMESPACE(shake256x8_squeezeblocks)
void shake256x8_squeezeblocks(uint8_t *out0, uint8_t *out1, uint8_t *out2,
                              uint8_t *out3, uint8_t *out4, uint8_t *out5,
                              uint8_t *out6, uint8_t *out7, size_t nblocks,
                              keccakx8_state *state);

#define shake128x8 FIPS202X8_NAMESPACE(shake128x8)
void shake128x8(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
                uint8_t *out4, uint8_t *out5, uint8_t *out6, uint8_t *out7,
                size_t outlen, const uint8_t *in0, const uint8_t *in1,
                const uint8_t *in2, const uint8_t *in3, const uint8_t *in4,
                const uint8_t *in5, const uint8_t *in6, const uint8_t *in7,
                size_t inlen);

#define shake256x8 FIPS202X8_NAMESPACE(shake256x8)
void shake256x8(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
                uint8_t *out4, uint8_t *out5, uint8_t *out6, uint8_t *out7,
                size_t outlen, const uint8_t *in0, const uint8_t *in1,
                const uint8_t *in2, const uint8_t *in3, const uint8_t *in4,
                const uint8_t *in5, const uint8_t *in6, const uint8_t *in7,
                size_t inlen);

#endif /* SHUTTLE_FIPS202X8_H */
