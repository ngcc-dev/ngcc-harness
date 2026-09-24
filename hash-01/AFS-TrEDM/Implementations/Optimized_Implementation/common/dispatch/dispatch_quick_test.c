#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CryptHash_AlgorithmInstance.h"
#include "afs_dispatch.h"

#define MAX_MSG_BYTES 1024U
#define DIGEST_BYTES (DIGEST_BIT_LENGTH / 8)

/* Function fill_msg: fills a deterministic dispatch test message buffer. */
static void fill_msg(unsigned char *msg, size_t n, unsigned seed)
{
    unsigned x = 0x9E3779B9U ^ seed;
    size_t i;

    for (i = 0U; i < n; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        msg[i] = (unsigned char)(x >> 24);
    }
}

/* Function digest_with_force: hashes with a requested dispatch backend for comparison. */
static int digest_with_force(const char *force,
                             unsigned long long bits,
                             const unsigned char *msg,
                             unsigned char digest[DIGEST_BYTES])
{
    int rc;

    if (force != NULL) {
        if (setenv("AFS_TREDM_DISPATCH_FORCE", force, 1) != 0) {
            return -100;
        }
    } else {
        unsetenv("AFS_TREDM_DISPATCH_FORCE");
    }
    memset(digest, 0, DIGEST_BYTES);
    rc = CryptHash_dispatch(DIGEST_BIT_LENGTH, msg, bits, digest);
    unsetenv("AFS_TREDM_DISPATCH_FORCE");
    return rc;
}

/* Function run_case: runs one dispatch quick-test case. */
static int run_case(unsigned long long bits)
{
    unsigned char msg[MAX_MSG_BYTES];
    unsigned char portable[DIGEST_BYTES];
    unsigned char automatic[DIGEST_BYTES];
    unsigned char avx2[DIGEST_BYTES];
    const size_t bytes = (size_t)((bits + 7ULL) >> 3);
    int rc;

    if (bytes > sizeof(msg)) {
        return -1;
    }
    fill_msg(msg, bytes, (unsigned)bits + (unsigned)DIGEST_BIT_LENGTH);

    rc = digest_with_force("portable", bits, msg, portable);
    if (rc != 0) {
        fprintf(stderr, "portable dispatch failed bits=%llu rc=%d\n", bits, rc);
        return -1;
    }
    rc = digest_with_force(NULL, bits, msg, automatic);
    if (rc != 0) {
        fprintf(stderr, "auto dispatch failed bits=%llu rc=%d backend=%s\n",
                bits, rc, CryptHash_dispatch_selected_backend());
        return -1;
    }
    if (memcmp(portable, automatic, DIGEST_BYTES) != 0) {
        fprintf(stderr, "auto dispatch mismatch bits=%llu\n", bits);
        return -1;
    }
    if (CryptHash_dispatch_avx2_available()) {
        rc = digest_with_force("avx2", bits, msg, avx2);
        if (rc != 0) {
            fprintf(stderr, "avx2 dispatch failed bits=%llu rc=%d\n", bits, rc);
            return -1;
        }
        if (memcmp(portable, avx2, DIGEST_BYTES) != 0) {
            fprintf(stderr, "avx2 dispatch mismatch bits=%llu\n", bits);
            return -1;
        }
    }
    return 0;
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(void)
{
    static const unsigned long long cases[] = {
        0ULL, 1ULL, 7ULL, 8ULL, 9ULL, 511ULL, 512ULL, 513ULL,
        767ULL, 768ULL, 769ULL, 1024ULL, 4096ULL, 8192ULL
    };
    size_t i;

    for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); i++) {
        if (run_case(cases[i]) != 0) {
            return 1;
        }
    }
    printf("dispatch quick test passed for %s backend=%s avx2=%s\n",
           ALGORITHM_INSTANCE,
           CryptHash_dispatch_selected_backend(),
           CryptHash_dispatch_avx2_available() ? "yes" : "no");
    return 0;
}
