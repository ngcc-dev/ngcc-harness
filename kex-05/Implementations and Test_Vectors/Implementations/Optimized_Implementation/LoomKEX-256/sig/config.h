#ifndef SHUTTLE_CONFIG_H
#define SHUTTLE_CONFIG_H

/*
 * SHUTTLE compile-time config (standalone — safe for f1600x4.S via __ASSEMBLER__).
 * LOOM_MODE and LOOM_AVX2 are set by CMake.
 */

#ifndef SHUTTLE_MODE
#    if LOOM_MODE == 1
#        define SHUTTLE_MODE 128
#    elif LOOM_MODE == 3
#        define SHUTTLE_MODE 256
#    elif LOOM_MODE == 5
#        define SHUTTLE_MODE 512
#    else
#        error "LOOM_MODE must be 1, 3, or 5"
#    endif
#endif

#if SHUTTLE_MODE == 128
#    define CRYPTO_ALGNAME "SHUTTLE-128"
#    if defined(LOOM_AVX2)
#        define SHUTTLE_NAMESPACETOP shuttle128_avx2
#        define SHUTTLE_NAMESPACE(s) shuttle128_avx2_##s
#    else
#        define SHUTTLE_NAMESPACETOP shuttle128_ref
#        define SHUTTLE_NAMESPACE(s) shuttle128_ref_##s
#    endif
#elif SHUTTLE_MODE == 256
#    define CRYPTO_ALGNAME "SHUTTLE-256"
#    if defined(LOOM_AVX2)
#        define SHUTTLE_NAMESPACETOP shuttle256_avx2
#        define SHUTTLE_NAMESPACE(s) shuttle256_avx2_##s
#    else
#        define SHUTTLE_NAMESPACETOP shuttle256_ref
#        define SHUTTLE_NAMESPACE(s) shuttle256_ref_##s
#    endif
#elif SHUTTLE_MODE == 512
#    define CRYPTO_ALGNAME "SHUTTLE-512"
#    if defined(LOOM_AVX2)
#        define SHUTTLE_NAMESPACETOP shuttle512_avx2
#        define SHUTTLE_NAMESPACE(s) shuttle512_avx2_##s
#    else
#        define SHUTTLE_NAMESPACETOP shuttle512_ref
#        define SHUTTLE_NAMESPACE(s) shuttle512_ref_##s
#    endif
#else
#    error "Unsupported SHUTTLE_MODE (expected 128, 256, or 512)"
#endif

#endif /* SHUTTLE_CONFIG_H */
