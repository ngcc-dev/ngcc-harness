/*
Copyright (c) 2026 Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polar encoding and decoding helpers for the optimized POLARLAC-Light instance.
*/


# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#include <stdint.h>
#include <immintrin.h> // AVX
#include "params.h"
#include "polar.h"


#define sign_macro(x) ((x > 0) - (x < 0))
#define absl_macro(x) (((x > 0) - (x < 0)) * x)
#define mini_macro(x, y) ((x < y) ? x : y)
#define f_macro(L1, L2) sign_macro(L1) * sign_macro(L2) * mini_macro(absl_macro(L1), absl_macro(L2))
#define g_macro(u, L1, L2) (((1 - 2*u) * L1) + L2)


/*
 * The large constant arrays used by the polar encoder/decoder are defined
 * in a single compilation unit (polar.c).  Here we only declare them as
 * extern to avoid multiple-definition linker errors when this header is
 * included in multiple .c files.
 */

// message bit is 1, frozen bit is 0
uint8_t info_nodes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// store 2^0~2^n
// the values in lambda_offset divide the intermediate result storage arrays P and C into segments
static const int lambda_offset[9] = {1, 2, 4, 8, 16, 32, 64, 128, 256};

// store the number of least significant zero bits in the binary representation of integers from 0 to N-1
// llr_layer_vec indicates the actual number of layers executed during LLR computation
static const int llr_layer_vec[256] = {0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};

// store the number of least significant one bits minus 1 in the binary representation of integers from 0 to N-1
// bit_layer_vec indicates the actual number of layers executed when returning the bit values
static const int bit_layer_vec[256] = {0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 6, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 7};

#define NONZERO 141 // NONZERO/3 is the number of subcodes

// subcode composition:
// the first column indicates the starting index of the subcode,
// the second column indicates the subcode length,
// and the third column indicates the subcode type 
// (-1 for R0 code, 1 for R1 code, 2 for Rep code, and 3 for Type-I code)
static const int node_type_matrix[][3] = {{0, 32, 2}, {32, 16, 2}, {48, 8, 2}, {56, 4, 2}, {60, 2, 2}, {62, 2, 1}, {64, 16, 2}, {80, 8, 2}, {88, 4, 2}, {92, 4, 1}, {96, 8, 3}, {104, 2, 2}, {106, 2, 1}, {108, 4, 1}, {112, 2, 2}, {114, 2, 1}, {116, 4, 1}, {120, 8, 1}, {128, 16, 2}, {144, 8, 2}, {152, 4, 3}, {156, 4, 1}, {160, 4, -1}, {164, 2, 2}, {166, 2, 1}, {168, 2, 2}, {170, 2, 1}, {172, 4, 1}, {176, 2, 2}, {178, 2, 1}, {180, 4, 1}, {184, 8, 1}, {192, 4, 2}, {196, 2, 2}, {198, 2, 1}, {200, 2, 2}, {202, 2, 1}, {204, 4, 1}, {208, 2, 2}, {210, 2, 1}, {212, 4, 1}, {216, 8, 1}, {224, 2, 2}, {226, 2, 1}, {228, 4, 1}, {232, 8, 1}, {240, 16, 1}};

// right-shift the binary representation of each subcode's starting index by logM bits (M is the code length of the subcode)
// psi_vec is used during the computation of intermediate bit values
static const int psi_vec[NONZERO / 3] = {0, 2, 6, 14, 30, 31, 4, 10, 22, 23, 12, 52, 53, 27, 56, 57, 29, 15, 8, 18, 38, 39, 40, 82, 83, 84, 85, 43, 88, 89, 45, 23, 48, 98, 99, 100, 101, 51, 104, 105, 53, 27, 112, 113, 57, 29, 15};

// information bit indices
static const int data_pos_sorted[128] = {31, 47, 55, 59, 61, 62, 63, 79, 87, 91, 92, 93, 94, 95, 102, 103, 105, 106, 107, 108, 109, 110, 111, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 143, 151, 154, 155, 156, 157, 158, 159, 165, 166, 167, 169, 170, 171, 172, 173, 174, 175, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 195, 197, 198, 199, 201, 202, 203, 204, 205, 206, 207, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

struct polar_control polar = {
.N = 256,
.n = 8,
.K = 128,
.ecc_bytes = CODE_LEN
}; 



/**
 * polar encode
 * Algorithm idea inspired by:
 * https://github.com/sravan-ankireddy/polar_codes (Repository does not specify a license)
 */
void encode_polar_avx(uint8_t *u_8)
{
    // level = 7;
    __m128i u1 = _mm_load_si128((__m128i *)u_8); // u0~u15
    __m128i u2 = _mm_load_si128((__m128i *)(u_8 + 16)); // u16~u31
    u1 = _mm_xor_si128(u1, u2);

    // level = 6;
    __m128i left = _mm_unpacklo_epi64(u1, u2);
    __m128i right = _mm_unpackhi_epi64(u1, u2);
    u1 = _mm_xor_si128(left, right);
    u2 = right;

    // level = 5;
    // shift+blend
    __m128i u2_sl = _mm_slli_si128(u2, 4);
    left = _mm_blend_epi32(u1, u2_sl, 0xA); // 0xA:0b1010
    __m128i u1_sr = _mm_srli_si128(u1, 4);
    right = _mm_blend_epi32(u1_sr, u2, 0xA); // 0xA:0b1010
    u1 = _mm_xor_si128(left, right);
    u2 = right;

    // level = 4;
    // shift+blend
    u2_sl = _mm_slli_si128(u2, 2);
    left = _mm_blend_epi16(u1, u2_sl, 0xAA); // 0xAA:0b10101010
    u1_sr = _mm_srli_si128(u1, 2);
    right = _mm_blend_epi16(u1_sr, u2, 0xAA); // 0xAA:0b10101010
    u1 = _mm_xor_si128(left, right);
    u2 = right;

    // level = 3;
    // 用shuffle+unpack
    __m128i mask = _mm_set_epi8(15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0);
    __m128i u1_lr = _mm_shuffle_epi8(u1, mask);
    __m128i u2_lr = _mm_shuffle_epi8(u2, mask);
    left = _mm_unpacklo_epi64(u1_lr, u2_lr);
    right = _mm_unpackhi_epi64(u1_lr, u2_lr);
    u1 = _mm_xor_si128(left, right);
    u2 = right;

    // level = 2;
    // AND+shift4+OR
    __m128i l_mask_4 = _mm_set1_epi8(0x0F);
    __m128i u1_left = _mm_and_si128(u1, l_mask_4);
    __m128i u2_left = _mm_and_si128(u2, l_mask_4);
    __m128i u2_left_sl = _mm_slli_epi64(u2_left, 4);
    left = _mm_or_si128(u1_left, u2_left_sl);
    __m128i r_mask_4 = _mm_set1_epi8((unsigned char)0xF0);
    __m128i u1_right = _mm_and_si128(u1, r_mask_4);
    __m128i u2_right = _mm_and_si128(u2, r_mask_4);
    __m128i u1_right_sr = _mm_srli_epi64(u1_right, 4);
    right = _mm_or_si128(u1_right_sr, u2_right);
    u1 = _mm_xor_si128(left, right);
    u2 = right;

    // level = 1;
    // AND+shift2+OR
    __m128i l_mask_2 = _mm_set1_epi8(0x33);
    u1_left = _mm_and_si128(u1, l_mask_2);
    u2_left = _mm_and_si128(u2, l_mask_2);
    u2_left_sl = _mm_slli_epi64(u2_left, 2);
    left = _mm_or_si128(u1_left, u2_left_sl);
    __m128i r_mask_2 = _mm_set1_epi8((unsigned char)0xCC);
    u1_right = _mm_and_si128(u1, r_mask_2);
    u2_right = _mm_and_si128(u2, r_mask_2);
    u1_right_sr = _mm_srli_epi64(u1_right, 2);
    right = _mm_or_si128(u1_right_sr, u2_right);
    u1 = _mm_xor_si128(left, right);
    u2 = right;

    // level = 0;
    // AND+shift1+OR
    __m128i l_mask_1 = _mm_set1_epi8(0x55);
    u1_left = _mm_and_si128(u1, l_mask_1);
    u2_left = _mm_and_si128(u2, l_mask_1);
    u2_left_sl = _mm_slli_epi64(u2_left, 1);
    left = _mm_or_si128(u1_left, u2_left_sl);
    __m128i r_mask_1 = _mm_set1_epi8((unsigned char)0xAA);
    u1_right = _mm_and_si128(u1, r_mask_1);
    u2_right = _mm_and_si128(u2, r_mask_1);
    u1_right_sr = _mm_srli_epi64(u1_right, 1);
    right = _mm_or_si128(u1_right_sr, u2_right);
    u1 = _mm_xor_si128(left, right);
    u2 = right;

    mask = _mm_set_epi8(15, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 0);
    left = _mm_shuffle_epi8(u1, mask);
    right = _mm_shuffle_epi8(u2, mask);
    __m128i left_even = _mm_and_si128(left, l_mask_1); // 0x55:0b01010101
    __m128i left_odd = _mm_and_si128(left, r_mask_1); // 0xAA:0b10101010
    __m128i right_even = _mm_and_si128(right, l_mask_1); // 0x55:0b01010101
    __m128i right_odd = _mm_and_si128(right, r_mask_1); // 0xAA:0b10101010
    left = _mm_or_si128(left_even, _mm_slli_epi64(right_even, 1));
    right = _mm_or_si128(_mm_srli_epi64(left_odd, 1), right_odd);

    left_even = _mm_and_si128(left, l_mask_2); // 0x33:0b00110011
    left_odd = _mm_and_si128(left, r_mask_2); // 0xCC:0b11001100
    right_even = _mm_and_si128(right, l_mask_2); // 0x33:0b00110011
    right_odd = _mm_and_si128(right, r_mask_2); // 0xCC:0b11001100
    left = _mm_or_si128(left_even, _mm_slli_epi64(right_even, 2));
    right = _mm_or_si128(_mm_srli_epi64(left_odd, 2), right_odd);

    left_even = _mm_and_si128(left, l_mask_4); // 0x0F:0b00001111
    left_odd = _mm_and_si128(left, r_mask_4); // 0xF0:0b11110000
    right_even = _mm_and_si128(right, l_mask_4); // 0x0F:0b00001111
    right_odd = _mm_and_si128(right, r_mask_4); // 0xF0:0b11110000
    left = _mm_or_si128(left_even, _mm_slli_epi64(right_even, 4));
    right = _mm_or_si128(_mm_srli_epi64(left_odd, 4), right_odd);
    u1 = _mm_unpacklo_epi8(left, right);
    u2 = _mm_unpackhi_epi8(left, right);

    _mm_store_si128((__m128i *)u_8, u1);
    _mm_store_si128((__m128i *)(u_8 + 16), u2);
}
void encode_polar_opt(uint64_t *u_64)
{
    int base;
    uint64_t left, right, new_left, new_right;
    uint64_t left_even, left_odd, right_even, right_odd;

    // level = 7;
    for (int offset = 0; offset < 2; offset++)
    {
        u_64[offset] = u_64[offset] ^ u_64[offset + 2];
    }

    // level = 6;
    for (int block = 0; block < 2; block++)
    {
        base = 2 * block;
        u_64[base] = u_64[base] ^ u_64[base + 1];
    }

    // level = 5;
    for (int block = 0; block < 2; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x00000000FFFFFFFFULL) | (right << 32);
        new_right = (right & 0xFFFFFFFF00000000ULL) | (left >> 32);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 4;
    for (int block = 0; block < 2; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x0000FFFF0000FFFFULL) | ((right & 0x0000FFFF0000FFFFULL) << 16);
        new_right = (right & 0xFFFF0000FFFF0000ULL) | ((left & 0xFFFF0000FFFF0000ULL) >> 16);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 3;
    for (int block = 0; block < 2; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x00FF00FF00FF00FFULL) | ((right & 0x00FF00FF00FF00FFULL) << 8);
        new_right = (right & 0xFF00FF00FF00FF00ULL) | ((left & 0xFF00FF00FF00FF00ULL) >> 8);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 2;
    for (int block = 0; block < 2; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x0F0F0F0F0F0F0F0FULL) | ((right & 0x0F0F0F0F0F0F0F0FULL) << 4);
        new_right = (right & 0xF0F0F0F0F0F0F0F0ULL) | ((left & 0xF0F0F0F0F0F0F0F0ULL) >> 4);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 1;
    for (int block = 0; block < 2; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x3333333333333333ULL) | ((right & 0x3333333333333333ULL) << 2);
        new_right = (right & 0xCCCCCCCCCCCCCCCCULL) | ((left & 0xCCCCCCCCCCCCCCCCULL) >> 2);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 0;
    for (int block = 0; block < 2; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x5555555555555555ULL) | ((right & 0x5555555555555555ULL) << 1);
        new_right = (right & 0xAAAAAAAAAAAAAAAAULL) | ((left & 0xAAAAAAAAAAAAAAAAULL) >> 1);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
        
        left_even = u_64[base] & 0x5555555555555555ULL;
        left_odd = u_64[base] & 0xAAAAAAAAAAAAAAAAULL;
        right_even = u_64[base + 1] & 0x5555555555555555ULL;
        right_odd = u_64[base + 1] & 0xAAAAAAAAAAAAAAAAULL;
        u_64[base] = left_even | (right_even << 1);
        u_64[base + 1] = (left_odd >> 1) | right_odd;

        left_even = u_64[base] & 0x3333333333333333ULL;
        left_odd = u_64[base] & 0xCCCCCCCCCCCCCCCCULL;
        right_even = u_64[base + 1] & 0x3333333333333333ULL;
        right_odd = u_64[base + 1] & 0xCCCCCCCCCCCCCCCCULL;
        u_64[base] = left_even | (right_even << 2);
        u_64[base + 1] = (left_odd >> 2) | right_odd;

        left_even = u_64[base] & 0x0F0F0F0F0F0F0F0FULL;
        left_odd = u_64[base] & 0xF0F0F0F0F0F0F0F0ULL;
        right_even = u_64[base + 1] & 0x0F0F0F0F0F0F0F0FULL;
        right_odd = u_64[base + 1] & 0xF0F0F0F0F0F0F0F0ULL;
        u_64[base] = left_even | (right_even << 4);
        u_64[base + 1] = (left_odd >> 4) | right_odd;

        left_even = u_64[base] & 0x00FF00FF00FF00FFULL;
        left_odd = u_64[base] & 0xFF00FF00FF00FF00ULL;
        right_even = u_64[base + 1] & 0x00FF00FF00FF00FFULL;
        right_odd = u_64[base + 1] & 0xFF00FF00FF00FF00ULL;
        u_64[base] = left_even | (right_even << 8);
        u_64[base + 1] = (left_odd >> 8) | right_odd;

        left_even = u_64[base] & 0x0000FFFF0000FFFFULL;
        left_odd = u_64[base] & 0xFFFF0000FFFF0000ULL;
        right_even = u_64[base + 1] & 0x0000FFFF0000FFFFULL;
        right_odd = u_64[base + 1] & 0xFFFF0000FFFF0000ULL;
        u_64[base] = left_even | (right_even << 16);
        u_64[base + 1] = (left_odd >> 16) | right_odd;

        left_even = u_64[base] & 0x00000000FFFFFFFFULL;
        left_odd = u_64[base] & 0xFFFFFFFF00000000ULL;
        right_even = u_64[base + 1] & 0x00000000FFFFFFFFULL;
        right_odd = u_64[base + 1] & 0xFFFFFFFF00000000ULL;
        u_64[base] = left_even | (right_even << 32);
        u_64[base + 1] = (left_odd >> 32) | right_odd;
    }
}
void encode_polar_ref(uint8_t *u, int n)
{ 
    int stage_size = 1; // current butterfly size

    for (int level = 0; level < n; level++)
    {
        int group_size = stage_size * 2;
        int num_groups = (1 << n) / group_size;

        for (int block = 0; block < num_groups; block++)
        {
            uint8_t *segment = u + block * group_size;

            /* Apply butterfly transform */
            for (int offset = 0; offset < stage_size; offset++)
            {
                uint8_t left  = segment[offset];
                uint8_t right = segment[offset + stage_size];

                segment[offset] = left ^ right;
            }
        }

        stage_size <<= 1; // move to next level
    }
}
void encode_polar_N2(uint8_t *u)
{
    u[0] = u[0] ^ u[1];
}
void encode_polar_N4(uint8_t *u)
{
    u[0] = u[0] ^ u[1] ^ u[2] ^ u[3];
    u[1] = u[1] ^ u[3];
    u[2] = u[2] ^ u[3];
}
void encode_polar_N8(uint8_t *u)
{
    u[0] = u[0] ^ u[1] ^ u[2] ^ u[3] ^ u[4] ^ u[5] ^ u[6] ^ u[7];
    u[1] = u[1] ^ u[3] ^ u[5] ^ u[7];
    u[2] = u[2] ^ u[3] ^ u[6] ^ u[7];
    u[3] = u[3] ^ u[7];
    u[4] = u[4] ^ u[5] ^ u[6] ^ u[7];
    u[5] = u[5] ^ u[7];
    u[6] = u[6] ^ u[7];
}
void encode_polar_N16(uint8_t *u)
{
    u[0] = u[0] ^ u[1] ^ u[2] ^ u[3] ^ u[4] ^ u[5] ^ u[6] ^ u[7] ^ u[8] ^ u[9] ^ u[10] ^ u[11] ^ u[12] ^ u[13] ^ u[14] ^ u[15];
    u[1] = u[1] ^ u[3] ^ u[5] ^ u[7] ^ u[9] ^ u[11] ^ u[13] ^ u[15];
    u[2] = u[2] ^ u[3] ^ u[6] ^ u[7] ^ u[10] ^ u[11] ^ u[14] ^ u[15];
    u[3] = u[3] ^ u[7] ^ u[11] ^ u[15];
    u[4] = u[4] ^ u[5] ^ u[6] ^ u[7] ^ u[12] ^ u[13] ^ u[14] ^ u[15];
    u[5] = u[5] ^ u[7] ^ u[13] ^ u[15];
    u[6] = u[6] ^ u[7] ^ u[14] ^ u[15];
    u[7] = u[7] ^ u[15];
    u[8] = u[8] ^ u[9] ^ u[10] ^ u[11] ^ u[12] ^ u[13] ^ u[14] ^ u[15];
    u[9] = u[9] ^ u[11] ^ u[13] ^ u[15];
    u[10] = u[10] ^ u[11] ^ u[14] ^ u[15];
    u[11] = u[11] ^ u[15];
    u[12] = u[12] ^ u[13] ^ u[14] ^ u[15];
    u[13] = u[13] ^ u[15];
    u[14] = u[14] ^ u[15];
}

/**
 * polar decode
 * Algorithm idea inspired by:
 * https://github.com/YuYongRun/PolarCodeDecodersInMatlab (Repository does not specify a license)
 */
__m256i f_avx256(__m256i L1, __m256i L2)
{
    __m256i zero = _mm256_setzero_si256();

    /* sign masks: all-ones if negative, otherwise all-zeros */
    __m256i mask1 = _mm256_cmpgt_epi64(zero, L1);
    __m256i mask2 = _mm256_cmpgt_epi64(zero, L2);

    /* abs(L1), abs(L2) */
    __m256i abs1 = _mm256_sub_epi64(_mm256_xor_si256(L1, mask1), mask1);
    __m256i abs2 = _mm256_sub_epi64(_mm256_xor_si256(L2, mask2), mask2);

    /* min(abs(L1), abs(L2)) */
    __m256i gt = _mm256_cmpgt_epi64(abs1, abs2);
    __m256i min_abs = _mm256_blendv_epi8(abs1, abs2, gt);

    /* result sign is negative if L1 and L2 have opposite signs */
    __m256i sign_mask = _mm256_xor_si256(mask1, mask2);

    /* apply sign to min_abs */
    return _mm256_sub_epi64(_mm256_xor_si256(min_abs, sign_mask), sign_mask);
}

__m256i g_avx256(__m256i u, __m256i L1, __m256i L2)
{
    __m256i one = _mm256_set1_epi64x(1);

    // u = 1: mask = 0xffffffffffffffff, u = 0: mask = 0x0000000000000000
    __m256i mask = _mm256_cmpeq_epi64(u, one);

    // u = 0: signed_L1 = L1, u = 1: signed_L1 = -L1
    __m256i signed_L1 = _mm256_sub_epi64(_mm256_xor_si256(L1, mask), mask);

    return _mm256_add_epi64(signed_L1, L2);
}

// use the SC decoding algorithm for R1 codes
void R1_SC_arr(int64_t *llr, int N, int n, uint8_t *msg_cap) // llr refers to channel LLR
{
    uint8_t C[2 * N - 1][2]; // internal bit vector
    __attribute__((aligned(32))) int64_t P[N - 1]; // internal LLR vector, P[0] is used for decision
    int msg_index = 0;

    for (int i = 0; i < N; i++) // decode each source bit
    {
        if (i == 0) // for decoding 1st bit
        {
            int index_1 = lambda_offset[n - 1];
            int beta = 0;
            int end_beta = index_1 - 1;
            
            for (; beta <= end_beta - 3; beta += 4)
            {
                __m256i L1 = _mm256_loadu_si256((__m256i *)(&llr[beta]));
                __m256i L2 = _mm256_loadu_si256((__m256i *)(&llr[beta + index_1]));
                __m256i result = f_avx256(L1, L2);
                _mm256_storeu_si256((__m256i *)(&P[beta + index_1 - 1]), result);
            }
            for (; beta <= end_beta; beta++)
            {
                P[beta + index_1 - 1] = f_macro(llr[beta], llr[beta + index_1]);
            }

            for (int i_layer = n - 2; i_layer >= 0; i_layer--)
            {
                int index_1 = lambda_offset[i_layer];
                int index_2 = lambda_offset[i_layer + 1];
                int beta = index_1 - 1;
                int end_beta = index_2 - 2;
                
                for (; beta <= end_beta - 3; beta += 4)
                {
                    __m256i L1 = _mm256_loadu_si256((__m256i *)(&P[beta + index_1]));
                    __m256i L2 = _mm256_loadu_si256((__m256i *)(&P[beta + index_2]));
                    __m256i result = f_avx256(L1, L2);
                    _mm256_storeu_si256((__m256i *)(&P[beta]), result);
                }
                for (; beta <= end_beta; beta++)
                {
                    P[beta] = f_macro(P[beta + index_1], P[beta + index_2]);
                }
            }
        }
        else if (i == N / 2) // for decoding the middle bit
        {
            int index_1 = lambda_offset[n - 1];
            int beta = 0;
            int end_beta = index_1 - 1;
            
            for (; beta <= end_beta - 3; beta += 4)
            {
                __m256i L1 = _mm256_loadu_si256((__m256i *)(&llr[beta]));
                __m256i L2 = _mm256_loadu_si256((__m256i *)(&llr[beta + index_1]));
                __m256i u = _mm256_set_epi64x(C[beta + index_1 + 2][0], C[beta + index_1 + 1][0], C[beta + index_1][0], C[beta + index_1 - 1][0]);
                __m256i result = g_avx256(u, L1, L2);
                _mm256_storeu_si256((__m256i *)(&P[beta + index_1 - 1]), result);
            }
            for (; beta <= end_beta; beta++)
            {
                P[beta + index_1 - 1] = g_macro(C[beta + index_1 - 1][0], llr[beta], llr[beta + index_1]);
            }

            for (int i_layer = n - 2; i_layer >= 0; i_layer--)
            {
                int index_1 = lambda_offset[i_layer];
                int index_2 = lambda_offset[i_layer + 1];
                int beta = index_1 - 1;
                int end_beta = index_2 - 2;
                
                for (; beta <= end_beta - 3; beta += 4)
                {
                    __m256i L1 = _mm256_loadu_si256((__m256i *)(&P[beta + index_1]));
                    __m256i L2 = _mm256_loadu_si256((__m256i *)(&P[beta + index_2]));
                    __m256i result = f_avx256(L1, L2);
                    _mm256_storeu_si256((__m256i *)(&P[beta]), result);
                }
                for (; beta <= end_beta; beta++)
                {
                    P[beta] = f_macro(P[beta + index_1], P[beta + index_2]);
                }
            }
        }
        else
        {
            int llr_layer = llr_layer_vec[i];
            int index_1 = lambda_offset[llr_layer];
            int index_2 = lambda_offset[llr_layer + 1];
            int beta = index_1 - 1;
            int end_beta = index_2 - 2;
            
            for (; beta <= end_beta - 3; beta += 4)
            {
                __m256i L1 = _mm256_loadu_si256((__m256i *)(&P[beta + index_1]));
                __m256i L2 = _mm256_loadu_si256((__m256i *)(&P[beta + index_2]));
                __m256i u = _mm256_set_epi64x(C[beta + 3][0], C[beta + 2][0], C[beta + 1][0], C[beta][0]);
                __m256i result = g_avx256(u, L1, L2);
                _mm256_storeu_si256((__m256i *)(&P[beta]), result);
            }
            for (; beta <= end_beta; beta++)
            {
                P[beta] = g_macro(C[beta][0], P[beta + index_1], P[beta + index_2]);
            }

            for (int i_layer = llr_layer - 1; i_layer >= 0; i_layer--)
            {
                int index_1 = lambda_offset[i_layer];
                int index_2 = lambda_offset[i_layer + 1];
                int beta = index_1 - 1;
                int end_beta = index_2 - 2;
                
                for (; beta <= end_beta - 3; beta += 4)
                {
                    __m256i L1 = _mm256_loadu_si256((__m256i *)(&P[beta + index_1])); // 即使定义P时内存对齐了,但P[beta + index_1]和P[beta + index_2]不一定内存对齐,所以这里用loadu
                    __m256i L2 = _mm256_loadu_si256((__m256i *)(&P[beta + index_2]));
                    __m256i result = f_avx256(L1, L2);
                    _mm256_storeu_si256((__m256i *)(&P[beta]), result);
                }
                for (; beta <= end_beta; beta++)
                {
                    P[beta] = f_macro(P[beta + index_1], P[beta + index_2]);
                }
            }
        }

        int i_mod_2 = i & 1;
        int u_i = (P[0] < 0); // decision
        C[0][i_mod_2] = u_i; // store internal bit values
        msg_cap[msg_index] = u_i; // store decoding results
        msg_index++;
        if (i_mod_2 == 1) // bit recursion
        {
            int bit_layer = bit_layer_vec[i];
            int index_1;
            int index_2;
            for (int i_layer = 0; i_layer <= bit_layer - 1; i_layer++)
            {
                index_1 = lambda_offset[i_layer];
                index_2 = lambda_offset[i_layer + 1];
                for (int beta = index_1 - 1; beta <= index_2 - 2; beta++)
                {
                    C[beta + index_1][1] = C[beta][0] ^ C[beta][1];
                    C[beta + index_2][1] = C[beta][1];
                }
            }

            index_1 = lambda_offset[bit_layer];
            index_2 = lambda_offset[bit_layer + 1];
            for (int beta = index_1 - 1; beta <= index_2 - 2; beta++)
            {
                C[beta + index_1][0] = C[beta][0] ^ C[beta][1];
                C[beta + index_2][0] = C[beta][1];
            }
        }
    }
}

void decode_polar_avx(uint8_t *m_cap, const int64_t *llr)
{
    int l = NONZERO / 3;
    uint8_t C[2 * polar.N - 1][2]; // bit vector
    __attribute__((aligned(32))) int64_t P[2 * polar.N - 1]; // LLR vector(includes channel LLR)
    // LLR initialization
    memcpy(P + (polar.N - 1), llr, polar.N * sizeof(int64_t));

    for (int i_node = 0; i_node < l; i_node++)
    {
        int M = node_type_matrix[i_node][1]; // length of subcode
        // reduced_layer denotes where to stop  LLR calculation
        // reduced_layer also denotes where to start Internal Bits calculation
        int reduced_layer = 0;
        int M_temp = M;
        /* log function */
        while (M_temp >>= 1)
            reduced_layer++;
        // llr_layer denotes where to start LLR calculation
        int llr_layer = llr_layer_vec[node_type_matrix[i_node][0]];
        // bit_layer denotes where to stop Internal Bits calculation
        int bit_layer = bit_layer_vec[node_type_matrix[i_node][0] + M - 1];
        // psi is used for bits recursion
        int psi = psi_vec[i_node];
        int psi_mod_2 = psi & 1;

        // the first subcode
        if (i_node == 0) // first LLR calculation only uses f function
        {
            for (int i_layer = polar.n - 1; i_layer >= reduced_layer; i_layer--)
            {
                int index_1 = lambda_offset[i_layer];
                int index_2 = lambda_offset[i_layer + 1];
                int beta = index_1 - 1;
                int end_beta = index_2 - 2;
                
                for (; beta <= end_beta - 3; beta += 4)
                {
                    __m256i L1 = _mm256_loadu_si256((__m256i *)(&P[beta + index_1])); // 即使定义P时内存对齐了,但P[beta + index_1]和P[beta + index_2]不一定内存对齐,所以这里用loadu
                    __m256i L2 = _mm256_loadu_si256((__m256i *)(&P[beta + index_2]));
                    __m256i result = f_avx256(L1, L2);
                    _mm256_storeu_si256((__m256i *)(&P[beta]), result);
                }
                for (; beta <= end_beta; beta++)
                {
                    P[beta] = f_macro(P[beta + index_1], P[beta + index_2]);
                }
            }
        }
        else // non-first subcode
        {
            int index_1 = lambda_offset[llr_layer];
            int index_2 = lambda_offset[llr_layer + 1];
            
            int beta = index_1 - 1;
            int end_beta = index_2 - 2;
            for (; beta <= end_beta - 3; beta += 4)
            {
                __m256i L1 = _mm256_loadu_si256((__m256i *)(&P[beta + index_1]));
                __m256i L2 = _mm256_loadu_si256((__m256i *)(&P[beta + index_2]));
                __m256i u = _mm256_set_epi64x(C[beta + 3][0], C[beta + 2][0], C[beta + 1][0], C[beta][0]);
                __m256i result = g_avx256(u, L1, L2);
                _mm256_storeu_si256((__m256i *)(&P[beta]), result);
            }
            for (; beta <= end_beta; beta++)
            {
                P[beta] = g_macro(C[beta][0], P[beta + index_1], P[beta + index_2]);
            }

            for (int i_layer = llr_layer - 1; i_layer >= reduced_layer; i_layer--)
            {
                index_1 = lambda_offset[i_layer];
                index_2 = lambda_offset[i_layer + 1];
                beta = index_1 - 1;
                end_beta = index_2 - 2;
                
                for (; beta <= end_beta - 3; beta += 4)
                {
                    __m256i L1 = _mm256_loadu_si256((__m256i *)(&P[beta + index_1]));
                    __m256i L2 = _mm256_loadu_si256((__m256i *)(&P[beta + index_2]));
                    __m256i result = f_avx256(L1, L2);
                    _mm256_storeu_si256((__m256i *)(&P[beta]), result);
                }
                for (; beta <= end_beta; beta++)
                {
                    P[beta] = f_macro(P[beta + index_1], P[beta + index_2]);
                }
            }
        }

        switch (node_type_matrix[i_node][2])
        {
        case -1: // RATE 0
        {
            for (int j = M - 1; j <= 2 * M - 2; j++)
            {
                C[j][psi_mod_2] = 0;
            }
            break;
        }
        case 1: // RATE 1 is decoded using SC decoding
        {
            if (M == 1)
            {
                C[0][psi_mod_2] = (P[0] < 0);
            }
            else
            {
                uint8_t sub_u[M]; // source sequence of the R1 subcode
                R1_SC_arr(P + M - 1, M, reduced_layer, sub_u);

                // compute the codeword sequence of the R1 subcode
                // encode(sub_u, M, reduced_layer);
                // perform targeted optimization for encoding with different code lengths
                switch (M)
                {
                case 2:
                    encode_polar_N2(sub_u);
                    break;
                case 4:
                    encode_polar_N4(sub_u);
                    break;
                case 8:
                    encode_polar_N8(sub_u);
                    break;
                case 16:
                    encode_polar_N16(sub_u);
                    break;
                default:
                    encode_polar_ref(sub_u, reduced_layer);
                    break;
                }

                for (int j = M - 1; j <= 2 * M - 2; j++)
                {
                    C[j][psi_mod_2] = sub_u[j - M + 1];
                }
            }
            break;
        }
        case 2: // REP
        {
            int64_t sum_llr = 0;
            for (int j = M - 1; j <= 2 * M - 2; j++)
            {
                sum_llr += P[j];
            }
            uint8_t rep_bit = (sum_llr < 0); // decision
            for (int j = M - 1; j <= 2 * M - 2; j++)
            {
                C[j][psi_mod_2] = rep_bit;
            }
            break;
        }
        case 3: // Type-I
        {
            int64_t sum_even_llr = 0; // sum of the values at even indices in the LLR vector
            int64_t sum_odd_llr = 0; // sum of the values at odd indices in the LLR vector
            for (int j = M - 1; j <= 2 * M - 2; j++)
            {
                if(j % 2 == 0)
                {
                    sum_even_llr += P[j];
                }
                else
                {
                    sum_odd_llr += P[j];
                }
            }
            uint8_t even_bit = (sum_even_llr < 0); // make decisions on the bit values at even positions
            uint8_t odd_bit = (sum_odd_llr < 0); // make decisions on the bit values at odd positions
            for (int j = M - 1; j <= 2 * M - 2; j++)
            {
                if(j % 2 == 0)
                {
                    C[j][psi_mod_2] = even_bit;
                }
                else
                {
                    C[j][psi_mod_2] = odd_bit;
                } 
            }
            break;
        }
        }

        // bit recursion
        if (psi_mod_2 == 1)
        {
            int index_1;
            int index_2;
            for (int i_layer = reduced_layer; i_layer <= bit_layer - 1; i_layer++)
            {
                index_1 = lambda_offset[i_layer];
                index_2 = lambda_offset[i_layer + 1];
                for (int beta = index_1 - 1; beta <= index_2 - 2; beta++)
                {
                    C[beta + index_1][1] = C[beta][0] ^ C[beta][1];
                    C[beta + index_2][1] = C[beta][1];
                }
            }
            index_1 = lambda_offset[bit_layer];
            index_2 = lambda_offset[bit_layer + 1];
            for (int beta = index_1 - 1; beta <= index_2 - 2; beta++)
            {
                C[beta + index_1][0] = C[beta][0] ^ C[beta][1];
                C[beta + index_2][0] = C[beta][1];
            }
        }
    }

    // SSC decoding estimates the codeword sequence
    __attribute__((aligned(16))) uint8_t x_cap[polar.N/8];
    memset(x_cap, 0, (polar.N/8)*sizeof(uint8_t));
    for(int i = 0; i < polar.N/8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            x_cap[i] |= (C[8*i+j+polar.N-1][0] << j);
        }
    }
    encode_polar_avx(x_cap); // u=xG,compute the corresponding source sequence
    memset(m_cap, 0, (polar.K/8)*sizeof(uint8_t)); // estimated message(each element stores 8-bit)
    for(int i = 0; i < polar.K/8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            int pos = data_pos_sorted[8*i+j];
            m_cap[i] |= (((x_cap[pos/8] >> (pos%8)) & 0x01) << j);
        }
    }
}
