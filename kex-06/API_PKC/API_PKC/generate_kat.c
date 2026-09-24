/*
 * MAMBA-NIKE KAT Generator — produces .req / .rsp for NGCC submission
 *
 * Build (example for NIKE-128):
 *   gcc -O3 -I. -I../src -DNIKE_LEVEL=128 \
 *       -DPARAM_N=1024 -DPARAM_Q=8192 -DLOG2Q=13 \
 *       -DPARAM_T_PK=12 -DPARAM_T_U=12 -DPARAM_T_V=6 \
 *       generate_kat.c ../src/crypto_stream_chacha20.c ../src/poly.c \
 *       ../src/toom.c ../src/error_correction.c ../src/nike.c \
 *       ../src/reduce.c ../src/fips202.c drng.c -o generate_kat_128
 *
 * Output format (NGCC-style):
 *   Count = 0
 *   Seed_Len = 64
 *   Seed = <hex>
 *   PKa_Len = ...
 *   PKa = <hex>
 *   SKa_Len = POLY_BYTES + NIKE_SENDABYTES
 *   SKa = sk_poly || PKa
 *   M1_Len = ...
 *   M1 = <hex>
 *   SS_Len = NIKE_SSBYTES
 *   SS = <hex>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "params.h"
#include "nike.h"

#define SEED_LEN_BYTES 64
#define KAT_COUNT      10
#define SK_BYTES       (POLY_BYTES + NIKE_SENDABYTES)

/* DRNG context for the protocol (initialized per-test with the KAT seed) */
DRNG_ctx drng_algorithm;

/* Override randombytes to use DRNG */
void randombytes(unsigned char *x, unsigned long long xlen) {
    get_random_number(&drng_algorithm, x, xlen * 8);
}

static void fprintstr(FILE *f, const char *label, const unsigned char *data, int len) {
    fprintf(f, "%s", label);
    for (int i = 0; i < len; i++)
        fprintf(f, "%02X", data[i]);
    fprintf(f, "\n");
}

int main(void) {
    unsigned char nonce[SEED_LEN_BYTES];
    unsigned char seed[SEED_LEN_BYTES];
    poly sk_a;
    unsigned char key_a[NIKE_SSBYTES], key_b[NIKE_SSBYTES];
    unsigned char senda[NIKE_SENDABYTES];
    unsigned char sendb[NIKE_SENDBBYTES];

    /* Init seed-DRNG with fixed nonce (matches NGCC template) */
    memset(nonce, 0, sizeof(nonce));
    for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
        memcpy(nonce + 4 * i, "seed", 4);

    DRNG_ctx drng_seed;
    init_random_number(&drng_seed, nonce, SEED_LEN_BYTES);

    printf("=== MAMBA-NIKE KAT (n=%d, q=%d) ===\n", PARAM_N, PARAM_Q);
    printf("pk=%d sk=%d m1=%d ss=%d\n", NIKE_SENDABYTES, SK_BYTES, NIKE_SENDBBYTES, NIKE_SSBYTES);

    for (int count = 0; count < KAT_COUNT; count++) {
        /* Generate deterministic seed */
        get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);

        /* Initialize protocol DRNG with this seed */
        init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

        /* Run NIKE protocol */
        nike_keygen(senda, &sk_a);
        nike_sharedb(key_b, sendb, senda);
        nike_shareda(key_a, &sk_a, senda, sendb);

        /* Verify correctness */
        if (memcmp(key_a, key_b, NIKE_SSBYTES) != 0) {
            fprintf(stderr, "ERROR: key mismatch at count %d\n", count);
            return 1;
        }

        /* Output in NGCC KAT format */
        printf("Count = %d\n", count);
        printf("Seed_Len = %d\n", SEED_LEN_BYTES);
        fprintstr(stdout, "Seed = ", seed, SEED_LEN_BYTES);
        printf("PKa_Len = %d\n", NIKE_SENDABYTES);
        fprintstr(stdout, "PKa = ", senda, NIKE_SENDABYTES);
        printf("SKa_Len = %d\n", SK_BYTES);

        /* Serialize sk as sk_poly || own public key, matching the NGCC KEX API. */
        unsigned char sk_bytes[SK_BYTES];
        for (int i = 0; i < PARAM_N; i++) {
            uint16_t t = sk_a.coeffs[i] & (PARAM_Q - 1);
            sk_bytes[2*i]   = t & 0xFF;
            sk_bytes[2*i+1] = (t >> 8) & 0xFF;
        }
        memcpy(sk_bytes + POLY_BYTES, senda, NIKE_SENDABYTES);
        fprintstr(stdout, "SKa = ", sk_bytes, SK_BYTES);

        printf("M1_Len = %d\n", NIKE_SENDBBYTES);
        fprintstr(stdout, "M1 = ", sendb, NIKE_SENDBBYTES);
        printf("SS_Len = %d\n", NIKE_SSBYTES);
        fprintstr(stdout, "SS = ", key_a, NIKE_SSBYTES);
        printf("\n");
    }

    return 0;
}
