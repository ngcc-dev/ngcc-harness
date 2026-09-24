/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "CryptHash_AlgorithmInstance.h"

#define ULL uint64_t

#define ROUND 16

/// @brief The rotation offsets for ShiftRow
static const int rhoy0[3][3] = {
    {0,3,6},
    {9,12,15},
    {18,21,24}
};

static const int rhoy1[3][3] = {
    {0,24,48},
    {8,32,56},
    {16,40,7}
};


/// @brief The rotation offsets for Key transposition
static const int rhok[2][3][3] = {
    {
        {1,5,11},
        {17,0,31},
        {41,47,59}
    },
    {
        {3,7,19},
        {43,53,61},
        {0,29,37},
    }
};


/// @brief The round constants for AddRoundConstant
static const ULL ct[24] = {
	0x24, 0x3f, 0x6a, 0x88, 0x85, 0xa3, 0x08, 0xd3, 0x13, 0x19, 0x8a, 0x2e,
    0x03, 0x70, 0x73, 0x44, 0xa4, 0x09, 0x38, 0x22, 0x29, 0x9f, 0x31, 0xd0
};


static inline ULL rotl(ULL x, int shift) {
    return (x << shift) | (x >> (64 - shift));
}


/// @brief The multiplication function for bit-sliced operations
static inline void MUL(ULL *x0, ULL *x1, ULL *x2){
    ULL tmp = *x2;; 
    *x2 = *x1; *x1 = *x0; *x0 = tmp; 
    *x1 ^= *x0; 
}

/// @brief The bit-sliced MDS matrix multiplication
static inline void MixColumns(ULL state[3][3]){
    ULL tmp[3]; 
    tmp[0] = state[0][0] ^ state[1][0] ^ state[2][0];       // tmp = C ^ B ^ A
    tmp[1] = state[0][1] ^ state[1][1] ^ state[2][1];
    tmp[2] = state[0][2] ^ state[1][2] ^ state[2][2];      
    MUL(&tmp[0], &tmp[1], &tmp[2]);                         //  tmp = MUL(tmp)
    state[2][0] ^= tmp[0]; state[2][1] ^= tmp[1]; state[2][2] ^= tmp[2]; //  C ^= tmp
    state[1][0] ^= tmp[0]; state[1][1] ^= tmp[1]; state[1][2] ^= tmp[2]; //  B ^= tmp
    state[0][0] ^= tmp[0]; state[0][1] ^= tmp[1]; state[0][2] ^= tmp[2]; //  A ^= tmp

}


/// @brief The based block cipher of MoFang with keylen 2n
/// @param[in] state The input internal state/chaining value
/// @param[in] Key The Key/message state
/// @param[in] roundNum The number of rounds to execute
void MoFang_BC_1(ULL state[3][3], ULL Key[2][3][3],  int roundNum){
    ULL A[3][3], B[3][3], C[3][3], X[3][3];
    ULL K[2][3][3] = {0};
    ULL TmpS[3];

    memcpy(K, Key, sizeof(K));
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            A[i][j] = state[i][j] ^ Key[0][i][j] ^ Key[1][i][j];
        }
    }

    for (int r = 0; r < roundNum; r++) {
        //keyexpansion
        //S box
        TmpS[0] = (K[0][0][1] & K[0][0][2]) ^ K[0][0][0];
        TmpS[1] = (TmpS[0] & K[0][0][2]) ^ K[0][0][1];
        TmpS[2] = (TmpS[0] | TmpS[1]) ^ K[0][0][2];

        K[0][0][0] = TmpS[0];
        K[0][0][1] = TmpS[1];
        K[0][0][2] = TmpS[2];

        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 3; j++) {
                X[i][j] = K[0][j][(j+2*i)%3];
            }
        }

        for(int i=0; i<3; i++) {
            for(int j=0; j<3; j++){
                K[0][i][j] = rotl(X[i][j], rhok[0][i][j]);
            }
        }

        TmpS[0] = (K[1][0][1] & K[1][0][2]) ^ K[1][0][0];
        TmpS[1] = (TmpS[0] & K[1][0][2]) ^ K[1][0][1];
        TmpS[2] = (TmpS[0] | TmpS[1]) ^ K[1][0][2];

        K[1][0][0] = TmpS[0];
        K[1][0][1] = TmpS[1];
        K[1][0][2] = TmpS[2];

        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 3; j++) {
                X[i][j] = K[1][(i+2*j)%3][i];
            }
        }

        
        for(int i=0; i<3; i++) {
            for(int j=0; j<3; j++){
                K[1][i][j] = rotl(X[i][j], rhok[1][i][j]);
            }
        }
        K[1][0][0] ^= ct[r];
       

        // Sbox
         for (int i = 0; i < 3; i++){ 
            B[i][0] = (A[i][1] & A[i][2]) ^ A[i][0];
            B[i][1] = (B[i][0] & A[i][2]) ^ A[i][1];
            B[i][2] = (B[i][0] | B[i][1]) ^ A[i][2];

        }
        
        // MixColumns
        MixColumns(B);
        
        // ShiftRows
        if(r%2==0) {
            for(int i=0; i<3; i++){
                for(int j=0; j<3; j++){
                    C[i][j] = rotl(B[i][j], rhoy0[i][j]);
                }
            }
        } else {
            for(int i=0; i<3; i++){
                for(int j=0; j<3; j++){
                    C[i][j] = rotl(B[i][j], rhoy1[i][j]);
                }
            }
        }
        

        for(int i=0; i<3; i++) {
            for(int j=0; j<3; j++){
                A[i][j] = C[i][j] ^ K[0][i][j] ^ K[1][i][j];
            }
        }

    }

    memcpy(state, A, sizeof(A));

}

void MoFang_BC_1_DM(ULL state[3][3], ULL Key[2][3][3],  int roundNum){
    ULL s[3][3] = {0};
    ULL k[2][3][3] = {0};
    memcpy(s, state, sizeof(s));
    memcpy(k, Key, sizeof(k));
    MoFang_BC_1(s, k, roundNum);
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            state[i][j] ^= s[i][j];
        }
    }
    
}


/// @brief The hash function based on DM+MDP
/// @param[in] input The base address of message
/// @param[in] input_len_bits The length of message in BITS
/// @param[in] output_len_bits The length of digest in BITS (should be equal to 256)
/// @param[out] output The base address of digest
int hash_MDP(int output_len_bits, const unsigned char *input, ULL input_len_bits, unsigned char *output){ 
   uint64_t padded_input_len_bits = ((input_len_bits + 1152) / 1152) * 1152;
    uint64_t padded_bytes = padded_input_len_bits / 8;
    unsigned char *padded_input = (unsigned char *)malloc(padded_bytes);
    if (padded_input == NULL) {
        return -1;
    }
    memset(padded_input, 0, padded_bytes);
    ULL full_bytes = input_len_bits / 8;
    int remaining_bits = input_len_bits % 8;
    memcpy(padded_input, input, full_bytes);
    if (remaining_bits > 0) {
        unsigned char mask = 0xFF << (8 - remaining_bits);
        padded_input[full_bytes] = input[full_bytes] & mask;
        padded_input[full_bytes] |= (0x80 >> remaining_bits); 
    } else {
         if (full_bytes < padded_bytes) { 
            padded_input[full_bytes] = 0x80;
        }
    }

    ULL state[3][3] = {0};   
    ULL K[2][3][3] = {0};
    state[2][2] = 5;
    ULL blocks = padded_input_len_bits / 1152;
    for (ULL t = 0; t < blocks; t++) {
        if(t == blocks -1){
            state[2][2] ^= 1;
        }
        memset(K, 0, sizeof(K));
        for (int i = 0; i < 3; i ++){
            for (int j = 0; j < 3; j ++){
                for (int k = 7; k >= 0; k --){
                    K[0][i][j] <<= 8;
                    K[0][i][j] |= padded_input[t*144 + (i * 3 + j) * 8 + k];

                    K[1][i][j] <<= 8;
                    K[1][i][j] |= padded_input[t*144 + 72 + (i * 3 + j) * 8 + k];

                }
            }
        }
        MoFang_BC_1_DM(state, K,  ROUND);

    }

    // store state back to output  
    for (int i = 0; i < 4; i ++){
        for (int k = 0; k < 8; k++){
            output[i * 8 + k] = (state[i/3][i%3] >> (8 * k)) & 0xFF;
        }
    }
    free(padded_input);
    return 0;
   
   
}


int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    return hash_MDP(digest_len_bits, msg, msg_len_bits,  digest);
}