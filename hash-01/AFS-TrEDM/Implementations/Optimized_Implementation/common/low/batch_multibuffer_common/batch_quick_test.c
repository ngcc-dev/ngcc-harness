#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "CryptHash_AlgorithmInstance.h"
#include "CryptHash_Batch.h"

#if DIGEST_BIT_LENGTH == 512
#define AFS_BATCH_RATE_BITS 1024ULL
#elif DIGEST_BIT_LENGTH == 768
#define AFS_BATCH_RATE_BITS 768ULL
#elif DIGEST_BIT_LENGTH == 1024
#define AFS_BATCH_RATE_BITS 512ULL
#else
#error "Unsupported AFS-TrEDM digest length for batch quick test"
#endif

/* Function fill_message: fills a deterministic test or benchmark message buffer. */
static void fill_message(unsigned char *msg, size_t nbytes, unsigned lane)
{
    uint64_t x = UINT64_C(0x9E3779B97F4A7C15) ^ ((uint64_t)lane << 32);
    size_t i;
    for (i = 0U; i < nbytes; i++) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        msg[i] = (unsigned char)(x >> 56);
    }
}

/* Function mask_unused_low_bits: clears unused low bits in the last partial message byte. */
static void mask_unused_low_bits(unsigned char *msg, unsigned long long bit_len)
{
    unsigned rem = (unsigned)(bit_len & 7ULL);
    size_t nbytes;
    unsigned char mask;
    if (rem == 0U || bit_len == 0ULL) {
        return;
    }
    nbytes = (size_t)((bit_len + 7ULL) / 8ULL);
    mask = (unsigned char)(0xffU << (8U - rem));
    msg[nbytes - 1U] &= mask;
}

/* Function compare_batch: compares one fixed-width batch result against scalar CryptHash outputs. */
static int compare_batch(unsigned width, const unsigned long long lens[16])
{
    const size_t digest_bytes = DIGEST_BIT_LENGTH / 8U;
    const unsigned char *msgs[16];
    unsigned char *msg_storage[16];
    unsigned char *digests[16];
    unsigned char *expected[16];
    unsigned i;
    int rc;

    for (i = 0U; i < 16U; i++) {
        size_t nbytes = (size_t)((lens[i] + 7ULL) / 8ULL);
        msg_storage[i] = NULL;
        digests[i] = NULL;
        expected[i] = NULL;
        msgs[i] = NULL;
        if (nbytes != 0U) {
            msg_storage[i] = (unsigned char *)malloc(nbytes);
            if (msg_storage[i] == NULL) {
                return 1;
            }
            fill_message(msg_storage[i], nbytes, i + width);
            mask_unused_low_bits(msg_storage[i], lens[i]);
            msgs[i] = msg_storage[i];
        }
        digests[i] = (unsigned char *)calloc(digest_bytes, 1U);
        expected[i] = (unsigned char *)calloc(digest_bytes, 1U);
        if (digests[i] == NULL || expected[i] == NULL) {
            return 1;
        }
    }

    if (width == 4U) {
        rc = CryptHash_Batch4(DIGEST_BIT_LENGTH, msgs, lens, digests);
    } else if (width == 8U) {
        rc = CryptHash_Batch8(DIGEST_BIT_LENGTH, msgs, lens, digests);
    } else {
        rc = CryptHash_Batch16(DIGEST_BIT_LENGTH, msgs, lens, digests);
    }
    if (rc != 0) {
        fprintf(stderr, "CryptHash_Batch%u failed, rc=%d\n", width, rc);
        return 1;
    }

    for (i = 0U; i < width; i++) {
        rc = CryptHash(DIGEST_BIT_LENGTH, msgs[i], lens[i], expected[i]);
        if (rc != 0) {
            fprintf(stderr, "CryptHash scalar failed on lane %u, rc=%d\n", i, rc);
            return 1;
        }
        if (memcmp(digests[i], expected[i], digest_bytes) != 0) {
            fprintf(stderr, "Batch%u mismatch on lane %u, msg_bits=%llu\n",
                    width, i, lens[i]);
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

/* Function compare_many: compares arbitrary-count batch results against scalar CryptHash outputs. */
static int compare_many(unsigned count, const unsigned long long *lens)
{
    const size_t digest_bytes = DIGEST_BIT_LENGTH / 8U;
    const unsigned char **msgs = NULL;
    unsigned char **msg_storage = NULL;
    unsigned char **digests = NULL;
    unsigned char **expected = NULL;
    unsigned i;
    int rc;

    msgs = (const unsigned char **)calloc(count, sizeof(*msgs));
    msg_storage = (unsigned char **)calloc(count, sizeof(*msg_storage));
    digests = (unsigned char **)calloc(count, sizeof(*digests));
    expected = (unsigned char **)calloc(count, sizeof(*expected));
    if (msgs == NULL || msg_storage == NULL || digests == NULL || expected == NULL) {
        free(expected);
        free(digests);
        free(msg_storage);
        free(msgs);
        return 1;
    }

    for (i = 0U; i < count; i++) {
        size_t nbytes = (size_t)((lens[i] + 7ULL) / 8ULL);
        if (nbytes != 0U) {
            msg_storage[i] = (unsigned char *)malloc(nbytes);
            if (msg_storage[i] == NULL) {
                return 1;
            }
            fill_message(msg_storage[i], nbytes, i + count);
            mask_unused_low_bits(msg_storage[i], lens[i]);
            msgs[i] = msg_storage[i];
        }
        digests[i] = (unsigned char *)calloc(digest_bytes, 1U);
        expected[i] = (unsigned char *)calloc(digest_bytes, 1U);
        if (digests[i] == NULL || expected[i] == NULL) {
            return 1;
        }
    }

    rc = CryptHash_BatchMany(DIGEST_BIT_LENGTH, count, msgs, lens, digests);
    if (rc != 0) {
        fprintf(stderr, "CryptHash_BatchMany failed, count=%u rc=%d\n", count, rc);
        return 1;
    }

    for (i = 0U; i < count; i++) {
        rc = CryptHash(DIGEST_BIT_LENGTH, msgs[i], lens[i], expected[i]);
        if (rc != 0) {
            fprintf(stderr, "CryptHash scalar failed on many lane %u, rc=%d\n", i, rc);
            return 1;
        }
        if (memcmp(digests[i], expected[i], digest_bytes) != 0) {
            fprintf(stderr, "BatchMany mismatch on lane %u, msg_bits=%llu\n", i, lens[i]);
            return 1;
        }
    }

    for (i = 0U; i < count; i++) {
        free(expected[i]);
        free(digests[i]);
        free(msg_storage[i]);
    }
    free(expected);
    free(digests);
    free(msg_storage);
    free(msgs);
    return 0;
}

/* Function compare_required_lengths: runs batch comparisons for required representative message lengths. */
static int compare_required_lengths(void)
{
    static const unsigned long long required_lengths[] = {
        0ULL,
        1ULL,
        7ULL,
        8ULL,
        9ULL,
        AFS_BATCH_RATE_BITS - 1ULL,
        AFS_BATCH_RATE_BITS,
        AFS_BATCH_RATE_BITS + 1ULL,
        512ULL,
        1024ULL,
        8192ULL,
        1048576ULL
    };
    unsigned i;

    for (i = 0U; i < sizeof(required_lengths) / sizeof(required_lengths[0]); i++) {
        unsigned long long lens[16];
        unsigned k;
        for (k = 0U; k < 16U; k++) {
            lens[k] = required_lengths[i];
        }
        if (compare_batch(4U, lens) != 0 ||
            compare_batch(8U, lens) != 0 ||
            compare_batch(16U, lens) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Function compare_tail_counts: runs batch comparisons for non-multiple-of-width tail counts. */
static int compare_tail_counts(void)
{
    static const unsigned long long required_lengths[] = {
        0ULL,
        1ULL,
        7ULL,
        8ULL,
        9ULL,
        AFS_BATCH_RATE_BITS - 1ULL,
        AFS_BATCH_RATE_BITS,
        AFS_BATCH_RATE_BITS + 1ULL,
        512ULL,
        1024ULL,
        8192ULL,
        1048576ULL
    };
    const unsigned n_lengths =
        (unsigned)(sizeof(required_lengths) / sizeof(required_lengths[0]));
    unsigned count;

    for (count = 1U; count < 16U; count++) {
        unsigned long long lens[16];
        unsigned i;
        for (i = 0U; i < count; i++) {
            lens[i] = required_lengths[(i + count) % n_lengths];
        }
        if (compare_many(count, lens) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(void)
{
    static const unsigned long long equal_lengths[] = {
        0ULL, 8ULL, 512ULL, 8192ULL, 65536ULL, 1048576ULL
    };
    unsigned i;

    if (CryptHash_Batch_RoundConstantSelfTest() != 0) {
        fprintf(stderr, "batch round constant self-test failed\n");
        return 1;
    }

    for (i = 0U; i < sizeof(equal_lengths) / sizeof(equal_lengths[0]); i++) {
        unsigned long long lens[16];
        unsigned k;
        for (k = 0U; k < 16U; k++) {
            lens[k] = equal_lengths[i];
        }
        if (compare_batch(4U, lens) != 0 ||
            compare_batch(8U, lens) != 0 ||
            compare_batch(16U, lens) != 0) {
            return 1;
        }
    }

    {
        unsigned long long unequal[16] = {
            1ULL, 9ULL, 512ULL, 520ULL, 1024ULL, 1032ULL, 2048ULL, 2056ULL,
            4096ULL, 4104ULL, 8192ULL, 8200ULL, 16384ULL, 16392ULL, 32768ULL, 32776ULL
        };
        if (compare_batch(4U, unequal) != 0 ||
            compare_batch(8U, unequal) != 0 ||
            compare_batch(16U, unequal) != 0) {
            return 1;
        }
    }

    if (compare_required_lengths() != 0 || compare_tail_counts() != 0) {
        return 1;
    }

    {
        unsigned long long many_lens[21] = {
            0ULL, 8ULL, 9ULL, 511ULL, 512ULL, 513ULL, 768ULL,
            1024ULL, 2048ULL, 4096ULL, 8192ULL, 16384ULL, 32768ULL,
            65536ULL, 1048576ULL, 520ULL, 1032ULL, 2056ULL, 4104ULL,
            8200ULL, 16392ULL
        };
        if (compare_many(21U, many_lens) != 0) {
            return 1;
        }
    }

    {
        unsigned long long grouped_lens[37] = {
            512ULL, 9ULL, 8192ULL, 512ULL, 1024ULL, 8192ULL, 512ULL, 0ULL,
            1048576ULL, 1024ULL, 512ULL, 8192ULL, 1048576ULL, 512ULL, 1024ULL, 8192ULL,
            512ULL, 1048576ULL, 1024ULL, 8192ULL, 512ULL, 1024ULL, 8192ULL, 1048576ULL,
            512ULL, 1024ULL, 8192ULL, 1048576ULL, 512ULL, 1024ULL, 8192ULL, 1048576ULL,
            512ULL, 1024ULL, 8192ULL, 1048576ULL, 7ULL
        };
        if (compare_many(37U, grouped_lens) != 0) {
            return 1;
        }
    }

    printf("batch quick test passed for %s\n", ALGORITHM_INSTANCE);
    return 0;
}
