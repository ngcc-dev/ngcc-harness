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

/* 64-bit rotate left */
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

/* ---------- 1.6 P1920 permutation ---------- */
static uint64_t Generate_RC(int q){

    static const int p[7] = {0,2,3,6,13,28,59};
    uint8_t s[7];
    int i;
    // reset LFSR
    s[0] = 1;
    for(i = 1;i < 7;i ++){
        s[i] = 0;
    }
    // advance q times
    for(i = 0;i < q;i ++){
        uint8_t next[7];
        next[0] = s[6];
        next[1] = s[0] ^ s[6];
        next[2] = s[1];
        next[3] = s[2];
        next[4] = s[3];
        next[5] = s[4];
        next[6] = s[5];
        memcpy(s,next,7);
    }

    uint64_t C = 0;
    for(i = 0;i < 7;i ++){
        if(s[i]){
            C ^= (1ULL << p[i]);
        }
    }
    return C;
}
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

        /* 3. K2 lane permutation  */
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
            A[j] ^= Generate_RC(q);
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
static void TaiChi_Step(uint64_t state[30], uint64_t R[CAPACITY_WORDS],const uint64_t M[RATE_WORDS], DOMAIN D) {

    uint64_t B[CAPACITY_WORDS] = {0};
    int i;

    for(i = 0; i < RATE_WORDS; i++) {
        state[i] ^= M[i];
    }
    state[29] ^= (uint64_t)D;
    TaiChi_P1(state);

    memcpy(B, state + RATE_WORDS, CAPACITY_WORDS * sizeof(uint64_t));
    for(i = 0; i < CAPACITY_WORDS; i++) {
        state[RATE_WORDS + i] = R[i] ^ B[i];
    }
    TaiChi_P2(state);

    for(i = 0; i < CAPACITY_WORDS; i++) {
        R[i] = B[i] ^ state[RATE_WORDS + i];
    }
}

/* ---------- 1.5 Hash ---------- */
static void TaiChi_Hash(const unsigned char *msg,unsigned long long msg_len_bits,unsigned char *digest){
    uint64_t state[30] = {0};
    uint64_t R[CAPACITY_WORDS] = {0};
    uint64_t M[RATE_WORDS] = {0};

    unsigned long long full_blocks = msg_len_bits / RATE_BITS;
    unsigned long long rem_bits = msg_len_bits % RATE_BITS;

    for (unsigned long long i = 0; i < full_blocks; i++) {
        memcpy(M, msg + i * RATE_BYTES, RATE_BYTES);
        TaiChi_Step(state, R, M, DOMAIN_AB);
    }

    const unsigned char *tail = msg + full_blocks * RATE_BYTES;
    unsigned long long rem_bytes = rem_bits / 8;
    unsigned int rem_tail = (unsigned int)(rem_bits % 8);
    unsigned char *dst = (unsigned char *)M;

    memset(M, 0, RATE_BYTES);
    if (rem_bytes > 0) {
        memcpy(dst, tail, rem_bytes);
    }
    if (rem_tail > 0) {
        dst[rem_bytes] = tail[rem_bytes] & (0xff << (8 - rem_tail));
    }

    dst[rem_bits / 8] |= (unsigned char)(1U << (7 - (rem_bits % 8)));
    if (rem_bits == RATE_BITS - 1) {
        TaiChi_Step(state, R, M, DOMAIN_AB);
        memset(M, 0, RATE_BYTES);
        dst[RATE_BYTES - 1] = 0x01;
        TaiChi_Step(state, R, M, DOMAIN_FIN);
    }
    else {
        dst[RATE_BYTES - 1] |= 0x01;
        TaiChi_Step(state, R, M, DOMAIN_FIN);
    }
    // 1024 > 896
    memcpy(digest, state, DIGEST_BYTES);
    memcpy(digest,state,112);
    memset(M, 0, RATE_BYTES);

    TaiChi_Step(state,R,M,DOMAIN_SQ);

    memcpy(digest+112,state,16);
}

//Hash API
int CryptHash(int digest_len_bits,const unsigned char *msg,unsigned long long msg_len_bits,unsigned char *digest){

    if(digest_len_bits != TAICHI_INSTANCE){
        return -1;
    }
    TaiChi_Hash(msg,msg_len_bits,digest);
    return 0;
}