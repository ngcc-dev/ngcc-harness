/*
 * CTL KEM correctness self-test.
 *
 * This file keeps only correctness/KAT-style tests:
 *   1) deterministic RNG seed;
 *   2) length-interface checks;
 *   3) keygen -> encapsulation -> decapsulation;
 *   4) shared-secret equality check;
 *   5) full test vectors written to ctl_kem_kat_vectors.txt.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "ctl.h"

DRNG_ctx drng_algorithm;

#define KAT_CASES     3
#define DRNG_SEED_LEN 55

static void make_seed(unsigned char seed[DRNG_SEED_LEN]) {
    int i;
    for (i = 0; i < DRNG_SEED_LEN; i++) {
        seed[i] = (unsigned char)(0xAAU ^ (unsigned int)(i * 13U + 7U));
    }
}

static int checked_size(unsigned long long v, size_t *out, const char *name) {
    if (v == 0ULL || v > (unsigned long long)SIZE_MAX) {
        printf("FAILED: invalid %s length: %llu\n", name, v);
        return -1;
    }
    *out = (size_t)v;
    return 0;
}

static void write_hex(FILE *fp, const char *label,
                      const unsigned char *buf, size_t len) {
    size_t i;

    fprintf(fp, "%s = ", label);
    for (i = 0; i < len; i++) {
        fprintf(fp, "%02X", (unsigned int)buf[i]);
    }
    fprintf(fp, "\n");
}

static void print_hex_preview(const char *label,
                              const unsigned char *buf,
                              size_t len) {
    size_t i;
    size_t preview_len = len < 16U ? len : 16U;

    printf("   %s[0..%zu] = ", label, preview_len == 0U ? 0U : preview_len - 1U);
    for (i = 0; i < preview_len; i++) {
        printf("%02X", (unsigned int)buf[i]);
    }
    if (len > preview_len) {
        printf("...");
    }
    printf("\n");
}

static int run_one_case(int case_id,
                        FILE *kat_file,
                        size_t pk_size,
                        size_t sk_size,
                        size_t ct_size,
                        size_t ss_size,
                        unsigned long long exp_pk_len,
                        unsigned long long exp_sk_len,
                        unsigned long long exp_ct_len,
                        unsigned long long exp_ss_len) {
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ss1 = NULL;
    unsigned char *ss2 = NULL;

    unsigned long long pk_len = 0;
    unsigned long long sk_len = 0;
    unsigned long long ct_len = 0;
    unsigned long long ss1_len = 0;
    unsigned long long ss2_len = 0;

    int ret;

    pk = (unsigned char *)calloc(pk_size, 1U);
    sk = (unsigned char *)calloc(sk_size, 1U);
    ct = (unsigned char *)calloc(ct_size, 1U);
    ss1 = (unsigned char *)calloc(ss_size, 1U);
    ss2 = (unsigned char *)calloc(ss_size, 1U);

    if (pk == NULL || sk == NULL || ct == NULL || ss1 == NULL || ss2 == NULL) {
        printf("FAILED: memory allocation failed in case %d\n", case_id);
        free(pk);
        free(sk);
        free(ct);
        free(ss1);
        free(ss2);
        return 1;
    }

    printf("Case %d: keygen -> encapsulation -> decapsulation\n", case_id);

    ret = kem_keygen(pk, &pk_len, sk, &sk_len);
    if (ret != 0) {
        printf("FAILED: kem_keygen ret=%d\n", ret);
        free(pk);
        free(sk);
        free(ct);
        free(ss1);
        free(ss2);
        return 1;
    }

    if (pk_len != exp_pk_len || sk_len != exp_sk_len) {
        printf("FAILED: key length mismatch. expected pk/sk=%llu/%llu, actual=%llu/%llu\n",
               exp_pk_len, exp_sk_len, pk_len, sk_len);
        free(pk);
        free(sk);
        free(ct);
        free(ss1);
        free(ss2);
        return 1;
    }

    ret = kem_enc(pk, pk_len, ss1, &ss1_len, ct, &ct_len);
    if (ret != 0) {
        printf("FAILED: kem_enc ret=%d\n", ret);
        free(pk);
        free(sk);
        free(ct);
        free(ss1);
        free(ss2);
        return 1;
    }

    if (ct_len != exp_ct_len || ss1_len != exp_ss_len) {
        printf("FAILED: encapsulation length mismatch. expected ct/ss=%llu/%llu, actual=%llu/%llu\n",
               exp_ct_len, exp_ss_len, ct_len, ss1_len);
        free(pk);
        free(sk);
        free(ct);
        free(ss1);
        free(ss2);
        return 1;
    }

    ret = kem_dec(sk, sk_len, ct, ct_len, ss2, &ss2_len);
    if (ret != 0) {
        printf("FAILED: kem_dec ret=%d\n", ret);
        free(pk);
        free(sk);
        free(ct);
        free(ss1);
        free(ss2);
        return 1;
    }

    if (ss2_len != exp_ss_len) {
        printf("FAILED: decapsulation shared-secret length mismatch. expected=%llu, actual=%llu\n",
               exp_ss_len, ss2_len);
        free(pk);
        free(sk);
        free(ct);
        free(ss1);
        free(ss2);
        return 1;
    }

    if (ss1_len != ss2_len || memcmp(ss1, ss2, ss_size) != 0) {
        printf("FAILED: shared secrets differ in case %d\n", case_id);
        print_hex_preview("ss_enc", ss1, ss_size);
        print_hex_preview("ss_dec", ss2, ss_size);
        free(pk);
        free(sk);
        free(ct);
        free(ss1);
        free(ss2);
        return 1;
    }

    printf("   lengths: pk=%llu, sk=%llu, ct=%llu, ss=%llu\n",
           pk_len, sk_len, ct_len, ss1_len);
    print_hex_preview("pk", pk, pk_size);
    print_hex_preview("ct", ct, ct_size);
    print_hex_preview("ss", ss1, ss_size);
    printf("   OK\n");

    if (kat_file != NULL) {
        fprintf(kat_file, "case = %d\n", case_id);
        fprintf(kat_file, "pk_len = %llu\n", pk_len);
        fprintf(kat_file, "sk_len = %llu\n", sk_len);
        fprintf(kat_file, "ct_len = %llu\n", ct_len);
        fprintf(kat_file, "ss_len = %llu\n", ss1_len);
        write_hex(kat_file, "pk", pk, pk_size);
        write_hex(kat_file, "sk", sk, sk_size);
        write_hex(kat_file, "ct", ct, ct_size);
        write_hex(kat_file, "ss_enc", ss1, ss_size);
        write_hex(kat_file, "ss_dec", ss2, ss_size);
        fprintf(kat_file, "result = PASS\n\n");
    }

    free(pk);
    free(sk);
    free(ct);
    free(ss1);
    free(ss2);

    return 0;
}

int main(void) {
    unsigned char seed[DRNG_SEED_LEN];
    FILE *kat_file = NULL;

    unsigned long long exp_pk_len = kem_get_pk_len_bytes();
    unsigned long long exp_sk_len = kem_get_sk_len_bytes();
    unsigned long long exp_ct_len = kem_get_ct_len_bytes();
    unsigned long long exp_ss_len = kem_get_ss_len_bytes();

    size_t pk_size = 0;
    size_t sk_size = 0;
    size_t ct_size = 0;
    size_t ss_size = 0;

    int ret;
    int case_id;

    printf("CTL KEM correctness self-test\n");
    printf("=============================\n");
    printf("Algorithm name     : CTL-3329-2048\n\n");

    if (checked_size(exp_pk_len, &pk_size, "pk") != 0 ||
        checked_size(exp_sk_len, &sk_size, "sk") != 0 ||
        checked_size(exp_ct_len, &ct_size, "ct") != 0 ||
        checked_size(exp_ss_len, &ss_size, "ss") != 0) {
        return 1;
    }

    printf("Length interface check\n");
    printf("----------------------\n");
    printf("pk_len = %llu bytes\n", exp_pk_len);
    printf("sk_len = %llu bytes\n", exp_sk_len);
    printf("ct_len = %llu bytes\n", exp_ct_len);
    printf("ss_len = %llu bytes\n\n", exp_ss_len);

    make_seed(seed);
    ret = init_random_number(&drng_algorithm, seed, DRNG_SEED_LEN);
    if (ret != 0) {
        printf("FAILED: init_random_number ret=%d\n", ret);
        return 1;
    }

    kat_file = fopen("ctl_kem_kat_vectors.txt", "w");
    if (kat_file == NULL) {
        printf("Warning: cannot write ctl_kem_kat_vectors.txt; console test continues.\n");
    } else {
        int i;
        fprintf(kat_file, "algorithm = CTL-3329-2048\n");
        fprintf(kat_file, "seed_len = %d\n", DRNG_SEED_LEN);
        fprintf(kat_file, "initial_seed = ");
        for (i = 0; i < DRNG_SEED_LEN; i++) {
            fprintf(kat_file, "%02X", (unsigned int)seed[i]);
        }
        fprintf(kat_file, "\n\n");
    }

    for (case_id = 1; case_id <= KAT_CASES; case_id++) {
        if (run_one_case(case_id, kat_file,
                         pk_size, sk_size, ct_size, ss_size,
                         exp_pk_len, exp_sk_len, exp_ct_len, exp_ss_len) != 0) {
            if (kat_file != NULL) {
                fclose(kat_file);
            }
            return 1;
        }
    }

    if (kat_file != NULL) {
        fclose(kat_file);
    }

    printf("\nAll CTL KEM correctness tests passed.\n");
    printf("KAT vector file: ctl_kem_kat_vectors.txt\n");
    return 0;
}
