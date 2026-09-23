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

#define TAICHI_INSTANCE 512

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

typedef enum{
    DOMAIN_AB  = 1,
    DOMAIN_FIN = 2,
    DOMAIN_SQ  = 3
}DOMAIN;

static inline uint64_t ROTL64(uint64_t x,unsigned int n){
    return (x << n) | (x >> ((64-n)&63));
}

// 1.6 P1920 permutation
static uint64_t RC[120];

static void precompute_RC() {

    static const int p[7] = {0,2,3,6,13,28,59};
    uint8_t s[7] = {1,0,0,0,0,0,0};

    for (int q = 0;q < 120;q ++) {
        uint64_t C = 0;
        for (int i = 0;i < 7;i ++) {
            if (s[i]) {
                C ^= (1ULL << p[i]);
            }
        }
        RC[q] = C;

        uint8_t next_s[7];
        next_s[0] = s[6];
        next_s[1] = s[0] ^ s[6];
        next_s[2] = s[1];
        next_s[3] = s[2];
        next_s[4] = s[3];
        next_s[5] = s[4];
        next_s[6] = s[5];

        for (int i = 0; i < 7; i++) {
            s[i] = next_s[i];
        }
    }
}
static void P1920(uint64_t A[30],int b){
    static const uint8_t n1[16] = {0,1,2,4,3,5,6,7,13,14,15,16,15,18,11,20};
    static const uint8_t n2[16] = {1,2,4,3,5,6,0,11,8,9,10,12,17,14,19,13};
    static const uint8_t m[16] = {8,0,0,0,0,0,23,62,35,14,48,1,57,63,58,22};

    uint64_t tmp[30];
    uint64_t *cur = A;
    uint64_t *next = tmp;
    int t;
    precompute_RC();
    for(t = 0;t < 12;t ++){
        // 1. MRM layer
        int j;
        for(j = 0;j < 5;j ++){
            uint64_t y[22];
            int i;
            for(i = 0;i < 6;i ++){
                y[i] = cur[5*i+j];
            }
            for(i = 6;i < 22;i ++){
                y[i] = y[n1[i-6]] ^ ROTL64(y[n2[i-6]],m[i-6]);
            }
            cur[5*0+j] = ROTL64(y[16],0);
            cur[5*1+j] = ROTL64(y[17],15);
            cur[5*2+j] = ROTL64(y[18],48);
            cur[5*3+j] = ROTL64(y[19],63);
            cur[5*4+j] = ROTL64(y[20],6);
            cur[5*5+j] = ROTL64(y[21],33);
        }

        // 2. S-box layer
        int i;
        for(i = 0;i < 6;i ++){
            uint64_t a0 = cur[5*i+0];
            uint64_t a1 = cur[5*i+1];
            uint64_t a2 = cur[5*i+2];
            uint64_t a3 = cur[5*i+3];
            uint64_t a4 = cur[5*i+4];
            cur[5*i+0] = a1 ^ (a0&a1) ^ a2 ^ (a1&a2) ^ a3 ^ (a3&a4);
            cur[5*i+1] = ~0ULL ^ a1 ^ (a0&a3) ^ (a1&a3) ^ a4 ^ (a2&a4);
            cur[5*i+2] = (a1&a2)^a3^(a2&a3)^a4^(a0&a4);
            cur[5*i+3] = (a0&a2)^(a1&a3)^(a2&a3)^a4;
            cur[5*i+4] = a0^(a2&a3)^(a1&a4);
        }

        // 3. K2 lane permutation
        for(i = 0;i < 6;i ++){
            for(j = 0;j < 5;j ++){
                int n = 5*i + j;
                int np = (10*(n+1) % 31) - 1;
                int ip = np / 5;
                int jp = np % 5;
                next[5*ip+jp] = cur[n];
            }
        }

        // 4. Add round constants
        for (int j = 0; j < 5; j++){
            int q = (b - 1) * 60 + 5 * t + j;
            next[j] ^= RC[q]; 
        }

        uint64_t *swap;
        swap = cur;
        cur = next;
        next = swap;
    }
    if(cur != A){
        memcpy(A,tmp,30*sizeof(uint64_t));
    }
}

// 1.6 P1
static void TaiChi_P1(uint64_t state[30]){
    P1920(state,1);
}

// 1.6 P2
static void TaiChi_P2(uint64_t state[30]){
    P1920(state,2);
}

// 1.3 Padding
static unsigned long long TaiChi_Padding(unsigned char *out,const unsigned char *msg,unsigned long long msg_len_bits){
    unsigned long long bytes;
    bytes = msg_len_bits / 8;
    // Copy message
    memcpy(out,msg,bytes);
    unsigned long long remain;
    remain = msg_len_bits % 8;
    if(remain){
        out[bytes] = msg[bytes] & (0xff<<(8-remain));
    }

    // M || 1
    unsigned long long total_bits;
    total_bits = msg_len_bits;
    out[total_bits/8] |= (1 << (7-total_bits%8));
    total_bits ++;

    // M || 1 || 0^j
    while((total_bits + 1) % RATE_BITS != 0){
        total_bits ++;
    }

    // M || 1 || 0^j || 1
    out[total_bits/8] |= (1 << (7-total_bits%8));
    total_bits ++;

    return (total_bits + 7) / 8;
}

// 1.4 TaiChi Step Function
static void TaiChi_Step(uint64_t S[RATE_WORDS],uint64_t L[CAPACITY_WORDS],uint64_t R[CAPACITY_WORDS],uint64_t M[RATE_WORDS],DOMAIN D){
    uint64_t state[30]={0};
    uint64_t A[RATE_WORDS]={0};
    uint64_t B[CAPACITY_WORDS]={0};
    int i;

    // Xi = S xor M
    for(i = 0;i < RATE_WORDS;i ++){
        state[i] = S[i] ^ M[i];
    }

    // L xor enc(D)
    for(i = 0;i < CAPACITY_WORDS;i ++){
        state[RATE_WORDS + i] = L[i];
    }
    if(D == DOMAIN_AB){
        state[29] ^= 0x01;
    }
    else if(D == DOMAIN_FIN){
        state[29] ^= 0x02;
    }
    else{
        state[29] ^= 0x03;
    }
    TaiChi_P1(state);
    memcpy(A,state,RATE_WORDS*sizeof(uint64_t));
    memcpy(B,state+RATE_WORDS,CAPACITY_WORDS*sizeof(uint64_t));

    // P2
    for(i = 0;i < RATE_WORDS;i ++){
        state[i] = A[i];
    }
    for(i = 0;i < CAPACITY_WORDS;i ++){
        state[RATE_WORDS + i] = R[i] ^ B[i];
    }
    TaiChi_P2(state);

    // update
    for (i = 0; i < RATE_WORDS; i++) {
        S[i] = state[i];
    }
    for(i = 0;i < CAPACITY_WORDS;i ++){
        L[i] = state[RATE_WORDS + i];
        R[i] = B[i] ^ state[RATE_WORDS + i];
    }
}

// 1.5 Hash Function
static void TaiChi_Hash(const unsigned char *msg,unsigned long long msg_len_bits,unsigned char *digest){
    uint64_t S[RATE_WORDS]={0};
    uint64_t L[CAPACITY_WORDS]={0};
    uint64_t R[CAPACITY_WORDS]={0};

    unsigned long long padded_len;
    padded_len = (msg_len_bits + 2 + RATE_BITS - 1) / RATE_BITS * RATE_BYTES;
    unsigned char *buf;
    buf = (unsigned char*)malloc(padded_len);
    if(buf == NULL){
        return;
    }
    memset(buf,0,padded_len);

    unsigned long long plen;
    plen = TaiChi_Padding(buf,msg,msg_len_bits);

    uint64_t M[RATE_WORDS]={0};
    unsigned long long blocks = plen/RATE_BYTES;
    unsigned long long i;
    for(i = 0;i < blocks;i ++){
        memcpy(M,buf+i*RATE_BYTES,RATE_BYTES);
        if(i == blocks-1){
            // Finalization
            TaiChi_Step(S,L,R,M,DOMAIN_FIN);
        }
        else{
            // Absorb
            TaiChi_Step(S,L,R,M,DOMAIN_AB);
        }
    }

    // 512 < 1408
    memcpy(digest,S,DIGEST_BYTES);
    free(buf);
}

// Hash API
int CryptHash(int digest_len_bits,const unsigned char *msg,unsigned long long msg_len_bits,unsigned char *digest){
    if(digest_len_bits != TAICHI_INSTANCE){
        return -1;
    }
    TaiChi_Hash(msg,msg_len_bits,digest);
    return 0;
}