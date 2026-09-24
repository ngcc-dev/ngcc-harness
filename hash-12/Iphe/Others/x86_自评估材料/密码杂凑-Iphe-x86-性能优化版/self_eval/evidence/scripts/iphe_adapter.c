#include <stdio.h>
#include <string.h>
#include "crypthash_adapter.h"
#include "registry.h"

#define IPHE_MAX_DIGEST_BYTES 128U

typedef int (*iphe_hash_fn)(int, const unsigned char *,
                            unsigned long long, unsigned char *);

int CryptHash_iphe512(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe768(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe1024(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe512_perf(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe768_perf(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe1024_perf(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe512_res(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe768_res(int, const unsigned char *, unsigned long long, unsigned char *);
int CryptHash_iphe1024_res(int, const unsigned char *, unsigned long long, unsigned char *);

static int iphe_compare_case(iphe_hash_fn reference, iphe_hash_fn candidate,
                             int digest_bits, const unsigned char *msg,
                             unsigned long long msg_bits)
{
    unsigned char expected[IPHE_MAX_DIGEST_BYTES];
    unsigned char actual[IPHE_MAX_DIGEST_BYTES];
    size_t digest_bytes = (size_t)digest_bits / 8U;

    memset(expected, 0, sizeof(expected));
    memset(actual, 0xff, sizeof(actual));
    if (reference(digest_bits, msg, msg_bits, expected) != 0 ||
        candidate(digest_bits, msg, msg_bits, actual) != 0)
        return 1;
    return memcmp(expected, actual, digest_bytes) != 0;
}

static int iphe_self_test(iphe_hash_fn reference, iphe_hash_fn candidate,
                          int digest_bits)
{
    static const unsigned char abc[] = {'a', 'b', 'c'};
    unsigned char long_msg[4096];
    unsigned char bit_msg[2] = {0xb6, 0x80};
    unsigned char digest[IPHE_MAX_DIGEST_BYTES];
    size_t i;

    for (i = 0; i < sizeof(long_msg); i++)
        long_msg[i] = (unsigned char)(11U + 37U * i);

    if (iphe_compare_case(reference, candidate, digest_bits, NULL, 0) ||
        iphe_compare_case(reference, candidate, digest_bits, abc, 24) ||
        iphe_compare_case(reference, candidate, digest_bits, long_msg,
                          (unsigned long long)sizeof(long_msg) * 8ULL) ||
        iphe_compare_case(reference, candidate, digest_bits, bit_msg, 1) ||
        iphe_compare_case(reference, candidate, digest_bits, bit_msg, 7) ||
        iphe_compare_case(reference, candidate, digest_bits, bit_msg, 9) ||
        iphe_compare_case(reference, candidate, digest_bits, bit_msg, 15))
        return 1;

    return candidate(digest_bits == 512 ? 768 : 512, abc, 24, digest) == 0;
}

#define DEFINE_IPHE_REFERENCE(BITS, RATE_BYTES)                                    \
    static size_t iphe_##BITS##_get_digest_len(void) { return (BITS) / 8U; }        \
    static size_t iphe_##BITS##_get_block_len(void) { return (RATE_BYTES); }        \
    static int iphe_##BITS##_do_hash(int digest_bits, const uint8_t *msg,           \
                                     size_t msg_bits, uint8_t *digest)              \
    {                                                                               \
        return CryptHash_iphe##BITS(digest_bits, msg,                               \
                                    (unsigned long long)msg_bits, digest);           \
    }                                                                               \
    static int iphe_##BITS##_self_test(void)                                        \
    {                                                                               \
        return iphe_self_test(CryptHash_iphe##BITS, CryptHash_iphe##BITS, BITS);    \
    }                                                                               \
    static HASH_METHOD iphe_##BITS##_method = {                                     \
        iphe_##BITS##_get_digest_len, iphe_##BITS##_get_block_len,                  \
        iphe_##BITS##_do_hash, iphe_##BITS##_self_test                              \
    };                                                                              \
    static ALGORITHM iphe_##BITS##_alg = {                                          \
        ALG_HASH, IPHE_##BITS, "Iphe-" #BITS, "ICCS",                             \
        "API_CryptHash/Implementations/Reference_Implementation/libiphe_" #BITS ".a", \
        &iphe_##BITS##_method                                                       \
    };                                                                              \
    void register_iphe_##BITS(void)                                                 \
    {                                                                               \
        if (registry_algorithm(&iphe_##BITS##_alg))                                 \
            fprintf(stderr, "Failed to register Iphe-%d.\n", BITS);               \
    }

#define DEFINE_IPHE_VARIANT(BITS, RATE_BYTES, TAG, ALG_ID, LABEL, LIB_PATH, HASH_FN) \
    static size_t iphe_##BITS##_##TAG##_get_digest_len(void) { return (BITS) / 8U; } \
    static size_t iphe_##BITS##_##TAG##_get_block_len(void) { return (RATE_BYTES); } \
    static int iphe_##BITS##_##TAG##_do_hash(int digest_bits, const uint8_t *msg,    \
                                              size_t msg_bits, uint8_t *digest)       \
    {                                                                                \
        return HASH_FN(digest_bits, msg, (unsigned long long)msg_bits, digest);       \
    }                                                                                \
    static int iphe_##BITS##_##TAG##_self_test(void)                                 \
    {                                                                                \
        return iphe_self_test(CryptHash_iphe##BITS, HASH_FN, BITS);                  \
    }                                                                                \
    static HASH_METHOD iphe_##BITS##_##TAG##_method = {                              \
        iphe_##BITS##_##TAG##_get_digest_len, iphe_##BITS##_##TAG##_get_block_len,   \
        iphe_##BITS##_##TAG##_do_hash, iphe_##BITS##_##TAG##_self_test               \
    };                                                                               \
    static ALGORITHM iphe_##BITS##_##TAG##_alg = {                                   \
        ALG_HASH, ALG_ID, LABEL, "ICCS", LIB_PATH, &iphe_##BITS##_##TAG##_method    \
    };                                                                               \
    void register_iphe_##BITS##_##TAG(void)                                          \
    {                                                                                \
        if (registry_algorithm(&iphe_##BITS##_##TAG##_alg))                          \
            fprintf(stderr, "Failed to register %s.\n", LABEL);                    \
    }

DEFINE_IPHE_REFERENCE(512, 184U)
DEFINE_IPHE_REFERENCE(768, 152U)
DEFINE_IPHE_REFERENCE(1024, 120U)

DEFINE_IPHE_VARIANT(512, 184U, perf, IPHE_512_PERF, "Iphe-512-Performance",
    "API_CryptHash/Implementations/Optimized_Implementation/libiphe_512_perf.a", CryptHash_iphe512_perf)
DEFINE_IPHE_VARIANT(768, 152U, perf, IPHE_768_PERF, "Iphe-768-Performance",
    "API_CryptHash/Implementations/Optimized_Implementation/libiphe_768_perf.a", CryptHash_iphe768_perf)
DEFINE_IPHE_VARIANT(1024, 120U, perf, IPHE_1024_PERF, "Iphe-1024-Performance",
    "API_CryptHash/Implementations/Optimized_Implementation/libiphe_1024_perf.a", CryptHash_iphe1024_perf)

DEFINE_IPHE_VARIANT(512, 184U, res, IPHE_512_RES, "Iphe-512-Resource",
    "API_CryptHash/Implementations/Optimized_Implementation/libiphe_512_res.a", CryptHash_iphe512_res)
DEFINE_IPHE_VARIANT(768, 152U, res, IPHE_768_RES, "Iphe-768-Resource",
    "API_CryptHash/Implementations/Optimized_Implementation/libiphe_768_res.a", CryptHash_iphe768_res)
DEFINE_IPHE_VARIANT(1024, 120U, res, IPHE_1024_RES, "Iphe-1024-Resource",
    "API_CryptHash/Implementations/Optimized_Implementation/libiphe_1024_res.a", CryptHash_iphe1024_res)
