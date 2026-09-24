/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"
#include <string.h>


/* --- 1. Permutation Declaration and Definition -*/

#define TOTAL_ROUNDS 16
/* Round Constants */
static const u64 RC[16] = {
    0x0FULL, 0x1EULL, 0x2DULL, 0x3CULL,
    0x4BULL, 0x5AULL, 0x69ULL, 0x78ULL,
    0x87ULL, 0x96ULL, 0xA5ULL, 0xB4ULL,
    0xC3ULL, 0xD2ULL, 0xE1ULL, 0xF0ULL
};

/* Rotation Constants */
static const int ra = 1, rb = 10, rc = 24, rd = 33, re = 49;

/* Left Rotation Function */
static u64 ROTL64(u64 x, int shift) {
    return ((x << shift) | (x >> (64 - shift)));
}

/* Core Update Function */
static void Update(u64 *a, u64 *b, u64 *c, u64 *d, u64 *e) {
    *b += *e;
    *d = ROTL64(*d, rd);
    *d ^= *b;
    *a += *d;
    *c = ROTL64(*c, rc);
    *c ^= *a;
    *e += *c;
    *b = ROTL64(*b, rb);
    *b ^= *e;
    *d += *b;
    *a = ROTL64(*a, ra);
    *a ^= *d;
    *c += *a;
    *e = ROTL64(*e, re);
    *e ^= *c;
}

/* Phi */
static void phi(u64 S[5][5]) {
    Update(&S[0][0], &S[1][1], &S[2][2], &S[3][3], &S[4][4]);
    Update(&S[0][1], &S[1][2], &S[2][3], &S[3][4], &S[4][0]);
    Update(&S[0][2], &S[1][3], &S[2][4], &S[3][0], &S[4][1]);
    Update(&S[0][3], &S[1][4], &S[2][0], &S[3][1], &S[4][2]);
    Update(&S[0][4], &S[1][0], &S[2][1], &S[3][2], &S[4][3]);

    Update(&S[0][0], &S[0][1], &S[0][2], &S[0][3], &S[0][4]);
    Update(&S[1][0], &S[1][1], &S[1][2], &S[1][3], &S[1][4]);
    Update(&S[2][0], &S[2][1], &S[2][2], &S[2][3], &S[2][4]);
    Update(&S[3][0], &S[3][1], &S[3][2], &S[3][3], &S[3][4]);
    Update(&S[4][0], &S[4][1], &S[4][2], &S[4][3], &S[4][4]);
}

/* Iota */
static void iota(u64 S[5][5], int r) {
    S[0][0] ^= RC[r];
}

/* Permutation */
void permutation(u64 S[5][5]) {
    for (int i = 0; i < TOTAL_ROUNDS; i++) {
        iota(S, i);
        phi(S);
    }
}

static u64 load64_be(const unsigned char *b) {
    return ((u64)b[0] << 56) | ((u64)b[1] << 48) | ((u64)b[2] << 40) | ((u64)b[3] << 32) |
           ((u64)b[4] << 24) | ((u64)b[5] << 16) | ((u64)b[6] << 8)  | ((u64)b[7]);
}

static void store64_be(unsigned char *b, u64 w) {
    b[0] = (unsigned char)(w >> 56); b[1] = (unsigned char)(w >> 48);
    b[2] = (unsigned char)(w >> 40); b[3] = (unsigned char)(w >> 32);
    b[4] = (unsigned char)(w >> 24); b[5] = (unsigned char)(w >> 16);
    b[6] = (unsigned char)(w >> 8);  b[7] = (unsigned char)w;
}

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{   
    unsigned long long rem_bits = msg_len_bits;
    int i;
    u64 S[IV_LANE_NUM];
    
    /* --- 1. Initialize Phase --- */
    u64 X[5][5] = {0};
    for (i = 0; i < IV_LANE_NUM; i++) {
        X[i % 5][i / 5] = IV[IV_LANE_NUM - 1 - i];
    }

    /* --- 2. Absorbing Phase --- */
    while (rem_bits >= RATE_BITS) {
        for (i = 0; i < IV_LANE_NUM; i++) {
            S[IV_LANE_NUM - 1 - i] = X[i % 5][i / 5];
        }
        for (i = 0; i < MSG_LANE_NUM; i++) {
           int idx = 24 - i;
            X[idx % 5][idx / 5] ^= load64_be(msg + (i * 8));
        }

        permutation(X);

        for (i = 0; i < IV_LANE_NUM; i++) {
            X[i % 5][i / 5] ^= S[IV_LANE_NUM - 1 - i];
        }
        msg += RATE_BYTES;
        rem_bits -= RATE_BITS;
    }

    /* --- 3. Padding pd10* --- */
    unsigned char pad_block[RATE_BYTES] = {0};
    unsigned int full_bytes = (unsigned int)(rem_bits / 8);
    unsigned int partial_bits = (unsigned int)(rem_bits % 8);
    
    if (full_bytes > 0) {
        memcpy(pad_block, msg, full_bytes);
    }

    if (partial_bits > 0) {
        pad_block[full_bytes] = msg[full_bytes] & (0xFFU << (8 - partial_bits));
        pad_block[full_bytes] |= (0x80U >> partial_bits);
    } else {
        pad_block[full_bytes] = 0x80U;
    }

    /* --- 4. last block ---
     * Case 1: rem || 1 || 0* fits in one rate block.
     * Case 2: rem || 1 exactly fills one rate block; absorb it first, then
     *         absorb one final all-zero rate block.
     * Case 3: rem_bits == 0, i.e. the original message length is a multiple
     *         of the rate; the final block is 1 || 0^{r-1}.
     *
     * The final-domain constant X[0][0] ^= 0x01ULL is applied only to the
     * final message block.
     */

    int need_extra_zero_block = (rem_bits == RATE_BITS - 1ULL);

    for (i = 0; i < MSG_LANE_NUM; i++) {
        int idx = 24 - i;
        X[idx % 5][idx / 5] ^= load64_be(pad_block + (i * 8));
    }

    X[0][0] ^= (!need_extra_zero_block);

    for (i = 0; i < IV_LANE_NUM; i++) {
        S[IV_LANE_NUM - 1 - i] = X[i % 5][i / 5];
    }

    permutation(X);

    for (i = 0; i < IV_LANE_NUM; i++) {
        X[i % 5][i / 5] ^= S[IV_LANE_NUM - 1 - i];
    }

    if (need_extra_zero_block) {
        /* The extra final block is 0^r, so the rate-part XOR is a no-op. */
        X[0][0] ^= 0x01ULL;

        for (i = 0; i < IV_LANE_NUM; i++) {
            S[IV_LANE_NUM - 1 - i] = X[i % 5][i / 5];
        }

        permutation(X);

        for (i = 0; i < IV_LANE_NUM; i++) {
            X[i % 5][i / 5] ^= S[IV_LANE_NUM - 1 - i];
        }
    }
    /* --- 5. Squeezing Phase --- */ 
    for (i = 0; i < DIGEST_LANE_NUM; i++) {
        int idx = DIGEST_LANE_NUM - 1 - i;
        store64_be(digest + (i * 8), X[idx % 5][idx / 5]);
    }

    return 0;
}
