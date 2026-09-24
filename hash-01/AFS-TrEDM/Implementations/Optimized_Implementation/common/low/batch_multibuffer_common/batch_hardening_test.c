#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CryptHash_AlgorithmInstance.h"
#include "CryptHash_Batch.h"

/* Function expect_ok: checks that a batch API call succeeds when success is expected. */
static int expect_ok(const char *name, int rc)
{
    if (rc != 0) {
        fprintf(stderr, "%s: expected success, rc=%d\n", name, rc);
        return 1;
    }
    return 0;
}

/* Function expect_fail: checks that a batch API call fails when failure is expected. */
static int expect_fail(const char *name, int rc)
{
    if (rc == 0) {
        fprintf(stderr, "%s: expected failure, rc=0\n", name);
        return 1;
    }
    return 0;
}

/* Function call_batch_width: calls the batch API variant for the requested batch width. */
static int call_batch_width(unsigned width,
                            int digest_len_bits,
                            const unsigned char *const *msg,
                            const unsigned long long *bits,
                            unsigned char *const *digest)
{
    if (width == 4U) {
        return CryptHash_Batch4(digest_len_bits, msg, bits, digest);
    }
    if (width == 8U) {
        return CryptHash_Batch8(digest_len_bits, msg, bits, digest);
    }
    if (width == 16U) {
        return CryptHash_Batch16(digest_len_bits, msg, bits, digest);
    }
    return -99;
}

/* Function test_null_zero_batch_width: tests null/zero-count batch API argument handling. */
static int test_null_zero_batch_width(unsigned width)
{
    const unsigned char *msg[16];
    unsigned long long bits[16];
    unsigned char out[16][DIGEST_BIT_LENGTH / 8U];
    unsigned char *digest[16];
    char name[64];
    unsigned i;

    for (i = 0U; i < 16U; i++) {
        msg[i] = NULL;
        bits[i] = 0ULL;
        memset(out[i], 0, sizeof(out[i]));
        digest[i] = out[i];
    }
    snprintf(name, sizeof(name), "Batch%u NULL+0-bit", width);
    return expect_ok(name, call_batch_width(width, DIGEST_BIT_LENGTH, msg, bits, digest));
}

/* Function test_invalid_batch_width: tests invalid batch API argument handling. */
static int test_invalid_batch_width(unsigned width)
{
    const unsigned char *msg[16];
    unsigned long long bad_bits[16];
    unsigned long long zero_bits[16];
    unsigned char out[16][DIGEST_BIT_LENGTH / 8U];
    unsigned char *digest[16];
    unsigned char *bad_digest[16];
    char name[80];
    int failures = 0;
    unsigned i;

    for (i = 0U; i < 16U; i++) {
        msg[i] = NULL;
        bad_bits[i] = 0ULL;
        zero_bits[i] = 0ULL;
        memset(out[i], 0, sizeof(out[i]));
        digest[i] = out[i];
        bad_digest[i] = out[i];
    }
    bad_bits[0] = 8ULL;
    if (width > 1U) {
        bad_digest[1] = NULL;
    } else {
        bad_digest[0] = NULL;
    }

    snprintf(name, sizeof(name), "Batch%u NULL+nonzero-bit", width);
    failures += expect_fail(name,
                            call_batch_width(width, DIGEST_BIT_LENGTH, msg, bad_bits, digest));
    snprintf(name, sizeof(name), "Batch%u invalid digest length", width);
    failures += expect_fail(name,
                            call_batch_width(width, DIGEST_BIT_LENGTH - 1, msg, zero_bits, digest));
    snprintf(name, sizeof(name), "Batch%u NULL digest", width);
    failures += expect_fail(name,
                            call_batch_width(width, DIGEST_BIT_LENGTH, msg, zero_bits, bad_digest));
    snprintf(name, sizeof(name), "Batch%u NULL msg array", width);
    failures += expect_fail(name,
                            call_batch_width(width, DIGEST_BIT_LENGTH, NULL, zero_bits, digest));
    snprintf(name, sizeof(name), "Batch%u NULL length array", width);
    failures += expect_fail(name,
                            call_batch_width(width, DIGEST_BIT_LENGTH, msg, NULL, digest));
    snprintf(name, sizeof(name), "Batch%u NULL digest array", width);
    failures += expect_fail(name,
                            call_batch_width(width, DIGEST_BIT_LENGTH, msg, zero_bits, NULL));
    return failures;
}

/* Function test_batchmany_api: tests the arbitrary-count batch API wrapper. */
static int test_batchmany_api(void)
{
    const unsigned char *msg[5] = { NULL, NULL, NULL, NULL, NULL };
    const unsigned long long zero_bits[5] = { 0ULL, 0ULL, 0ULL, 0ULL, 0ULL };
    const unsigned long long bad_bits[5] = { 0ULL, 7ULL, 0ULL, 0ULL, 0ULL };
    unsigned char out0[DIGEST_BIT_LENGTH / 8U];
    unsigned char out1[DIGEST_BIT_LENGTH / 8U];
    unsigned char out2[DIGEST_BIT_LENGTH / 8U];
    unsigned char out3[DIGEST_BIT_LENGTH / 8U];
    unsigned char out4[DIGEST_BIT_LENGTH / 8U];
    unsigned char *digest[5] = { out0, out1, out2, out3, out4 };
    unsigned char *bad_digest[5] = { out0, out1, NULL, out3, out4 };
    int failures = 0;

    failures += expect_ok("BatchMany count=0",
                          CryptHash_BatchMany(DIGEST_BIT_LENGTH, 0U, NULL, NULL, NULL));
    failures += expect_ok("BatchMany NULL+0-bit",
                          CryptHash_BatchMany(DIGEST_BIT_LENGTH, 5U, msg, zero_bits, digest));
    failures += expect_fail("BatchMany invalid digest length",
                            CryptHash_BatchMany(DIGEST_BIT_LENGTH + 1, 5U, msg, zero_bits, digest));
    failures += expect_fail("BatchMany NULL+nonzero-bit",
                            CryptHash_BatchMany(DIGEST_BIT_LENGTH, 5U, msg, bad_bits, digest));
    failures += expect_fail("BatchMany NULL digest lane",
                            CryptHash_BatchMany(DIGEST_BIT_LENGTH, 5U, msg, zero_bits, bad_digest));
    failures += expect_fail("BatchMany NULL msg array",
                            CryptHash_BatchMany(DIGEST_BIT_LENGTH, 5U, NULL, zero_bits, digest));
    failures += expect_fail("BatchMany NULL length array",
                            CryptHash_BatchMany(DIGEST_BIT_LENGTH, 5U, msg, NULL, digest));
    failures += expect_fail("BatchMany NULL digest array",
                            CryptHash_BatchMany(DIGEST_BIT_LENGTH, 5U, msg, zero_bits, NULL));
    return failures;
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(void)
{
    int failures = 0;
    failures += test_null_zero_batch_width(4U);
    failures += test_invalid_batch_width(4U);
    failures += test_null_zero_batch_width(8U);
    failures += test_invalid_batch_width(8U);
    failures += test_null_zero_batch_width(16U);
    failures += test_invalid_batch_width(16U);
    failures += test_batchmany_api();
    if (failures != 0) {
        fprintf(stderr, "batch hardening test failed: %d failure(s)\n", failures);
        return 1;
    }
    printf("batch hardening test passed for %s\n", ALGORITHM_INSTANCE);
    return 0;
}
