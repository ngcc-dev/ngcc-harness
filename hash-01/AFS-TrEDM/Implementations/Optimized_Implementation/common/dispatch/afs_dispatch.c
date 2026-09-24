/*
 * Optional runtime-dispatch wrapper.
 *
 * The portable and AVX2 implementations are linked into dispatch binaries with
 * renamed symbols. This file chooses between them without changing the standard
 * ICCS CryptHash() symbol.
 */
#include <stdlib.h>
#include <string.h>

#include "CryptHash_AlgorithmInstance.h"
#include "afs_dispatch.h"

#define AFS_DISPATCH_AVX2_UNAVAILABLE (-90)

int afs_dispatch_portable_CryptHash(int digest_len_bits,
                                    const unsigned char *msg,
                                    unsigned long long msg_len_bits,
                                    unsigned char *digest);

int afs_dispatch_avx2_CryptHash(int digest_len_bits,
                                const unsigned char *msg,
                                unsigned long long msg_len_bits,
                                unsigned char *digest);

static const char *dispatch_force_value(void)
{
    const char *v = getenv("AFS_TREDM_DISPATCH_FORCE");
    if (v == NULL || v[0] == '\0') {
        return "auto";
    }
    return v;
}

/* Function CryptHash_dispatch_avx2_available: detects whether the current host supports the AVX2 dispatch backend. */
int CryptHash_dispatch_avx2_available(void)
{
#if defined(AFS_TREDM_DISPATCH_HAS_AVX2) && \
    (defined(__GNUC__) || defined(__clang__)) && \
    (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2") ? 1 : 0;
#else
    return 0;
#endif
}

const char *CryptHash_dispatch_selected_backend(void)
{
    const char *force = dispatch_force_value();

    if (strcmp(force, "portable") == 0 || strcmp(force, "opt64") == 0) {
        return "portable";
    }
    if (strcmp(force, "avx2") == 0) {
        return CryptHash_dispatch_avx2_available() ? "avx2" : "avx2-unavailable";
    }
    return CryptHash_dispatch_avx2_available() ? "avx2" : "portable";
}

/* Function CryptHash_dispatch: selects and invokes the portable or AVX2 CryptHash backend. */
int CryptHash_dispatch(int digest_len_bits,
                       const unsigned char *msg,
                       unsigned long long msg_len_bits,
                       unsigned char *digest)
{
    const char *backend = CryptHash_dispatch_selected_backend();

    if (strcmp(backend, "portable") == 0) {
        return afs_dispatch_portable_CryptHash(digest_len_bits, msg, msg_len_bits, digest);
    }
    if (strcmp(backend, "avx2") == 0) {
#if defined(AFS_TREDM_DISPATCH_HAS_AVX2)
        return afs_dispatch_avx2_CryptHash(digest_len_bits, msg, msg_len_bits, digest);
#else
        return AFS_DISPATCH_AVX2_UNAVAILABLE;
#endif
    }
    return AFS_DISPATCH_AVX2_UNAVAILABLE;
}
