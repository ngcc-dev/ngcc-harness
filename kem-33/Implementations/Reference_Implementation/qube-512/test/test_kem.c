#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "drng.h"
#include "kem_qube.h"
#include "parameters.h"

DRNG_ctx drng_algorithm;

static int expect_len(const char *name, unsigned long long got, unsigned long long want) {
    if (got != want) {
        fprintf(stderr, "%s length mismatch: got %llu, want %llu\n", name, got, want);
        return 1;
    }
    return 0;
}

int main(void) {
    static const uint8_t seed[64] = {
        0x51, 0x55, 0x42, 0x45, 0x2d, 0x53, 0x4d, 0x33,
        0x2d, 0x72, 0x65, 0x66, 0x61, 0x63, 0x74, 0x6f,
        0x72, 0x2d, 0x74, 0x65, 0x73, 0x74, 0x2d, 0x73,
        0x65, 0x65, 0x64, 0x2d, 0x30, 0x30, 0x30, 0x31,
        0x51, 0x55, 0x42, 0x45, 0x2d, 0x53, 0x4d, 0x33,
        0x2d, 0x72, 0x65, 0x66, 0x61, 0x63, 0x74, 0x6f,
        0x72, 0x2d, 0x74, 0x65, 0x73, 0x74, 0x2d, 0x73,
        0x65, 0x65, 0x64, 0x2d, 0x30, 0x30, 0x30, 0x32,
    };
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ct_bad = NULL;
    unsigned char *ss1 = NULL;
    unsigned char *ss2 = NULL;
    unsigned char *ss_bad1 = NULL;
    unsigned char *ss_bad2 = NULL;
    unsigned long long pk_len = 0;
    unsigned long long sk_len = 0;
    unsigned long long ct_len = 0;
    unsigned long long ss_len = 0;
    int ret = 1;

    if (expect_len("pk", kem_get_pk_len_bytes(), CRYPTO_PUBLICKEYBYTES) ||
        expect_len("sk", kem_get_sk_len_bytes(), CRYPTO_SECRETKEYBYTES) ||
        expect_len("ct", kem_get_ct_len_bytes(), CRYPTO_CIPHERTEXTBYTES) ||
        expect_len("ss", kem_get_ss_len_bytes(), CRYPTO_BYTES)) {
        return 1;
    }

    pk = (unsigned char *)calloc(CRYPTO_PUBLICKEYBYTES, 1);
    sk = (unsigned char *)calloc(CRYPTO_SECRETKEYBYTES, 1);
    ct = (unsigned char *)calloc(CRYPTO_CIPHERTEXTBYTES, 1);
    ct_bad = (unsigned char *)calloc(CRYPTO_CIPHERTEXTBYTES, 1);
    ss1 = (unsigned char *)calloc(CRYPTO_BYTES, 1);
    ss2 = (unsigned char *)calloc(CRYPTO_BYTES, 1);
    ss_bad1 = (unsigned char *)calloc(CRYPTO_BYTES, 1);
    ss_bad2 = (unsigned char *)calloc(CRYPTO_BYTES, 1);
    if (pk == NULL || sk == NULL || ct == NULL || ct_bad == NULL || ss1 == NULL || ss2 == NULL ||
        ss_bad1 == NULL || ss_bad2 == NULL) {
        fprintf(stderr, "allocation failed\n");
        goto cleanup;
    }

    if (init_random_number(&drng_algorithm, seed, sizeof seed) != 0) {
        fprintf(stderr, "DRNG init failed\n");
        goto cleanup;
    }

    if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
        pk_len != CRYPTO_PUBLICKEYBYTES || sk_len != CRYPTO_SECRETKEYBYTES) {
        fprintf(stderr, "kem_keygen failed\n");
        goto cleanup;
    }

    if (kem_enc(pk, pk_len, ss1, &ss_len, ct, &ct_len) != 0 ||
        ss_len != CRYPTO_BYTES || ct_len != CRYPTO_CIPHERTEXTBYTES) {
        fprintf(stderr, "kem_enc failed\n");
        goto cleanup;
    }

    if (kem_dec(sk, sk_len, ct, ct_len, ss2, &ss_len) != 0 ||
        ss_len != CRYPTO_BYTES || memcmp(ss1, ss2, CRYPTO_BYTES) != 0) {
        fprintf(stderr, "kem_dec failed to recover shared secret\n");
        goto cleanup;
    }

    memcpy(ct_bad, ct, CRYPTO_CIPHERTEXTBYTES);
    ct_bad[0] ^= 1U;
    if (kem_dec(sk, sk_len, ct_bad, ct_len, ss_bad1, &ss_len) != 0 || ss_len != CRYPTO_BYTES) {
        fprintf(stderr, "tampered ciphertext decapsulation returned API failure\n");
        goto cleanup;
    }
    if (memcmp(ss_bad1, ss1, CRYPTO_BYTES) == 0) {
        fprintf(stderr, "tampered ciphertext did not use fallback key\n");
        goto cleanup;
    }
    if (kem_dec(sk, sk_len, ct_bad, ct_len, ss_bad2, &ss_len) != 0 ||
        memcmp(ss_bad1, ss_bad2, CRYPTO_BYTES) != 0) {
        fprintf(stderr, "fallback key is not deterministic\n");
        goto cleanup;
    }

    printf("%s lengths pk=%d sk=%d ct=%d ss=%d: PASS\n", CRYPTO_ALGNAME,
           CRYPTO_PUBLICKEYBYTES, CRYPTO_SECRETKEYBYTES, CRYPTO_CIPHERTEXTBYTES, CRYPTO_BYTES);
    ret = 0;

cleanup:
    free(ss_bad2);
    free(ss_bad1);
    free(ss2);
    free(ss1);
    free(ct_bad);
    free(ct);
    free(sk);
    free(pk);
    return ret;
}
