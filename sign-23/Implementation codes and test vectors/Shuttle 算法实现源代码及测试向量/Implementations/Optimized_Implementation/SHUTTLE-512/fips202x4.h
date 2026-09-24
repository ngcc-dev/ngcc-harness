/*
 * fips202x4.h -- 4-way (AVX2) SHAKE128/SHAKE256 for SHUTTLE SHA3_MODE.
 *
 * Vendored from the Dilithium AVX2 reference
 * (/home/jipengzhang/code/dilithium/avx2/{fips202x4.{c,h},f1600x4.S});
 * self-contained inside SHUTTLE/.  The namespace macro FIPS202X4_NAMESPACE
 * is bound to the SHUTTLE per-set/per-backend SHUTTLE_NAMESPACE (config.h)
 * so the C side (fips202x4.c) and the asm side (f1600x4.S, which #includes
 * this header under __ASSEMBLER__ to label f1600x4 via cdecl()) agree on
 * the symbol name and link.  Compiled ONLY under SHA3_MODE; the default
 * NGCC_MODE avx2 build never links it.
 *
 * 4-way Keccak (XOF_LANES_AVX2 for SHA3): packs one 64-bit lane per
 * __m256i qword.  The scalar KeccakF_RoundConstants table lives in
 * ref/fips202.c (one shared, namespaced symbol).
 */
#ifndef SHUTTLE_FIPS202X4_H
#define SHUTTLE_FIPS202X4_H

#include "config.h" /* SHUTTLE_NAMESPACE (per set x backend) */

#ifndef SHUTTLE_NAMESPACE
#    define SHUTTLE_NAMESPACE(s) s
#endif
#define FIPS202X4_NAMESPACE(s) SHUTTLE_NAMESPACE(fips202x4_##s)

#ifdef __ASSEMBLER__
/* MacOS/Win leading-underscore handling, kept from the upstream header. */
#    if defined(__WIN32__) || defined(__APPLE__)
#        define decorate(s) _##s
#        define _cdecl(s) decorate(s)
#        define cdecl(s) _cdecl(FIPS202X4_NAMESPACE(##s))
#    else
#        define cdecl(s) FIPS202X4_NAMESPACE(##s)
#    endif

#else
#    include <immintrin.h>
#    include <stddef.h>
#    include <stdint.h>

typedef struct {
    __m256i s[25];
} keccakx4_state;

#    define f1600x4 FIPS202X4_NAMESPACE(f1600x4)
void f1600x4(__m256i *s, const uint64_t *rc);

#    define shake128x4_absorb_once \
        FIPS202X4_NAMESPACE(shake128x4_absorb_once)
void shake128x4_absorb_once(keccakx4_state *state, const uint8_t *in0,
                            const uint8_t *in1, const uint8_t *in2,
                            const uint8_t *in3, size_t inlen);

#    define shake128x4_squeezeblocks \
        FIPS202X4_NAMESPACE(shake128x4_squeezeblocks)
void shake128x4_squeezeblocks(uint8_t *out0, uint8_t *out1, uint8_t *out2,
                              uint8_t *out3, size_t nblocks,
                              keccakx4_state *state);

#    define shake256x4_absorb_once \
        FIPS202X4_NAMESPACE(shake256x4_absorb_once)
void shake256x4_absorb_once(keccakx4_state *state, const uint8_t *in0,
                            const uint8_t *in1, const uint8_t *in2,
                            const uint8_t *in3, size_t inlen);

#    define shake256x4_squeezeblocks \
        FIPS202X4_NAMESPACE(shake256x4_squeezeblocks)
void shake256x4_squeezeblocks(uint8_t *out0, uint8_t *out1, uint8_t *out2,
                              uint8_t *out3, size_t nblocks,
                              keccakx4_state *state);

#    define shake128x4 FIPS202X4_NAMESPACE(shake128x4)
void shake128x4(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
                size_t outlen, const uint8_t *in0, const uint8_t *in1,
                const uint8_t *in2, const uint8_t *in3, size_t inlen);

#    define shake256x4 FIPS202X4_NAMESPACE(shake256x4)
void shake256x4(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
                size_t outlen, const uint8_t *in0, const uint8_t *in1,
                const uint8_t *in2, const uint8_t *in3, size_t inlen);

#endif /* __ASSEMBLER__ */
#endif /* SHUTTLE_FIPS202X4_H */
