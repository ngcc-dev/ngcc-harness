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
#include <emmintrin.h>
#include "CryptHash_AlgorithmInstance.h"

#define ULL unsigned long long

static const int rho[4][4] = {
	{0,14,20,22},
	{0,13,68,91},
	{0,27,42,106},
    {0,32,48,80}
};

static const unsigned char RCON[24] = {
    0x24, 0x3f, 0x6a, 0x88, 0x85, 0xa3, 0x08, 0xd3,
    0x13, 0x19, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x44,
    0xa4, 0x09, 0x38, 0x22, 0x29, 0x9f, 0x31, 0xd0
};

// n < 64
static inline __m128i rotr128(__m128i x, int n) {
    __m128i shifted = _mm_srli_epi64(x, n);
    __m128i shift_left = _mm_slli_epi64(x, 64 - n);
    __m128i swapped = _mm_shuffle_epi32(shift_left, _MM_SHUFFLE(1, 0, 3, 2));
    __m128i result = _mm_or_si128(shifted, swapped);
    return result;
}

// optimized rotation for 128-bit vector
static inline __m128i rot128bit(__m128i a, int count) {
    if (count > 64){
        count = count - 64;
        a = _mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 3, 2));
    }
    return rotr128(a, count);
}


static inline void Sbox(__m128i *in0, __m128i *in1, __m128i *in2, __m128i *in3){
    __m128i out0, out1, out2, out3;

    out3 = _mm_xor_si128(_mm_and_si128(*in3, *in2), *in1);
	out1 = _mm_xor_si128(_mm_or_si128(*in2, *in1), *in0);
	out0 = _mm_xor_si128(_mm_and_si128(out3, *in0), *in3);
	out2 = _mm_xor_si128(_mm_and_si128(out1, *in3), *in2);

    *in0 = out0;
    *in1 = out1;
	*in2 = out2;
	*in3 = out3;
}

static inline void SubBytes(__m128i state[4][4]){
    Sbox(&state[0][0], &state[0][1], &state[0][2], &state[0][3]);
    Sbox(&state[1][0], &state[1][1], &state[1][2], &state[1][3]);
    Sbox(&state[2][0], &state[2][1], &state[2][2], &state[2][3]);
    Sbox(&state[3][0], &state[3][1], &state[3][2], &state[3][3]);
}

static inline void mul_row(__m128i row[4]){
    __m128i tmp = row[3];
    row[3] = row[2];
    row[2] = row[1];
    row[1] = _mm_xor_si128(tmp, row[0]);
    row[0] = tmp;
}

static inline void xor_row(__m128i row1[4], __m128i row2[4]){
    row1[0] = _mm_xor_si128(row1[0], row2[0]); 
    row1[1] = _mm_xor_si128(row1[1], row2[1]); 
    row1[2] = _mm_xor_si128(row1[2], row2[2]); 
    row1[3] = _mm_xor_si128(row1[3], row2[3]); 
}

static inline void MixColumns(__m128i state[4][4]){
    xor_row(state[2], state[3]); /* C ^= D */
	xor_row(state[0], state[1]); /* A ^= B */
	mul_row(state[1]);           /* B = MUL(B) */
	mul_row(state[3]);           /* D = MUL(D) */
	xor_row(state[1], state[2]); /* B ^= C */
    xor_row(state[3], state[0]); /* D ^= A */
	mul_row(state[0]);           /* A = MUL(A) */
	mul_row(state[0]);           /* A = MUL(A) */
	mul_row(state[2]);           /* C = MUL(C) */
	mul_row(state[2]);           /* C = MUL(C) */
	xor_row(state[2], state[3]); /* C ^= D */
	xor_row(state[0], state[1]); /* A ^= B */
	xor_row(state[1], state[2]); /* B ^= C */
	xor_row(state[3], state[0]); /* D ^= A */
}

static inline void ShiftRows(__m128i state[4][4], int subround){
    for(int i=1; i<4; i++)
        for(int j=0; j<4; j++){
            state[i][j] = rot128bit(state[i][j], rho[subround][i]);
        }
}

static inline void permutation(unsigned char *state, int roundNum){
    __m128i S[4][4];
    // load state into S
    for (int i = 0; i < 4; i ++){
        for (int j = 0; j < 4; j ++){
            S[j][i] = _mm_loadu_si128((__m128i *)(state + (i * 4 + j) * 16));
        }
    }
    for(int r=0; r<roundNum; r++){
        
        SubBytes(S);
        MixColumns(S);
        ShiftRows(S, r%4);
        // add constants
        __m128i rc = _mm_cvtsi64_si128((long long)RCON[r]);
        S[3][3] = _mm_xor_si128(S[3][3], rc);
    }
    // store state back to output
    for (int i = 0; i < 4; i ++){
        for (int j = 0; j < 4; j ++){
            _mm_storeu_si128((__m128i *)(state + (i * 4 + j) * 16), S[j][i]);
        }
    }
} 

int hash_sponge(int hash, int rate, const unsigned char *input, ULL input_len_bits, int output_len_bits, unsigned char *output){
    if (rate != 160 || hash != 48){
        printf("Invalid rate or hash length\n");
        return 1;
    }
    unsigned char state[256];
    unsigned char IV[256];
    memset(IV, 0, sizeof(IV));
    memcpy(state, IV, sizeof(IV));
    // padding
    ULL padded_input_len_bits = ((input_len_bits + rate*8) / (rate*8)) * rate*8;
    ULL padded_input_len = padded_input_len_bits / 8;
    unsigned char *padded_input = (unsigned char *)malloc(padded_input_len);
    memset(padded_input, 0, padded_input_len);
    ULL full_bytes = input_len_bits / 8;
    int remaining_bits = input_len_bits % 8;
    memcpy(padded_input, input, full_bytes);
    if (remaining_bits > 0) {
        unsigned char mask = 0xFF << (8 - remaining_bits);
        padded_input[full_bytes] = input[full_bytes] & mask;
        padded_input[full_bytes] |= 0x80 >> remaining_bits; // padding with 0x80
    } else {
        padded_input[full_bytes] = 0x80; // padding with 0x80
    }

    // absorbing phase
    ULL blocks = padded_input_len / rate;
    for (ULL i = 0; i < blocks; i++) {
        for (int j = 0; j < rate; j++) {
            state[j] ^= padded_input[i * rate + j];
        }
        permutation(state, 20);
    }
    // squeezing phase
    for (int i = 0; i < hash; i ++){
        output[i] = state[i];
    }
    return 0;
}

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    return hash_sponge(digest_len_bits / 8, 160, msg, msg_len_bits, digest_len_bits, digest);
}