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
#include <emmintrin.h>
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

static const __m128i ct[24] = {
    {0x24, 0x24}, {0x3f, 0x3f}, {0x6a, 0x6a}, {0x88, 0x88}, {0x85, 0x85}, {0xa3, 0xa3},
    {0x08, 0x08}, {0xd3, 0xd3}, {0x13, 0x13}, {0x19, 0x19}, {0x8a, 0x8a}, {0x2e, 0x2e},
    {0x03, 0x03}, {0x70, 0x70}, {0x73, 0x73}, {0x44, 0x44}, {0xa4, 0xa4}, {0x09, 0x09},
    {0x38, 0x38}, {0x22, 0x22}, {0x29, 0x29}, {0x9f, 0x9f}, {0x31, 0x31}, {0xd0, 0xd0}
};


static inline __m128i rotl128(__m128i x, int n) {
    __m128i shiftl = _mm_slli_epi64(x, n);
    __m128i shiftr = _mm_srli_epi64(x, 64 - n);
    __m128i result = _mm_or_si128(shiftl, shiftr);
    return result;
}


static void Sbox(__m128i* state){
    __m128i out0, out1, out2;
	__m128i in0 = state[0];
	__m128i in1 = state[1];
	__m128i in2 = state[2];

    out0 = _mm_xor_si128(_mm_and_si128(in1, in2), in0);
    out1 = _mm_xor_si128(_mm_and_si128(out0, in2), in1);
    out2 = _mm_xor_si128(_mm_or_si128(out0, out1), in2);

    state[0] = out0;
    state[1] = out1;
	state[2] = out2;

}

static void SubBytes(__m128i state[3][3]){
    for(int i=0; i<3; i++){
        Sbox(state[i]);
    } 
}


static inline void AddM_Key2(__m128i state[3][3], __m128i tk1[3][3], __m128i tk2[3][3], __m128i tk3[3][3]){
    for(int i=0; i<3; i++){
        for(int j=0; j<3; j++){
            state[i][j] = _mm_xor_si128(_mm_xor_si128(_mm_xor_si128(tk1[i][j], tk2[i][j]), tk3[i][j]), state[i][j]);
        }
    } 
    
}


static inline void mul_row(__m128i row[3]){
    __m128i tmp = row[2];
    row[2] = row[1];
    row[1] = row[0];
    row[0] = tmp;
    row[1] = _mm_xor_si128(row[0], row[1]);
}


static inline void xor_row(__m128i row1[3], __m128i row2[3]){
    for(int i=0; i<3; i++){
        row1[i] = _mm_xor_si128(row1[i], row2[i]);
    }
}


static inline void MixColumns(__m128i state[3][3]){
    __m128i tmp[3]={0};
    xor_row(tmp, state[0]); 
    xor_row(tmp, state[1]);         // tmp = B ^ A
    xor_row(tmp, state[2]);         //  tmp = tmp ^ C
    mul_row(tmp);                   //  tmp = MUL(tmp)
    xor_row(state[2], tmp);         //  C ^= tmp
    xor_row(state[1], tmp);         //  B ^= tmp
    xor_row(state[0], tmp);         //  A ^= tmp

}


static void ShiftRows_0(__m128i state[3][3]){
    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++){
            state[i][j] = rotl128(state[i][j], rhoy0[i][j]); 
        }
}

static void ShiftRows_1(__m128i state[3][3]){
    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++){
            state[i][j] = rotl128(state[i][j], rhoy1[i][j]);
        }
}


static void MessageExpansion2(__m128i key0[3][3], __m128i key1[3][3], __m128i key2[3][3], int round){
    __m128i tmp[3][3];
    
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++) {
            tmp[i][j] = key0[j][(2*i+j)%3];
        }
    }
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            key0[i][j] = tmp[i][j];
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
            key1[i][j] = rotl128(tmp[i][j], rhok[0][i][j]);
        }
    }

    Sbox(key2[0]);
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++) {
           tmp[i][j] = key2[j][(2*i+j)%3];
        }
    }
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            key2[i][j] = rotl128(tmp[i][j], rhok[1][i][j]);
        }
    }

    key2[0][0] = _mm_xor_si128(ct[round], key2[0][0]);


}

static void encrypt2(__m128i state[3][3], __m128i key0[3][3], __m128i key1[3][3], __m128i key2[3][3]){
    AddM_Key2(state, key0, key1, key2);
    for(int round=0; round<ROUND; round++){         
        MessageExpansion2(key0, key1, key2, round); 
        SubBytes(state);
        MixColumns(state);
        if(round%2==0)
            ShiftRows_0(state);  
        else
            ShiftRows_1(state);
        AddM_Key2(state, key0, key1, key2); 
        
    }
}


void MoFang_BC_2_DBL(ULL in_g[3][3], ULL in_h[3][3], ULL in_m[2][3][3], ULL out_g[3][3], ULL out_h[3][3]){
    __m128i state[3][3], k[3][3][3],out[3][3];
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            state[i][j] = _mm_set1_epi64x(in_g[i][j]);
            k[0][i][j] = _mm_set1_epi64x(in_h[i][j]);
            k[1][i][j] = _mm_set1_epi64x(in_m[0][i][j]);
            k[2][i][j] = _mm_set1_epi64x(in_m[1][i][j]);
        }
    }
    __m128i v_diff1 = _mm_set_epi64x(0, 1);
    state[2][2] = _mm_xor_si128(state[2][2], v_diff1);

    encrypt2(state,k[0],k[1],k[2]);

    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            out[i][j] = _mm_xor_si128(state[i][j], _mm_set1_epi64x(in_g[i][j]));
        }
    }

    out[2][2] = _mm_xor_si128(out[2][2], v_diff1);

    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++){
            //out_h[i][j] = _mm_extract_epi64(out[i][j], 0);
            out_h[i][j] = (uint64_t)_mm_cvtsi128_si64(out[i][j]);
            //out_g[i][j] = _mm_extract_epi64(out[i][j], 1);
            out_g[i][j] = (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(out[i][j], 8));
        }
    }


}



/// @brief The hash function based on DBL+MDPH
/// @param[in] input The base address of message
/// @param[in] input_len_bits The length of message in BITS
/// @param[in] output_len_bits The length of digest in BITS 
/// @param[out] output The base address of digest
int hash_MDPH(int output_len_bits, const unsigned char *input, uint64_t input_len_bits, unsigned char *output){ 
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

    uint64_t in_g[3][3] = {0};
    uint64_t in_h[3][3] = {0};
    uint64_t in_m[2][3][3] = {0};
    uint64_t out_g[3][3] = {0};
    uint64_t out_h[3][3] = {0};
    in_g[2][2] = 11;
    uint64_t blocks = padded_input_len_bits / 1152;
    for (uint64_t t = 0; t < blocks; t++) {
        if(t == blocks - 1){
            in_g[2][2] ^= 2;
        }
        memset(in_m, 0, sizeof(in_m));
        for (int i = 0; i < 3; i ++){
            for (int j = 0; j < 3; j ++){
                for (int k = 7; k >= 0; k--){
                    in_m[0][i][j] <<=  8;
                    in_m[0][i][j] |= padded_input[t*144 + (i * 3 + j) * 8 + k];

                    in_m[1][i][j] <<= 8;
                    in_m[1][i][j] |= padded_input[t*144 + 72 + (i * 3 + j) * 8 + k];

                }
            }
        }
        MoFang_BC_2_DBL(in_g, in_h, in_m, out_g, out_h);
        for (int i = 0; i < 3; i ++){
            for (int j = 0; j < 3; j ++){
                in_g[i][j] = out_g[i][j];
                in_h[i][j] = out_h[i][j];
            }
        }

    }

    // store state back to output 
    for (int i = 0; i < 8; i ++){
        for (int k = 0; k < 8; k++){
            output[i * 8 + k] = (out_g[i/3][i%3] >> (8 * k)) & 0xFF;
        }
    }
    for (int i = 0; i < 8; i ++){
        for (int k = 0; k < 8; k++){
            output[64 + i * 8 + k] = (out_h[i/3][i%3] >> (8 * k)) & 0xFF;
        }
    }
    free(padded_input);
    return 0;

}


int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    return hash_MDPH(digest_len_bits, msg, msg_len_bits,  digest);
}