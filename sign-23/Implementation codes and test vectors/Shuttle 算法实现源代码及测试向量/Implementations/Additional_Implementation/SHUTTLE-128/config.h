#ifndef SHUTTLE_CONFIG_H
#define SHUTTLE_CONFIG_H

/*
 * Compile-time configuration for the SHUTTLE signature scheme (AVX-512
 * backend).
 *
 * Byte-identical to ref/config.h except the namespace tail _ref -> _avx512, so
 * the AVX-512 libraries co-link with the reference and AVX2 ones without symbol
 * clash.  See ref/config.h for the SHUTTLE_MODE documentation.
 *
 * Override at build time with: -DSHUTTLE_MODE=128|256|512.
 */

#ifndef SHUTTLE_MODE
#    define SHUTTLE_MODE 128
#endif

#if SHUTTLE_MODE == 128
#    define CRYPTO_ALGNAME "SHUTTLE-128"
#    define SHUTTLE_NAMESPACETOP shuttle128_avx512
#    define SHUTTLE_NAMESPACE(s) shuttle128_avx512_##s
#elif SHUTTLE_MODE == 256
#    define CRYPTO_ALGNAME "SHUTTLE-256"
#    define SHUTTLE_NAMESPACETOP shuttle256_avx512
#    define SHUTTLE_NAMESPACE(s) shuttle256_avx512_##s
#elif SHUTTLE_MODE == 512
#    define CRYPTO_ALGNAME "SHUTTLE-512"
#    define SHUTTLE_NAMESPACETOP shuttle512_avx512
#    define SHUTTLE_NAMESPACE(s) shuttle512_avx512_##s
#else
#    error "Unsupported SHUTTLE_MODE (expected 128, 256, or 512)"
#endif

#include "namespace.h"

#endif /* SHUTTLE_CONFIG_H */
