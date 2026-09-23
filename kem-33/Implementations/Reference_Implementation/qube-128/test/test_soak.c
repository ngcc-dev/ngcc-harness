#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "drng.h"
#include "kem_qube.h"

DRNG_ctx drng_algorithm;

static unsigned long parse_count(int argc, char **argv) {
    char *end = NULL;
    unsigned long count;

    if (argc < 2) {
        return 64;
    }

    count = strtoul(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || count == 0) {
        return 64;
    }
    return count;
}

int main(int argc, char **argv) {
    static const uint8_t seed[64] = {
        0x51, 0x55, 0x42, 0x45, 0x2d, 0x73, 0x6f, 0x61,
        0x6b, 0x2d, 0x73, 0x65, 0x65, 0x64, 0x2d, 0x30,
        0x30, 0x30, 0x31, 0x2d, 0x53, 0x4d, 0x33, 0x2d,
        0x72, 0x65, 0x66, 0x61, 0x63, 0x74, 0x6f, 0x72,
        0x51, 0x55, 0x42, 0x45, 0x2d, 0x73, 0x6f, 0x61,
        0x6b, 0x2d, 0x73, 0x65, 0x65, 0x64, 0x2d, 0x30,
        0x30, 0x30, 0x32, 0x2d, 0x53, 0x4d, 0x33, 0x2d,
        0x72, 0x65, 0x66, 0x61, 0x63, 0x74, 0x6f, 0x72,
    };
    const unsigned long count = parse_count(argc, argv);
    unsigned char *pk = (unsigned char *)calloc(CRYPTO_PUBLICKEYBYTES, 1);
    unsigned char *sk = (unsigned char *)calloc(CRYPTO_SECRETKEYBYTES, 1);
    unsigned char *ct = (unsigned char *)calloc(CRYPTO_CIPHERTEXTBYTES, 1);
    unsigned char *ct_bad = (unsigned char *)calloc(CRYPTO_CIPHERTEXTBYTES, 1);
    unsigned char *ss_enc = (unsigned char *)calloc(CRYPTO_BYTES, 1);
    unsigned char *ss_dec = (unsigned char *)calloc(CRYPTO_BYTES, 1);
    unsigned char *ss_bad = (unsigned char *)calloc(CRYPTO_BYTES, 1);
    unsigned long long pk_len = 0;
    unsigned long long sk_len = 0;
    unsigned long long ct_len = 0;
    unsigned long long ss_len = 0;
    int ret = 1;

    if (pk == NULL || sk == NULL || ct == NULL || ct_bad == NULL ||
        ss_enc == NULL || ss_dec == NULL || ss_bad == NULL) {
        fprintf(stderr, "allocation failed\n");
        goto cleanup;
    }

    if (init_random_number(&drng_algorithm, seed, sizeof seed) != 0) {
        fprintf(stderr, "DRNG init failed\n");
        goto cleanup;
    }

    for (unsigned long i = 0; i < count; i++) {
        if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
            pk_len != CRYPTO_PUBLICKEYBYTES || sk_len != CRYPTO_SECRETKEYBYTES) {
            fprintf(stderr, "kem_keygen failed at iteration %lu\n", i);
            goto cleanup;
        }
        if (kem_enc(pk, pk_len, ss_enc, &ss_len, ct, &ct_len) != 0 ||
            ss_len != CRYPTO_BYTES || ct_len != CRYPTO_CIPHERTEXTBYTES) {
            fprintf(stderr, "kem_enc failed at iteration %lu\n", i);
            goto cleanup;
        }
        if (kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_len) != 0 ||
            ss_len != CRYPTO_BYTES || memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
            fprintf(stderr, "kem_dec mismatch at iteration %lu\n", i);
            goto cleanup;
        }

        memcpy(ct_bad, ct, CRYPTO_CIPHERTEXTBYTES);
        ct_bad[(size_t)i % CRYPTO_CIPHERTEXTBYTES] ^= (unsigned char)(1U << (i & 7U));
        if (kem_dec(sk, sk_len, ct_bad, ct_len, ss_bad, &ss_len) != 0 ||
            ss_len != CRYPTO_BYTES || memcmp(ss_bad, ss_enc, CRYPTO_BYTES) == 0) {
            fprintf(stderr, "tampered decapsulation failed at iteration %lu\n", i);
            goto cleanup;
        }
    }

    printf("%s soak %lu iterations: PASS\n", CRYPTO_ALGNAME, count);
    ret = 0;

cleanup:
    free(ss_bad);
    free(ss_dec);
    free(ss_enc);
    free(ct_bad);
    free(ct);
    free(sk);
    free(pk);
    return ret;
}
