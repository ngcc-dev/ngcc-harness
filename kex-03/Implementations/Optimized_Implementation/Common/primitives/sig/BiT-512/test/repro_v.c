/*
 * BiT reference / AVX2 consistency & correctness harness.
 *
 * Drives the outermost NGCC (国密) API (sig_keygen / sig_sign / sig_verify)
 * using the SAME deterministic seed/message generation as KAT_SIG.c, but with
 * an adjustable iteration count.  For every (level, hash-route, implementation)
 * build it:
 *   - checks sign->verify correctness,
 *   - checks unforgeability (tampered message / signature / public key must be
 *     rejected) — the core 国密 digital-signature requirement,
 *   - checks empty-message and wrong-key-pair handling,
 *   - folds every (pk,sk,sn) into a self-contained SM3 digest chain so that the
 *     Python driver can assert byte-for-byte KAT equality between the reference
 *     and the AVX2 implementation (and between repeated runs).
 *
 * The digest hash is a SELF-CONTAINED SM3 (below) so it is identical across all
 * builds regardless of which symmetric route the implementation under test uses.
 *
 * Build (driven by run_consistency.py): linked against one BiT instance's
 * object files; KAT_SIG.o is excluded because this file provides main() and the
 * drng_algorithm definition the signature scheme expects.
 *
 * Usage: ./consistency_test [count]      (default 100)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SIG_AlgorithmInstance.h"
#include "drng.h"

/* The signature scheme pulls its randomness from this global (extern in sign.c). */
DRNG_ctx drng_algorithm;

/* ---------------------------------------------------------------------------
 * Self-contained SM3 (GB/T 32905-2016) used ONLY to chain a digest of outputs.
 * Constant-time-irrelevant; correctness-only.  Rotate is mask-safe (no UB).
 * ------------------------------------------------------------------------- */
#define SM3_ROTL(a, n) (((uint32_t)(a) << ((n) & 31)) | ((uint32_t)(a) >> ((32u - (n)) & 31)))
#define SM3_P0(x) ((x) ^ SM3_ROTL((x), 9) ^ SM3_ROTL((x), 17))
#define SM3_P1(x) ((x) ^ SM3_ROTL((x), 15) ^ SM3_ROTL((x), 23))

static void sm3_compress(uint32_t v[8], const unsigned char *blk) {
    uint32_t W[68], W1[64], A, B, C, D, E, F, G, H, SS1, SS2, TT1, TT2, T;
    int j;
    for (j = 0; j < 16; j++)
        W[j] = ((uint32_t)blk[4 * j] << 24) | ((uint32_t)blk[4 * j + 1] << 16) |
               ((uint32_t)blk[4 * j + 2] << 8) | (uint32_t)blk[4 * j + 3];
    for (j = 16; j < 68; j++)
        W[j] = SM3_P1(W[j - 16] ^ W[j - 9] ^ SM3_ROTL(W[j - 3], 15)) ^ SM3_ROTL(W[j - 13], 7) ^ W[j - 6];
    for (j = 0; j < 64; j++)
        W1[j] = W[j] ^ W[j + 4];
    A = v[0]; B = v[1]; C = v[2]; D = v[3]; E = v[4]; F = v[5]; G = v[6]; H = v[7];
    for (j = 0; j < 64; j++) {
        T = (j < 16) ? 0x79cc4519u : 0x7a879d8au;
        SS1 = SM3_ROTL(SM3_ROTL(A, 12) + E + SM3_ROTL(T, j & 31), 7);
        SS2 = SS1 ^ SM3_ROTL(A, 12);
        if (j < 16) {
            TT1 = (A ^ B ^ C) + D + SS2 + W1[j];
            TT2 = (E ^ F ^ G) + H + SS1 + W[j];
        } else {
            TT1 = ((A & B) | (A & C) | (B & C)) + D + SS2 + W1[j];
            TT2 = (((F ^ G) & E) ^ G) + H + SS1 + W[j];
        }
        D = C; C = SM3_ROTL(B, 9); B = A; A = TT1;
        H = G; G = SM3_ROTL(F, 19); F = E; E = SM3_P0(TT2);
    }
    v[0] ^= A; v[1] ^= B; v[2] ^= C; v[3] ^= D;
    v[4] ^= E; v[5] ^= F; v[6] ^= G; v[7] ^= H;
}

static void sm3(const unsigned char *msg, size_t len, unsigned char out[32]) {
    uint32_t v[8] = {0x7380166fu, 0x4914b2b9u, 0x172442d7u, 0xda8a0600u,
                     0xa96f30bcu, 0x163138aau, 0xe38dee4du, 0xb0fb0e4eu};
    unsigned char blk[64];
    size_t i, full = len / 64, rem = len % 64;
    uint64_t bits = (uint64_t)len * 8;
    for (i = 0; i < full; i++) sm3_compress(v, msg + i * 64);
    memset(blk, 0, 64);
    memcpy(blk, msg + full * 64, rem);
    blk[rem] = 0x80;
    if (rem >= 56) { sm3_compress(v, blk); memset(blk, 0, 64); }
    for (i = 0; i < 8; i++) blk[56 + i] = (unsigned char)(bits >> (8 * (7 - i)));
    sm3_compress(v, blk);
    for (i = 0; i < 8; i++) {
        out[4 * i]     = (unsigned char)(v[i] >> 24);
        out[4 * i + 1] = (unsigned char)(v[i] >> 16);
        out[4 * i + 2] = (unsigned char)(v[i] >> 8);
        out[4 * i + 3] = (unsigned char)(v[i]);
    }
}

/* digest <- SM3(digest || data) */
static void digest_absorb(unsigned char dig[32], const unsigned char *data, size_t len) {
    unsigned char *buf = (unsigned char *)malloc(32 + len);
    if (!buf) { fprintf(stderr, "OOM in digest_absorb\n"); exit(2); }
    memcpy(buf, dig, 32);
    if (len) memcpy(buf + 32, data, len);
    sm3(buf, 32 + len, dig);
    free(buf);
}

/* ------------------------------------------------------------------------- */

/* KAT-style seed/message DRNGs (mirrors KAT_SIG.c nonce construction). */
#define SEED_LEN_BYTES 64
#define MAX_MSG_BYTES  2048

/* Representative message lengths cycled through the batch (covers empty, tiny,
 * byte-aligned, and multi-KB messages — the 国密 "various message length"
 * requirement).  Both ref and avx2 walk the identical schedule. */
static const size_t MLENS[] = {0, 1, 2, 3, 5, 8, 13, 17, 32, 56, 64, 100,
                               128, 200, 256, 333, 512, 777, 1024, 1500, 2048};
#define NMLENS (sizeof(MLENS) / sizeof(MLENS[0]))

static void kat_seed_drng(DRNG_ctx *drng_seed, DRNG_ctx *drng_msg) {
    unsigned char nonce1[SEED_LEN_BYTES];
    unsigned char nonce2[SEED_LEN_BYTES];
    for (int i = 0; i < SEED_LEN_BYTES / 4; i++) memcpy(nonce1 + 4 * i, "seed", 4);
    memset(nonce2, 0, sizeof(nonce2));
    for (int i = 0; i < SEED_LEN_BYTES / 3; i++) memcpy(nonce2 + 3 * i, "msg", 3);
    memcpy(nonce2 + SEED_LEN_BYTES - 1, "m", 1);
    init_random_number(drng_seed, nonce1, SEED_LEN_BYTES);
    init_random_number(drng_msg, nonce2, SEED_LEN_BYTES);
}

int main(int argc, char **argv) {
    long N = (argc > 1) ? atol(argv[1]) : 100;
    if (N < 1) N = 1;

    unsigned long long pk_len = sig_get_pk_len_bytes();
    unsigned long long sk_len = sig_get_sk_len_bytes();
    unsigned long long sn_len = sig_get_sn_len_bytes();

    unsigned char *pk  = (unsigned char *)malloc(pk_len);
    unsigned char *sk  = (unsigned char *)malloc(sk_len);
    unsigned char *pk2 = (unsigned char *)malloc(pk_len);
    unsigned char *sk2 = (unsigned char *)malloc(sk_len);
    unsigned char *sn  = (unsigned char *)malloc(sn_len);
    unsigned char *seed = (unsigned char *)malloc(SEED_LEN_BYTES);
    unsigned char *m   = (unsigned char *)malloc(MAX_MSG_BYTES);
    if (!pk || !sk || !pk2 || !sk2 || !sn || !seed || !m) {
        fprintf(stderr, "allocation failed\n");
        return 2;
    }

    DRNG_ctx drng_seed, drng_msg;
    kat_seed_drng(&drng_seed, &drng_msg);

    unsigned char digest[32] = {0};
    long signfail = 0, verifyfail = 0, tamperfail = 0, edgefail = 0;
    unsigned long long pl, sl, nl;

    for (long i = 0; i < N; i++) {
        size_t mlen = MLENS[(size_t)i % NMLENS];

        get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8ULL);
        if (mlen) get_random_number(&drng_msg, m, (unsigned long long)mlen * 8ULL);

        /* The scheme's DRNG is reseeded per vector exactly like KAT_SIG.c. */
        if (init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES) != 0) { signfail++; continue; }

        pl = pk_len; sl = sk_len; nl = sn_len;
        if (sig_keygen(pk, &pl, sk, &sl) != 0) { signfail++; continue; }
        if (sig_sign(sk, sl, m, mlen, sn, &nl) != 0) { signfail++; continue; }

        /* (1) correctness: honest signature must verify */
        if (sig_verify(pk, pl, sn, nl, m, mlen) != 0) verifyfail++;

        /* (2) unforgeability: any tamper must be rejected */
        if (mlen > 0) {
            m[mlen / 2] ^= 0x80u;
            if (sig_verify(pk, pl, sn, nl, m, mlen) == 0) tamperfail++;
            m[mlen / 2] ^= 0x80u;
        }
        sn[nl / 3] ^= 0x40u;
        if (sig_verify(pk, pl, sn, nl, m, mlen) == 0) tamperfail++;
        sn[nl / 3] ^= 0x40u;

        pk[pl / 2] ^= 0x55u;
        if (sig_verify(pk, pl, sn, nl, m, mlen) == 0) tamperfail++;
        pk[pl / 2] ^= 0x55u;

        /* (3) fold deterministic KAT outputs into the consistency digest */
        digest_absorb(digest, pk, pl);
        digest_absorb(digest, sk, sl);
        digest_absorb(digest, sn, nl);
    }

    /* Edge case: wrong key pair must be rejected. */
    {
        unsigned char es[SEED_LEN_BYTES];
        memset(es, 0xC1, sizeof(es));
        init_random_number(&drng_algorithm, es, SEED_LEN_BYTES);
        pl = pk_len; sl = sk_len;
        if (sig_keygen(pk, &pl, sk, &sl) != 0) edgefail++;
        memset(es, 0xD2, sizeof(es));
        init_random_number(&drng_algorithm, es, SEED_LEN_BYTES);
        unsigned long long pl2 = pk_len, sl2 = sk_len;
        if (sig_keygen(pk2, &pl2, sk2, &sl2) != 0) edgefail++;
        memset(m, 0x5A, 64);
        nl = sn_len;
        if (sig_sign(sk, sl, m, 64, sn, &nl) != 0) edgefail++;
        if (sig_verify(pk, pl, sn, nl, m, 64) != 0) edgefail++;       /* right key: accept */
        if (sig_verify(pk2, pl2, sn, nl, m, 64) == 0) edgefail++;     /* wrong key: reject */
    }

    printf("N=%ld\n", N);
    printf("SIGNFAIL=%ld\n", signfail);
    printf("VERIFYFAIL=%ld\n", verifyfail);
    printf("TAMPERFAIL=%ld\n", tamperfail);
    printf("EDGEFAIL=%ld\n", edgefail);
    printf("DIGEST=");
    for (int i = 0; i < 32; i++) printf("%02x", digest[i]);
    printf("\n");
    int ok = (signfail == 0 && verifyfail == 0 && tamperfail == 0 && edgefail == 0);
    printf("RESULT=%s\n", ok ? "PASS" : "FAIL");

    free(m); free(seed); free(sn); free(sk2); free(pk2); free(sk); free(pk);
    return ok ? 0 : 1;
}
