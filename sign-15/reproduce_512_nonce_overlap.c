/* ATLAS-512 public transcript collector and recovered-key forgery check. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "api.h"
#include "drng.h"
#include "SIG_AlgorithmInstance.h"
#include "packing.h"
#include "auxfunc.h"

#if N != 512 || L != 7 || Q != 33554432U
#error This witness is for the submitted ATLAS-512 parameters only
#endif

#define ROUND_Q_TO_P(a) ((((a) & (Q - 1)) + (1U << (QBITS - PBITS - 1))) >> (QBITS - PBITS))

DRNG_ctx drng_algorithm;

static int centered(uint32_t x) {
    x &= Q - 1;
    return x > Q / 2 ? (int)x - (int)Q : (int)x;
}

static int collect(void) {
    unsigned char seed[64], pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    unsigned long long pk_len = 0, sk_len = 0;
    for (unsigned i = 0; i < sizeof(seed); i++) seed[i] = (unsigned char)(i + 1);
    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) return 1;
    if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0) return 2;
    if (pk_len != CRYPTO_PUBLICKEYBYTES || sk_len != CRYPTO_SECRETKEYBYTES) return 3;

    printf("PARAM %u %u %u\nPK ", (unsigned)N, (unsigned)L, (unsigned)Q);
    for (unsigned i = 0; i < CRYPTO_PUBLICKEYBYTES; i++) printf("%02x", pk[i]);
    putchar('\n');
    for (unsigned sample = 0; sample < 2; sample++) {
        unsigned char msg[16] = {0};
        unsigned char sig[CRYPTO_BYTES + sizeof(msg)];
        unsigned long long sig_len = 0;
        polyvecl z;
        polyveck h;
        poly c;
        msg[0] = (unsigned char)(sample + 1);
        msg[1] = 1;
        if (sig_sign(sk, sk_len, msg, sizeof(msg), sig, &sig_len) != 0) return 4;
        if (sig_len != CRYPTO_BYTES + sizeof(msg)) return 5;
        if (sig_verify(pk, pk_len, sig, CRYPTO_BYTES, msg, sizeof(msg)) != 0) return 6;
        unpack_sig(&z, &h, &c, sig);
        printf("C");
        for (unsigned j = 0; j < N; j++) printf(" %d", centered(c.coeffs[j]));
        printf("\nZ");
        for (unsigned i = 0; i < L; i++)
            for (unsigned j = 0; j < N; j++) printf(" %d", centered(z.vec[i].coeffs[j]));
        putchar('\n');
    }
    return 0;
}

static int forge(void) {
    unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    unsigned char rho[SEEDBYTES], key[SEEDBYTES], tr[CRHBYTES];
    unsigned char msg[16] = {0}, changed[16] = {0};
    unsigned char sig[CRYPTO_BYTES + sizeof(msg)] = {0};
    unsigned long long sig_len = 0;
    polyvecl recovered, mat[K];
    polyveck expected_t1, t, t1, t0;

    for (unsigned i = 0; i < CRYPTO_PUBLICKEYBYTES; i++)
        if (scanf("%2hhx", &pk[i]) != 1) return 21;
    for (unsigned i = 0; i < L; i++)
        for (unsigned j = 0; j < N; j++) {
            int value;
            if (scanf("%d", &value) != 1) return 22;
            recovered.vec[i].coeffs[j] = (uint32_t)(value < 0 ? value + (int)Q : value);
        }
    unpack_pk(rho, &expected_t1, pk);
    expand_mat(mat, rho);
    polyvec_l_mul(mat, &recovered, &t);
    for (unsigned i = 0; i < K; i++)
        for (unsigned j = 0; j < N; j++)
            t.vec[i].coeffs[j] = ROUND_Q_TO_P(t.vec[i].coeffs[j]);
    polyveck_power2round(&t1, &t0, &t);
    for (unsigned i = 0; i < K; i++)
        for (unsigned j = 0; j < N; j++)
            if (t1.vec[i].coeffs[j] != expected_t1.vec[i].coeffs[j]) return 23;

    memset(key, 0x5a, sizeof(key));
    if (pseudoXOF(CRHBYTES * 8, pk, CRYPTO_PUBLICKEYBYTES * 8, tr) != 0) return 24;
    pack_sk(sk, rho, key, tr, &recovered, &t0);
    msg[0] = 0x99;
    memcpy(changed, msg, sizeof(msg));
    changed[0] ^= 1;
    if (sig_sign(sk, sizeof(sk), msg, sizeof(msg), sig, &sig_len) != 0) return 25;
    int accepted = sig_verify(pk, sizeof(pk), sig, CRYPTO_BYTES, msg, sizeof(msg));
    int rejected = sig_verify(pk, sizeof(pk), sig, CRYPTO_BYTES, changed, sizeof(changed));
    printf("public-key consistency=pass; fresh-message verify=%d; changed-message verify=%d (0=accept)\n",
           accepted, rejected);
    return accepted == 0 && rejected != 0 ? 0 : 26;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--collect") == 0) return collect();
    if (argc == 2 && strcmp(argv[1], "--forge") == 0) return forge();
    fprintf(stderr, "usage: %s --collect | --forge\n", argv[0]);
    return 64;
}
