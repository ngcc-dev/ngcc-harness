#include "CryptHash_AlgorithmInstance.h"
#include "CryptHash_Batch.h"
#include "afs_batch16_s6_avx512.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Function prng64: generates deterministic pseudo-random test data for self-tests. */
static uint64_t prng64(uint64_t *s)
{
    uint64_t x = *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x;
    return x;
}

/* Function fill_message: fills a deterministic test or benchmark message buffer. */
static void fill_message(uint8_t *msg, size_t nbytes, unsigned lane, uint64_t tag)
{
    uint64_t s = UINT64_C(0xA0761D6478BD642F) ^ ((uint64_t)lane << 32) ^ tag;
    size_t i;
    for (i = 0U; i < nbytes; i++) {
        msg[i] = (uint8_t)(prng64(&s) >> 56);
    }
}

/* Function mask_unused_low_bits: clears unused low bits in the last partial message byte. */
static void mask_unused_low_bits(uint8_t *msg, unsigned long long bits)
{
    const unsigned rem = (unsigned)(bits & 7ULL);
    if (rem != 0U && bits != 0ULL) {
        const size_t nbytes = (size_t)((bits + 7ULL) >> 3);
        msg[nbytes - 1U] &= (uint8_t)(0xffU << (8U - rem));
    }
}

/* Function check_batch16_case: checks one batch16 AVX512 case against scalar reference outputs. */
static int check_batch16_case(unsigned long long bits, uint64_t tag, int expect_hot)
{
    const size_t digest_bytes = DIGEST_BIT_LENGTH / 8U;
    const size_t msg_bytes = (size_t)((bits + 7ULL) >> 3);
    const unsigned char *msgs[16];
    unsigned char *msg_storage[16];
    unsigned char *digests[16];
    unsigned char *expected[16];
    unsigned long long lens[16];
    AFS_TREDM_BatchBackendStats stats;
    unsigned i;
    int rc;

    for (i = 0U; i < 16U; i++) {
        msg_storage[i] = NULL;
        digests[i] = NULL;
        expected[i] = NULL;
        msgs[i] = NULL;
        lens[i] = bits;
        if (msg_bytes != 0U) {
            msg_storage[i] = (unsigned char *)malloc(msg_bytes);
            if (msg_storage[i] == NULL) {
                return 1;
            }
            fill_message(msg_storage[i], msg_bytes, i, tag);
            mask_unused_low_bits(msg_storage[i], bits);
            msgs[i] = msg_storage[i];
        }
        digests[i] = (unsigned char *)calloc(digest_bytes, 1U);
        expected[i] = (unsigned char *)calloc(digest_bytes, 1U);
        if (digests[i] == NULL || expected[i] == NULL) {
            return 1;
        }
    }

    CryptHash_Batch_ResetStats();
    rc = CryptHash_Batch16(DIGEST_BIT_LENGTH, msgs, lens, digests);
    if (rc != 0) {
        fprintf(stderr, "CryptHash_Batch16 failed for bits=%llu rc=%d\n", bits, rc);
        return 1;
    }
    stats = CryptHash_Batch_GetStats();
    if (expect_hot) {
        if (stats.batch16_avx512_hot_blocks == 0U ||
            stats.batch16_avx512_hot_messages != 16U ||
            stats.batch16_scalar_fallback_messages != 0U) {
            fprintf(stderr, "expected hot path for bits=%llu, got hot_blocks=%llu hot_messages=%llu fallback=%llu\n",
                    bits,
                    (unsigned long long)stats.batch16_avx512_hot_blocks,
                    (unsigned long long)stats.batch16_avx512_hot_messages,
                    (unsigned long long)stats.batch16_scalar_fallback_messages);
            return 1;
        }
    } else if (stats.batch16_scalar_fallback_messages != 16U) {
        fprintf(stderr, "expected fallback for bits=%llu, got fallback=%llu hot_blocks=%llu\n",
                bits,
                (unsigned long long)stats.batch16_scalar_fallback_messages,
                (unsigned long long)stats.batch16_avx512_hot_blocks);
        return 1;
    }

    for (i = 0U; i < 16U; i++) {
        rc = CryptHash(DIGEST_BIT_LENGTH, msgs[i], bits, expected[i]);
        if (rc != 0) {
            fprintf(stderr, "CryptHash oracle failed lane=%u bits=%llu rc=%d\n", i, bits, rc);
            return 1;
        }
        if (memcmp(digests[i], expected[i], digest_bytes) != 0) {
            fprintf(stderr, "digest mismatch lane=%u bits=%llu\n", i, bits);
            return 1;
        }
    }

    for (i = 0U; i < 16U; i++) {
        free(expected[i]);
        free(digests[i]);
        free(msg_storage[i]);
    }
    return 0;
}

/* Function check_count_fallback: checks fallback behavior for message counts not handled by true batch16. */
static int check_count_fallback(void)
{
    const size_t digest_bytes = DIGEST_BIT_LENGTH / 8U;
    unsigned char msg_storage[15][16];
    const unsigned char *msgs[15];
    unsigned char *digests[15];
    unsigned char digest_storage[15][DIGEST_BIT_LENGTH / 8U];
    unsigned long long lens[15];
    AFS_TREDM_BatchBackendStats stats;
    unsigned i;

    for (i = 0U; i < 15U; i++) {
        fill_message(msg_storage[i], sizeof(msg_storage[i]), i, UINT64_C(0x515));
        msgs[i] = msg_storage[i];
        digests[i] = digest_storage[i];
        memset(digests[i], 0, digest_bytes);
        lens[i] = 128ULL;
    }
    CryptHash_Batch_ResetStats();
    if (CryptHash_BatchMany(DIGEST_BIT_LENGTH, 15U, msgs, lens, digests) != 0) {
        return 1;
    }
    stats = CryptHash_Batch_GetStats();
    if (stats.scalar_fallback_messages != 15U ||
        stats.batch16_avx512_hot_messages != 0U) {
        fprintf(stderr, "count fallback stats mismatch\n");
        return 1;
    }
    return 0;
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(void)
{
    static const unsigned long long directed_bits[] = {
        0ULL,
        8ULL,
        512ULL,
        768ULL,
        1024ULL,
        8192ULL,
        8388608ULL
    };
    uint64_t rnd = UINT64_C(0x123456789ABCDEF0);
    unsigned i;

    if (!CryptHash_Batch16_IsTrueAVX512Enabled()) {
        fprintf(stderr, "true Batch16 AVX512 backend is not enabled\n");
        return 1;
    }
    if (afs_batch16_avx512_selftest_primitives() != 0) {
        fprintf(stderr, "AVX512 primitive selftest failed\n");
        return 1;
    }
    for (i = 0U; i < sizeof(directed_bits) / sizeof(directed_bits[0]); i++) {
        const unsigned long long bits = directed_bits[i];
        const int expect_hot = (bits & 7ULL) == 0ULL;
        if (check_batch16_case(bits, UINT64_C(0xD100) + i, expect_hot) != 0) {
            return 1;
        }
    }
    if (check_count_fallback() != 0) {
        return 1;
    }
    if (check_batch16_case(9ULL, UINT64_C(0xBAD9), 0) != 0) {
        return 1;
    }
    for (i = 0U; i < 100U; i++) {
        unsigned long long bits;
        if ((i % 5U) == 0U) {
            bits = (unsigned long long)((prng64(&rnd) % 16U) * 1024U);
        } else {
            bits = (unsigned long long)(prng64(&rnd) % 4097U);
        }
        if (check_batch16_case(bits, rnd ^ i, (bits & 7ULL) == 0ULL) != 0) {
            return 1;
        }
    }

    printf("batch16 AVX512 selftest passed for %s\n", ALGORITHM_INSTANCE);
    return 0;
}
