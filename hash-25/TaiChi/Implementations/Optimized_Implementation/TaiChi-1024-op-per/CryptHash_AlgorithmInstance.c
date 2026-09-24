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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define TAICHI_INSTANCE 1024

#if TAICHI_INSTANCE == 512
    #define RATE_BITS    1408
    #define DIGEST_BYTES 64
#elif TAICHI_INSTANCE == 768
    #define RATE_BITS    1152
    #define DIGEST_BYTES 96
#elif TAICHI_INSTANCE == 1024
    #define RATE_BITS    896
    #define DIGEST_BYTES 128
#else
    #error "Unsupported TaiChi instance"
#endif

#define RATE_BYTES     (RATE_BITS / 8)
#define RATE_WORDS     (RATE_BITS / 64)
#define CAPACITY_WORDS ((1920 - RATE_BITS) / 64) 

typedef enum {
    DOMAIN_AB  = 1,
    DOMAIN_FIN = 2,
    DOMAIN_SQ  = 3
} DOMAIN;

/* 64-bit rotate left – branchless, safe for n=0..63 */
static inline uint64_t ROTL64(uint64_t x, unsigned int n) {
    return (x << n) | (x >> ((64 - n) & 63));
}

/* ---------- 1. MRM layer (per column) ---------- */
static inline void MRM_COLUMN(uint64_t s[30], int j) {
    uint64_t y0 = s[5*0 + j];
    uint64_t y1 = s[5*1 + j];
    uint64_t y2 = s[5*2 + j];
    uint64_t y3 = s[5*3 + j];
    uint64_t y4 = s[5*4 + j];
    uint64_t y5 = s[5*5 + j];

    uint64_t y6  = y0 ^ ROTL64(y1,  8);
    uint64_t y7  = y1 ^ y2;
    uint64_t y8  = y2 ^ y4;
    uint64_t y9  = y4 ^ y3;
    uint64_t y10 = y3 ^ y5;
    uint64_t y11 = y5 ^ y6;
    uint64_t y12 = y6 ^ ROTL64(y0, 23);
    uint64_t y13 = y7 ^ ROTL64(y11,62);
    uint64_t y14 = y13 ^ ROTL64(y8, 35);
    uint64_t y15 = y14 ^ ROTL64(y9, 14);
    uint64_t y16 = y15 ^ ROTL64(y10,48);
    uint64_t y17 = y16 ^ ROTL64(y12, 1);
    uint64_t y18 = y15 ^ ROTL64(y17,57);
    uint64_t y19 = y18 ^ ROTL64(y14,63);
    uint64_t y20 = y11 ^ ROTL64(y19,58);
    uint64_t y21 = y20 ^ ROTL64(y13,22);

    s[5*0 + j] = y16;
    s[5*1 + j] = ROTL64(y17,15);
    s[5*2 + j] = ROTL64(y18,48);
    s[5*3 + j] = ROTL64(y19,63);
    s[5*4 + j] = ROTL64(y20, 6);
    s[5*5 + j] = ROTL64(y21,33);
}

/* ---------- 2. S-box layer (per row) ---------- */
static inline void SBOX_ROW(uint64_t s[30], int i) {
    uint64_t a0 = s[5*i + 0];
    uint64_t a1 = s[5*i + 1];
    uint64_t a2 = s[5*i + 2];
    uint64_t a3 = s[5*i + 3];
    uint64_t a4 = s[5*i + 4];

    s[5*i + 0] = a1 ^ (a0 & a1) ^ a2 ^ (a1 & a2) ^ a3 ^ (a3 & a4);
    s[5*i + 1] = ~0ULL ^ a1 ^ (a0 & a3) ^ (a1 & a3) ^ a4 ^ (a2 & a4);
    s[5*i + 2] = (a1 & a2) ^ a3 ^ (a2 & a3) ^ a4 ^ (a0 & a4);
    s[5*i + 3] = (a0 & a2) ^ (a1 & a3) ^ (a2 & a3) ^ a4;
    s[5*i + 4] = a0 ^ (a2 & a3) ^ (a1 & a4);
}

/* ---------- 1.6 P1920 permutation---------- */
static uint64_t RC[120] = { 0x0000000000000001ULL,0x0000000000000004ULL,0x0000000000000008ULL,0x0000000000000040ULL,0x0000000000002000ULL,
                            0x0000000010000000ULL,0x0800000000000000ULL,0x0000000000000005ULL,0x000000000000000cULL,0x0000000000000048ULL,
                            0x0000000000002040ULL,0x0000000010002000ULL,0x0800000010000000ULL,0x0800000000000005ULL,0x0000000000000009ULL,
                            0x0000000000000044ULL,0x0000000000002008ULL,0x0000000010000040ULL,0x0800000000002000ULL,0x0000000010000005ULL,
                            0x080000000000000cULL,0x000000000000004dULL,0x000000000000204cULL,0x0000000010002048ULL,0x0800000010002040ULL,
                            0x0800000010002005ULL,0x0800000010000009ULL,0x0800000000000041ULL,0x0000000000002001ULL,0x0000000010000004ULL,
                            0x0800000000000008ULL,0x0000000000000045ULL,0x000000000000200cULL,0x0000000010000048ULL,0x0800000000002040ULL,
                            0x0000000010002005ULL,0x080000001000000cULL,0x080000000000004dULL,0x0000000000002049ULL,0x0000000010002044ULL,
                            0x0800000010002008ULL,0x0800000010000045ULL,0x0800000000002009ULL,0x0000000010000041ULL,0x0800000000002004ULL,
                            0x000000001000000dULL,0x080000000000004cULL,0x000000000000204dULL,0x000000001000204cULL,0x0800000010002048ULL,
                            0x0800000010002045ULL,0x0800000010002009ULL,0x0800000010000041ULL,0x0800000000002001ULL,0x0000000010000001ULL,
                            0x0800000000000004ULL,0x000000000000000dULL,0x000000000000004cULL,0x0000000000002048ULL,0x0000000010002040ULL,
                            0x0800000010002000ULL,0x0800000010000005ULL,0x0800000000000009ULL,0x0000000000000041ULL,0x0000000000002004ULL,
                            0x0000000010000008ULL,0x0800000000000040ULL,0x0000000000002005ULL,0x000000001000000cULL,0x0800000000000048ULL,
                            0x0000000000002045ULL,0x000000001000200cULL,0x0800000010000048ULL,0x0800000000002045ULL,0x0000000010002009ULL,
                            0x0800000010000044ULL,0x080000000000200dULL,0x0000000010000049ULL,0x0800000000002044ULL,0x000000001000200dULL,
                            0x080000001000004cULL,0x080000000000204dULL,0x0000000010002049ULL,0x0800000010002044ULL,0x080000001000200dULL,
                            0x0800000010000049ULL,0x0800000000002041ULL,0x0000000010002001ULL,0x0800000010000004ULL,0x080000000000000dULL,
                            0x0000000000000049ULL,0x0000000000002044ULL,0x0000000010002008ULL,0x0800000010000040ULL,0x0800000000002005ULL,
                            0x0000000010000009ULL,0x0800000000000044ULL,0x000000000000200dULL,0x000000001000004cULL,0x0800000000002048ULL,
                            0x0000000010002045ULL,0x080000001000200cULL,0x080000001000004dULL,0x0800000000002049ULL,0x0000000010002041ULL,
                            0x0800000010002004ULL,0x080000001000000dULL,0x0800000000000049ULL,0x0000000000002041ULL,0x0000000010002004ULL,
                            0x0800000010000008ULL,0x0800000000000045ULL,0x0000000000002009ULL,0x0000000010000044ULL,0x0800000000002008ULL,
                            0x0000000010000045ULL,0x080000000000200cULL,0x000000001000004dULL,0x080000000000204cULL,0x000000001000204dULL };
static void P1920(uint64_t A[30], int b) {
    for (int t = 0; t < 12; t++) {
        /* 1. MRM layer */
        MRM_COLUMN(A, 0);
        MRM_COLUMN(A, 1);
        MRM_COLUMN(A, 2);
        MRM_COLUMN(A, 3);
        MRM_COLUMN(A, 4);

        /* 2. S-box layer */
        SBOX_ROW(A, 0);
        SBOX_ROW(A, 1);
        SBOX_ROW(A, 2);
        SBOX_ROW(A, 3);
        SBOX_ROW(A, 4);
        SBOX_ROW(A, 5);

        /* 3. K2 lane permutation */
        uint64_t temp;
        temp = A[0];
        A[0]  = A[27]; A[27] = A[8];  A[8]  = A[3];
        A[3]  = A[18]; A[18] = A[4];  A[4]  = A[15];
        A[15] = A[13]; A[13] = A[19]; A[19] = A[1];
        A[1]  = A[24]; A[24] = A[17]; A[17] = A[7];
        A[7]  = A[6];  A[6]  = A[9];  A[9]  = temp;

        temp = A[2];
        A[2]  = A[21]; A[21] = A[26]; A[26] = A[11];
        A[11] = A[25]; A[25] = A[14]; A[14] = A[16];
        A[16] = A[10]; A[10] = A[28]; A[28] = A[5];
        A[5]  = A[12]; A[12] = A[22]; A[22] = A[23];
        A[23] = A[20]; A[20] = A[29]; A[29] = temp;
        
        /* 4. Add round constants  */
        for(int j = 0;j < 5;j ++){
            int q;
            q = (b-1)*60 + 5*t + j;
            A[j] ^= RC[q];
        }
    }
}

/* 1.6 P1 and P2 */
static void TaiChi_P1(uint64_t state[30]){ 
    P1920(state, 1); 
}
static void TaiChi_P2(uint64_t state[30]){ 
    P1920(state, 2); 
}

/* ---------- 1.4 TaiChi Step ---------- */
static void TaiChi_Step(uint64_t S[RATE_WORDS],uint64_t L[CAPACITY_WORDS],uint64_t R[CAPACITY_WORDS],uint64_t M[RATE_WORDS],DOMAIN D){
    uint64_t state[30] = {0};
    uint64_t B[CAPACITY_WORDS] = {0};
    int i;

    for(i = 0; i < RATE_WORDS; i++) {
        state[i] = S[i] ^ M[i];
    }
    for(i = 0; i < CAPACITY_WORDS; i++) {
        state[RATE_WORDS + i] = L[i];
    }
    state[29] ^= (uint64_t)D;
    TaiChi_P1(state);

    memcpy(B, state + RATE_WORDS, CAPACITY_WORDS * sizeof(uint64_t));
    for(i = 0; i < CAPACITY_WORDS; i++) {
        state[RATE_WORDS + i] = R[i] ^ B[i];
    }
    TaiChi_P2(state);

    for(i = 0; i < RATE_WORDS; i++) {
        S[i] = state[i];
    }

    for(i = 0; i < CAPACITY_WORDS; i++) {
        L[i] = state[RATE_WORDS + i];
        R[i] = B[i] ^ state[RATE_WORDS + i];
    }
}

/* 1.5 Hash Function */
static void TaiChi_Hash(const unsigned char *msg,unsigned long long msg_len_bits,unsigned char *digest){
    uint64_t S[RATE_WORDS] = {0};
    uint64_t L[CAPACITY_WORDS] = {0};
    uint64_t R[CAPACITY_WORDS] = {0};
    uint64_t M[RATE_WORDS];

    unsigned long long full_blocks = msg_len_bits / RATE_BITS;
    unsigned long long total_blocks = (msg_len_bits + 2 + RATE_BITS - 1) / RATE_BITS;
    unsigned long long pad_blocks = total_blocks - full_blocks;

    /* Absorb full blocks */
    unsigned long long i;
    for (i = 0; i < full_blocks; i++) {
        memcpy(M, msg + i * RATE_BYTES, RATE_BYTES);
        TaiChi_Step(S, L, R, M, DOMAIN_AB);
    }

    /* Process padding */
    if (pad_blocks > 0) {
        unsigned char last_buf[RATE_BYTES * 2] = {0};
        unsigned long long rem_bits = msg_len_bits % RATE_BITS;
        unsigned long long rem_bytes = rem_bits / 8;
        unsigned int rem_tail = rem_bits % 8;

        if (rem_bytes > 0 || rem_tail > 0) {
            unsigned long long copy_len = rem_bytes + (rem_tail ? 1 : 0);
            memcpy(last_buf, msg + full_blocks * RATE_BYTES, copy_len);
            if (rem_tail > 0) {
                last_buf[rem_bytes] &= (0xff << (8 - rem_tail));
            }
        }
        /* M || 1 */
        unsigned long long bit_pos = rem_bits;
        last_buf[bit_pos / 8] |= (1 << (7 - (bit_pos % 8)));
        bit_pos++;
        /* M || 1 || 0^j */
        while (((bit_pos + 1) % RATE_BITS) != 0) {
            bit_pos++;
        }
        /* M || 1 || 0^j || 1 */
        last_buf[bit_pos / 8] |= (1 << (7 - (bit_pos % 8)));

        for (i = 0; i < pad_blocks; i++) {
            memcpy(M, last_buf + i * RATE_BYTES, RATE_BYTES);
            DOMAIN d = (i == pad_blocks - 1) ? DOMAIN_FIN : DOMAIN_AB;
            TaiChi_Step(S, L, R, M, d);
        }
    }
    // 1024 > 896
    // Z <-- S0
    memcpy(digest,S,112);
    uint64_t tmp[14]={0};
    // Squeeze one time
    // S1 
    TaiChi_Step(S,L,R,tmp,DOMAIN_SQ);
    // Z || S1
    // Select 1024 bits
    memcpy(digest+112,S,16);
}

//Hash API
int CryptHash(int digest_len_bits,const unsigned char *msg,unsigned long long msg_len_bits,unsigned char *digest){

    if(digest_len_bits != TAICHI_INSTANCE){
        return -1;
    }
    TaiChi_Hash(msg,msg_len_bits,digest);
    return 0;
}