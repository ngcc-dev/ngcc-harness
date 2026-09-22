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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "CryptHash_AlgorithmInstance.h"

#define ROUND 16
// rotate table for shiftrow
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

// rotate table for key
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


static const uint64_t ct[24] = {
	0x24, 0x3f, 0x6a, 0x88, 0x85, 0xa3, 0x08, 0xd3, 0x13, 0x19, 0x8a, 0x2e,
    0x03, 0x70, 0x73, 0x44, 0xa4, 0x09, 0x38, 0x22, 0x29, 0x9f, 0x31, 0xd0
};


static inline uint64_t rotl(uint64_t x, int shift) {
    return (x << shift) | (x >> (64 - shift));
}



static void Sbox(uint64_t* state){
    uint64_t out0, out1, out2;
	uint64_t in0 = state[0];
	uint64_t in1 = state[1];
	uint64_t in2 = state[2];

    out0 = (in1 & in2) ^ in0;
    out1 = (out0 & in2) ^ in1;
	out2 = (out0 | out1) ^ in2;
	
    state[0] = out0;
    state[1] = out1;
	state[2] = out2;

}


static void SubBytes(uint64_t state[3][3]){
    for(int i=0; i<3; i++){
        Sbox(state[i]);
    } 
}


static inline void AddM_Key1(uint64_t state[3][3], uint64_t tk1[3][3], uint64_t tk2[3][3]){
    for(int i=0; i<3; i++){
        for(int j=0; j<3; j++){
            state[i][j] = state[i][j] ^ tk1[i][j] ^ tk2[i][j];
        }
    } 
}


static inline void mul_row(uint64_t row[3]){
    uint64_t tmp = row[2];
    row[2] = row[1];
    row[1] = row[0];
    row[0] = tmp;
    row[1] = row[0] ^ row[1];
}

static inline void xor_row(uint64_t row1[3], uint64_t row2[3]){
    for(int i=0; i<3; i++){
        row1[i] ^= row2[i]; 
    }
}

static inline void MixColumns(uint64_t state[3][3]){
    uint64_t tmp[3]={0};
    xor_row(tmp, state[0]); 
    xor_row(tmp, state[1]);         // tmp = B ^ A
    xor_row(tmp, state[2]);         //  tmp = tmp ^ C
    mul_row(tmp);                   //  tmp = MUL(tmp)
    xor_row(state[2], tmp);         //  C ^= tmp
    xor_row(state[1], tmp);         //  B ^= tmp
    xor_row(state[0], tmp);         //  A ^= tmp

}

static void ShiftRows_0(uint64_t state[3][3]){
    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++){
            state[i][j] = rotl(state[i][j], rhoy0[i][j]); 
        }
}

static void ShiftRows_1(uint64_t state[3][3]){
    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++){
            state[i][j] = rotl(state[i][j], rhoy1[i][j]); 
        }
}


static void MessageExpansion1(uint64_t key0[3][3], uint64_t key1[3][3], int round){
    uint64_t tmp[3][3];
    Sbox(key0[0]);
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++) {
            tmp[i][j] = key0[j][(j+2*i)%3];
        }
    }
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            key0[i][j] = rotl(tmp[i][j], rhok[0][i][j]);
        }
    }

    Sbox(key1[0]);
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++) {
            tmp[i][j] = key1[(i+2*j)%3][i];
            
        }
    }
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            key1[i][j] = rotl(tmp[i][j], rhok[1][i][j]);
        }
    }
    
    key1[0][0] ^= ct[round];

}


static void encrypt1(uint64_t state[3][3], uint64_t key0[3][3], uint64_t key1[3][3]){
    AddM_Key1(state, key0, key1); 
    for(int round=0; round<ROUND; round++){ 
        MessageExpansion1(key0, key1, round);    
        SubBytes(state);         
        MixColumns(state);    
        if(round%2==0)
            ShiftRows_0(state);  
        else
            ShiftRows_1(state);
        AddM_Key1(state, key0, key1); 

    }
}



static void MoFang_BC_1_DM(uint64_t state[3][3], uint64_t key[2][3][3]){
    uint64_t s[3][3];
    uint64_t k[2][3][3];
    memcpy(s, state, sizeof(s));
    memcpy(k, key, sizeof(k));
    encrypt1(s,k[0],k[1]);
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
int hash_MDP(int output_len_bits, const unsigned char *input, uint64_t input_len_bits, unsigned char *output){ 
    uint64_t padded_input_len_bits = ((input_len_bits + 1152) / 1152) * 1152;
    uint64_t padded_bytes = padded_input_len_bits / 8;
    unsigned char *padded_input = (unsigned char *)malloc(padded_bytes);
    if (padded_input == NULL) {
        return -1;
    }
    memset(padded_input, 0, padded_bytes);
    uint64_t full_bytes = input_len_bits / 8;
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

    uint64_t state[3][3] = {0};
    uint64_t K[2][3][3] = {0};
    state[2][2] = 5;
    uint64_t blocks = padded_input_len_bits / 1152;
    for (uint64_t t = 0; t < blocks; t++) {
        if(t == blocks - 1){
            state[2][2] ^= 1;
        }
        memset(K, 0, sizeof(K));
        for (int i = 0; i < 3; i ++){
            for (int j = 0; j < 3; j ++){
                for (int k = 7; k >= 0; k--){
                    K[0][i][j] <<=  8;
                    K[0][i][j] |= padded_input[t*144 + (i * 3 + j) * 8 + k];

                    K[1][i][j] <<= 8;
                    K[1][i][j] |= padded_input[t*144 + 72 + (i * 3 + j) * 8 + k];

                }
            }
        }
        MoFang_BC_1_DM(state, K);

    }

    // store state to output  
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