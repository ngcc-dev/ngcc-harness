#ifndef SHUTTLE_CONFIG_H
#define SHUTTLE_CONFIG_H

/*
 * Compile-time configuration for the SHUTTLE signature scheme (reference
 * backend).
 *
 * SHUTTLE_MODE selects the SUF parameter set:
 *   128 -> SHUTTLE-128  (n=256,  q=15361, signed-int16 NTT)
 *   256 -> SHUTTLE-256  (n=512,  q=61441, unsigned-uint16 valley NTT)
 *   512 -> SHUTTLE-512  (n=1024, q=59393, unsigned-uint16 valley NTT)
 *
 * Override at build time with: -DSHUTTLE_MODE=128|256|512.
 *
 * This is a REAL FORK per backend: ref/avx2/avx512 differ ONLY in the
 * namespace tail (_ref / _avx2 / _avx512) so all 3 sets x 3 backends (9
 * libraries) co-link without symbol clash.  Everything else (params.h,
 * namespace.h, api.h) is backend-independent and symlinked from ref/.
 */

#ifndef SHUTTLE_MODE
#    define SHUTTLE_MODE 128
#endif

#if SHUTTLE_MODE == 128
#    define CRYPTO_ALGNAME "SHUTTLE-128"
#    define SHUTTLE_NAMESPACETOP shuttle128_ref
#    define SHUTTLE_NAMESPACE(s) shuttle128_ref_##s
#elif SHUTTLE_MODE == 256
#    define CRYPTO_ALGNAME "SHUTTLE-256"
#    define SHUTTLE_NAMESPACETOP shuttle256_ref
#    define SHUTTLE_NAMESPACE(s) shuttle256_ref_##s
#elif SHUTTLE_MODE == 512
#    define CRYPTO_ALGNAME "SHUTTLE-512"
#    define SHUTTLE_NAMESPACETOP shuttle512_ref
#    define SHUTTLE_NAMESPACE(s) shuttle512_ref_##s
#else
#    error "Unsupported SHUTTLE_MODE (expected 128, 256, or 512)"
#endif

/* Symbol renames (shuttle<set>_ref_*) all live in one place; see namespace.h.
 * Included here, after SHUTTLE_NAMESPACE is defined, so every translation unit
 * that reaches config.h (the whole tree, via params.h) gets the renames.
 * Disable with -DDISABLE_NAMESPACE=1. */
#include "namespace.h"

#endif /* SHUTTLE_CONFIG_H */
