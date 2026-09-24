/*
Copyright (c) 2026 Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polar encoding and decoding helpers for the optimized POLARLAC-512 instance.
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
uint8_t info_nodes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// store 2^0~2^n
// the values in lambda_offset divide the intermediate result storage arrays P and C into segments
static const int lambda_offset[11] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};

// store the number of least significant zero bits in the binary representation of integers from 0 to N-1
// llr_layer_vec indicates the actual number of layers executed during LLR computation
static const int llr_layer_vec[1024] = {0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 9, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};

// store the number of least significant one bits minus 1 in the binary representation of integers from 0 to N-1
// bit_layer_vec indicates the actual number of layers executed when returning the bit values
static const int bit_layer_vec[1024] = {0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 6, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 7, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 6, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 8, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 6, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 7, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 6, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 5, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 9};

#define NONZERO 549 // NONZERO/3 is the number of subcodes

// subcode composition:
// the first column indicates the starting index of the subcode,
// the second column indicates the subcode length,
// and the third column indicates the subcode type 
// (-1 for R0 code, 1 for R1 code, 2 for Rep code, and 3 for Type-I code)
static const int node_type_matrix[][3] = {{0, 64, 2}, {64, 32, 2}, {96, 16, 2}, {112, 8, 2}, {120, 4, 2}, {124, 2, 2}, {126, 2, 1}, {128, 32, 2}, {160, 16, 2}, {176, 8, 2}, {184, 4, 2}, {188, 2, 2}, {190, 2, 1}, {192, 16, 2}, {208, 8, 2}, {216, 4, 2}, {220, 2, 2}, {222, 2, 1}, {224, 8, 2}, {232, 4, 2}, {236, 2, 2}, {238, 2, 1}, {240, 4, 2}, {244, 2, 2}, {246, 2, 1}, {248, 2, 2}, {250, 2, 1}, {252, 4, 1}, {256, 32, 2}, {288, 16, 2}, {304, 8, 2}, {312, 4, 2}, {316, 2, 2}, {318, 2, 1}, {320, 16, 2}, {336, 8, 2}, {344, 4, 2}, {348, 2, 2}, {350, 2, 1}, {352, 8, 2}, {360, 4, 2}, {364, 4, 1}, {368, 2, 2}, {370, 2, 1}, {372, 4, 1}, {376, 8, 1}, {384, 16, 2}, {400, 8, 2}, {408, 4, 3}, {412, 4, 1}, {416, 4, -1}, {420, 2, 2}, {422, 2, 1}, {424, 2, 2}, {426, 2, 1}, {428, 4, 1}, {432, 2, 2}, {434, 2, 1}, {436, 4, 1}, {440, 8, 1}, {448, 4, 2}, {452, 2, 2}, {454, 2, 1}, {456, 2, 2}, {458, 2, 1}, {460, 4, 1}, {464, 2, 2}, {466, 2, 1}, {468, 4, 1}, {472, 8, 1}, {480, 2, 2}, {482, 2, 1}, {484, 4, 1}, {488, 8, 1}, {496, 16, 1}, {512, 32, 2}, {544, 16, 2}, {560, 8, 2}, {568, 4, 2}, {572, 2, 2}, {574, 2, 1}, {576, 16, 2}, {592, 8, 2}, {600, 4, 2}, {604, 4, 1}, {608, 4, -1}, {612, 2, 2}, {614, 2, 1}, {616, 2, 2}, {618, 2, 1}, {620, 4, 1}, {624, 2, 2}, {626, 2, 1}, {628, 4, 1}, {632, 8, 1}, {640, 16, 2}, {656, 4, -1}, {660, 2, 2}, {662, 2, 1}, {664, 2, 2}, {666, 2, 1}, {668, 4, 1}, {672, 4, 2}, {676, 2, 2}, {678, 2, 1}, {680, 2, 2}, {682, 2, 1}, {684, 4, 1}, {688, 2, 2}, {690, 2, 1}, {692, 4, 1}, {696, 8, 1}, {704, 4, 2}, {708, 2, 2}, {710, 2, 1}, {712, 2, 2}, {714, 2, 1}, {716, 4, 1}, {720, 2, 2}, {722, 2, 1}, {724, 4, 1}, {728, 8, 1}, {736, 2, 2}, {738, 2, 1}, {740, 4, 1}, {744, 8, 1}, {752, 16, 1}, {768, 8, -1}, {776, 4, 2}, {780, 2, 2}, {782, 2, 1}, {784, 4, 2}, {788, 2, 2}, {790, 2, 1}, {792, 2, 2}, {794, 2, 1}, {796, 4, 1}, {800, 4, 2}, {804, 2, 2}, {806, 2, 1}, {808, 2, 2}, {810, 2, 1}, {812, 4, 1}, {816, 2, 2}, {818, 2, 1}, {820, 4, 1}, {824, 8, 1}, {832, 4, 2}, {836, 2, 2}, {838, 2, 1}, {840, 2, 2}, {842, 2, 1}, {844, 4, 1}, {848, 2, 2}, {850, 2, 1}, {852, 4, 1}, {856, 8, 1}, {864, 2, 2}, {866, 2, 1}, {868, 4, 1}, {872, 8, 1}, {880, 16, 1}, {896, 4, 2}, {900, 2, 2}, {902, 2, 1}, {904, 2, 2}, {906, 2, 1}, {908, 4, 1}, {912, 2, 2}, {914, 2, 1}, {916, 4, 1}, {920, 8, 1}, {928, 2, 2}, {930, 2, 1}, {932, 4, 1}, {936, 8, 1}, {944, 16, 1}, {960, 2, 2}, {962, 2, 1}, {964, 4, 1}, {968, 8, 1}, {976, 16, 1}, {992, 32, 1}};

// right-shift the binary representation of each subcode's starting index by logM bits (M is the code length of the subcode)
// psi_vec is used during the computation of intermediate bit values
static const int psi_vec[NONZERO / 3] = {0, 2, 6, 14, 30, 62, 63, 4, 10, 22, 46, 94, 95, 12, 26, 54, 110, 111, 28, 58, 118, 119, 60, 122, 123, 124, 125, 63, 8, 18, 38, 78, 158, 159, 20, 42, 86, 174, 175, 44, 90, 91, 184, 185, 93, 47, 24, 50, 102, 103, 104, 210, 211, 212, 213, 107, 216, 217, 109, 55, 112, 226, 227, 228, 229, 115, 232, 233, 117, 59, 240, 241, 121, 61, 31, 16, 34, 70, 142, 286, 287, 36, 74, 150, 151, 152, 306, 307, 308, 309, 155, 312, 313, 157, 79, 40, 164, 330, 331, 332, 333, 167, 168, 338, 339, 340, 341, 171, 344, 345, 173, 87, 176, 354, 355, 356, 357, 179, 360, 361, 181, 91, 368, 369, 185, 93, 47, 96, 194, 390, 391, 196, 394, 395, 396, 397, 199, 200, 402, 403, 404, 405, 203, 408, 409, 205, 103, 208, 418, 419, 420, 421, 211, 424, 425, 213, 107, 432, 433, 217, 109, 55, 224, 450, 451, 452, 453, 227, 456, 457, 229, 115, 464, 465, 233, 117, 59, 480, 481, 241, 121, 61, 31};

// information bit indices
static const int data_pos_sorted[512] = {63, 95, 111, 119, 123, 125, 126, 127, 159, 175, 183, 187, 189, 190, 191, 207, 215, 219, 221, 222, 223, 231, 235, 237, 238, 239, 243, 245, 246, 247, 249, 250, 251, 252, 253, 254, 255, 287, 303, 311, 315, 317, 318, 319, 335, 343, 347, 349, 350, 351, 359, 363, 364, 365, 366, 367, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 399, 407, 410, 411, 412, 413, 414, 415, 421, 422, 423, 425, 426, 427, 428, 429, 430, 431, 433, 434, 435, 436, 437, 438, 439, 440, 441, 442, 443, 444, 445, 446, 447, 451, 453, 454, 455, 457, 458, 459, 460, 461, 462, 463, 465, 466, 467, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 478, 479, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495, 496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 543, 559, 567, 571, 573, 574, 575, 591, 599, 603, 604, 605, 606, 607, 613, 614, 615, 617, 618, 619, 620, 621, 622, 623, 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 655, 661, 662, 663, 665, 666, 667, 668, 669, 670, 671, 675, 677, 678, 679, 681, 682, 683, 684, 685, 686, 687, 689, 690, 691, 692, 693, 694, 695, 696, 697, 698, 699, 700, 701, 702, 703, 707, 709, 710, 711, 713, 714, 715, 716, 717, 718, 719, 721, 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734, 735, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753, 754, 755, 756, 757, 758, 759, 760, 761, 762, 763, 764, 765, 766, 767, 779, 781, 782, 783, 787, 789, 790, 791, 793, 794, 795, 796, 797, 798, 799, 803, 805, 806, 807, 809, 810, 811, 812, 813, 814, 815, 817, 818, 819, 820, 821, 822, 823, 824, 825, 826, 827, 828, 829, 830, 831, 835, 837, 838, 839, 841, 842, 843, 844, 845, 846, 847, 849, 850, 851, 852, 853, 854, 855, 856, 857, 858, 859, 860, 861, 862, 863, 865, 866, 867, 868, 869, 870, 871, 872, 873, 874, 875, 876, 877, 878, 879, 880, 881, 882, 883, 884, 885, 886, 887, 888, 889, 890, 891, 892, 893, 894, 895, 899, 901, 902, 903, 905, 906, 907, 908, 909, 910, 911, 913, 914, 915, 916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 926, 927, 929, 930, 931, 932, 933, 934, 935, 936, 937, 938, 939, 940, 941, 942, 943, 944, 945, 946, 947, 948, 949, 950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 961, 962, 963, 964, 965, 966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981, 982, 983, 984, 985, 986, 987, 988, 989, 990, 991, 992, 993, 994, 995, 996, 997, 998, 999, 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023};

struct polar_control polar = {
.N = 1024, 
.n = 10,
.K = 512,
.ecc_bytes = CODE_LEN
}; 




/**
 * polar encode
 * Algorithm idea inspired by:
 * https://github.com/sravan-ankireddy/polar_codes (Repository does not specify a license)
 */
void encode_polar_avx(uint8_t *u_8)
{
    // level = 9;
    __m256i u11 = _mm256_load_si256((__m256i *)u_8); // u0~u31
    __m256i u12 = _mm256_load_si256((__m256i *)(u_8 + 32)); // u32~u63
    __m256i u21 = _mm256_load_si256((__m256i *)(u_8 + 64)); // u64~u95
    __m256i u22 = _mm256_load_si256((__m256i *)(u_8 + 96)); // u96~u127
    u11 = _mm256_xor_si256(u11, u21);
    u12 = _mm256_xor_si256(u12, u22);
    
    // level = 8;
    u11 = _mm256_xor_si256(u11, u12);
    u21 = _mm256_xor_si256(u21, u22);

    // level = 7;
    __m256i left1 = _mm256_permute2x128_si256(u11, u12, 0x20); // 0x20:0b00100000
    __m256i right1 = _mm256_permute2x128_si256(u11, u12, 0x31); // 0x31:0b00110001
    u11 = _mm256_xor_si256(left1, right1);
    u12 = right1;
    __m256i left2 = _mm256_permute2x128_si256(u21, u22, 0x20); // 0x20:0b00100000
    __m256i right2 = _mm256_permute2x128_si256(u21, u22, 0x31); // 0x31:0b00110001
    u21 = _mm256_xor_si256(left2, right2);
    u22 = right2;

    // level = 6;
    left1 = _mm256_unpacklo_epi64(u11, u12);
    right1 = _mm256_unpackhi_epi64(u11, u12);
    u11 = _mm256_xor_si256(left1, right1);
    u12 = right1;
    left2 = _mm256_unpacklo_epi64(u21, u22);
    right2 = _mm256_unpackhi_epi64(u21, u22);
    u21 = _mm256_xor_si256(left2, right2);
    u22 = right2;

    // level = 5;
    // shift+blend
    __m256i u12_sl = _mm256_slli_si256(u12, 4);
    left1 = _mm256_blend_epi32(u11, u12_sl, 0xAA); // 0xAA:0b10101010
    __m256i u11_sr = _mm256_srli_si256(u11, 4);
    right1 = _mm256_blend_epi32(u11_sr, u12, 0xAA); // 0xAA:0b10101010
    u11 = _mm256_xor_si256(left1, right1);
    u12 = right1;
    __m256i u22_sl = _mm256_slli_si256(u22, 4);
    left2 = _mm256_blend_epi32(u21, u22_sl, 0xAA); // 0xAA:0b10101010
    __m256i u21_sr = _mm256_srli_si256(u21, 4);
    right2 = _mm256_blend_epi32(u21_sr, u22, 0xAA); // 0xAA:0b10101010
    u21 = _mm256_xor_si256(left2, right2);
    u22 = right2;

    // level = 4;
    // shift+blend
    u12_sl = _mm256_slli_si256(u12, 2);
    left1 = _mm256_blend_epi16(u11, u12_sl, 0xAA); // 0xAA:0b10101010
    u11_sr = _mm256_srli_si256(u11, 2);
    right1 = _mm256_blend_epi16(u11_sr, u12, 0xAA); // 0xAA:0b10101010
    u11 = _mm256_xor_si256(left1, right1);
    u12 = right1;
    u22_sl = _mm256_slli_si256(u22, 2);
    left2 = _mm256_blend_epi16(u21, u22_sl, 0xAA); // 0xAA:0b10101010
    u21_sr = _mm256_srli_si256(u21, 2);
    right2 = _mm256_blend_epi16(u21_sr, u22, 0xAA); // 0xAA:0b10101010
    u21 = _mm256_xor_si256(left2, right2);
    u22 = right2;

    // level = 3;
    // shuffle+unpack
    __m256i mask = _mm256_set_epi8(15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0,
                                   15, 13, 11, 9, 7, 5, 3, 1, 14, 12, 10, 8, 6, 4, 2, 0);
    __m256i u11_lrlr = _mm256_shuffle_epi8(u11, mask);
    __m256i u12_lrlr = _mm256_shuffle_epi8(u12, mask);
    left1 = _mm256_unpacklo_epi64(u11_lrlr, u12_lrlr);
    right1 = _mm256_unpackhi_epi64(u11_lrlr, u12_lrlr);
    u11 = _mm256_xor_si256(left1, right1);
    u12 = right1;
    __m256i u21_lrlr = _mm256_shuffle_epi8(u21, mask);
    __m256i u22_lrlr = _mm256_shuffle_epi8(u22, mask);
    left2 = _mm256_unpacklo_epi64(u21_lrlr, u22_lrlr);
    right2 = _mm256_unpackhi_epi64(u21_lrlr, u22_lrlr);
    u21 = _mm256_xor_si256(left2, right2);
    u22 = right2;

    // level = 2;
    // AND+shift4+OR
    __m256i l_mask_4 = _mm256_set1_epi8(0x0F);
    __m256i u11_left = _mm256_and_si256(u11, l_mask_4);
    __m256i u12_left = _mm256_and_si256(u12, l_mask_4);
    __m256i u12_left_sl = _mm256_slli_epi64(u12_left, 4);
    left1 = _mm256_or_si256(u11_left, u12_left_sl);
    __m256i r_mask_4 = _mm256_set1_epi8((unsigned char)0xF0);
    __m256i u11_right = _mm256_and_si256(u11, r_mask_4);
    __m256i u12_right = _mm256_and_si256(u12, r_mask_4);
    __m256i u11_right_sr = _mm256_srli_epi64(u11_right, 4);
    right1 = _mm256_or_si256(u11_right_sr, u12_right);
    u11 = _mm256_xor_si256(left1, right1);
    u12 = right1;
    __m256i u21_left = _mm256_and_si256(u21, l_mask_4);
    __m256i u22_left = _mm256_and_si256(u22, l_mask_4);
    __m256i u22_left_sl = _mm256_slli_epi64(u22_left, 4);
    left2 = _mm256_or_si256(u21_left, u22_left_sl);
    __m256i u21_right = _mm256_and_si256(u21, r_mask_4);
    __m256i u22_right = _mm256_and_si256(u22, r_mask_4);
    __m256i u21_right_sr = _mm256_srli_epi64(u21_right, 4);
    right2 = _mm256_or_si256(u21_right_sr, u22_right);
    u21 = _mm256_xor_si256(left2, right2);
    u22 = right2;

    // level = 1;
    // AND+shift2+OR
    __m256i l_mask_2 = _mm256_set1_epi8(0x33);
    u11_left = _mm256_and_si256(u11, l_mask_2);
    u12_left = _mm256_and_si256(u12, l_mask_2);
    u12_left_sl = _mm256_slli_epi64(u12_left, 2);
    left1 = _mm256_or_si256(u11_left, u12_left_sl);
    __m256i r_mask_2 = _mm256_set1_epi8((unsigned char)0xCC);
    u11_right = _mm256_and_si256(u11, r_mask_2);
    u12_right = _mm256_and_si256(u12, r_mask_2);
    u11_right_sr = _mm256_srli_epi64(u11_right, 2);
    right1 = _mm256_or_si256(u11_right_sr, u12_right);
    u11 = _mm256_xor_si256(left1, right1);
    u12 = right1;
    u21_left = _mm256_and_si256(u21, l_mask_2);
    u22_left = _mm256_and_si256(u22, l_mask_2);
    u22_left_sl = _mm256_slli_epi64(u22_left, 2);
    left2 = _mm256_or_si256(u21_left, u22_left_sl);
    u21_right = _mm256_and_si256(u21, r_mask_2);
    u22_right = _mm256_and_si256(u22, r_mask_2);
    u21_right_sr = _mm256_srli_epi64(u21_right, 2);
    right2 = _mm256_or_si256(u21_right_sr, u22_right);
    u21 = _mm256_xor_si256(left2, right2);
    u22 = right2;

    // level = 0;
    // AND+shift1+OR
    __m256i l_mask_1 = _mm256_set1_epi8(0x55);
    u11_left = _mm256_and_si256(u11, l_mask_1);
    u12_left = _mm256_and_si256(u12, l_mask_1);
    u12_left_sl = _mm256_slli_epi64(u12_left, 1);
    left1 = _mm256_or_si256(u11_left, u12_left_sl);
    __m256i r_mask_1 = _mm256_set1_epi8((unsigned char)0xAA);
    u11_right = _mm256_and_si256(u11, r_mask_1);
    u12_right = _mm256_and_si256(u12, r_mask_1);
    u11_right_sr = _mm256_srli_epi64(u11_right, 1);
    right1 = _mm256_or_si256(u11_right_sr, u12_right);
    u11 = _mm256_xor_si256(left1, right1);
    u12 = right1;
    
    mask = _mm256_set_epi8(15, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 0,
                           15, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 0);
    left1 = _mm256_shuffle_epi8(u11, mask);
    right1 = _mm256_shuffle_epi8(u12, mask);
    __m256i left1_even = _mm256_and_si256(left1, l_mask_1); // 0x55:0b01010101
    __m256i left1_odd = _mm256_and_si256(left1, r_mask_1); // 0xAA:0b10101010
    __m256i right1_even = _mm256_and_si256(right1, l_mask_1); // 0x55:0b01010101
    __m256i right1_odd = _mm256_and_si256(right1, r_mask_1); // 0xAA:0b10101010
    left1 = _mm256_or_si256(left1_even, _mm256_slli_epi64(right1_even, 1));
    right1 = _mm256_or_si256(_mm256_srli_epi64(left1_odd, 1), right1_odd);
    
    left1_even = _mm256_and_si256(left1, l_mask_2); // 0x33:0b00110011
    left1_odd = _mm256_and_si256(left1, r_mask_2); // 0xCC:0b11001100
    right1_even = _mm256_and_si256(right1, l_mask_2); // 0x33:0b00110011
    right1_odd = _mm256_and_si256(right1, r_mask_2); // 0xCC:0b11001100
    left1 = _mm256_or_si256(left1_even, _mm256_slli_epi64(right1_even, 2));
    right1 = _mm256_or_si256(_mm256_srli_epi64(left1_odd, 2), right1_odd);
    
    left1_even = _mm256_and_si256(left1, l_mask_4); // 0x0F:0b00001111
    left1_odd = _mm256_and_si256(left1, r_mask_4); // 0xF0:0b11110000
    right1_even = _mm256_and_si256(right1, l_mask_4); // 0x0F:0b00001111
    right1_odd = _mm256_and_si256(right1, r_mask_4); // 0xF0:0b11110000
    left1 = _mm256_or_si256(left1_even, _mm256_slli_epi64(right1_even, 4));
    right1 = _mm256_or_si256(_mm256_srli_epi64(left1_odd, 4), right1_odd);

    __m256i u11_0145 = _mm256_unpacklo_epi8(left1, right1);
    __m256i u12_2367 = _mm256_unpackhi_epi8(left1, right1);
    u11 = _mm256_permute2x128_si256(u11_0145, u12_2367, 0x20); // 0x20:0b00100000
    u12 = _mm256_permute2x128_si256(u11_0145, u12_2367, 0x31); // 0x31:0b00110001

    _mm256_store_si256((__m256i *)u_8, u11);
    _mm256_store_si256((__m256i *)(u_8 + 32), u12);

    u21_left = _mm256_and_si256(u21, l_mask_1);
    u22_left = _mm256_and_si256(u22, l_mask_1);
    u22_left_sl = _mm256_slli_epi64(u22_left, 1);
    left2 = _mm256_or_si256(u21_left, u22_left_sl);
    u21_right = _mm256_and_si256(u21, r_mask_1);
    u22_right = _mm256_and_si256(u22, r_mask_1);
    u21_right_sr = _mm256_srli_epi64(u21_right, 1);
    right2 = _mm256_or_si256(u21_right_sr, u22_right);
    u21 = _mm256_xor_si256(left2, right2);
    u22 = right2;
    
    left2 = _mm256_shuffle_epi8(u21, mask);
    right2 = _mm256_shuffle_epi8(u22, mask);
    __m256i left2_even = _mm256_and_si256(left2, l_mask_1); // 0x55:0b01010101
    __m256i left2_odd = _mm256_and_si256(left2, r_mask_1); // 0xAA:0b10101010
    __m256i right2_even = _mm256_and_si256(right2, l_mask_1); // 0x55:0b01010101
    __m256i right2_odd = _mm256_and_si256(right2, r_mask_1); // 0xAA:0b10101010
    left2 = _mm256_or_si256(left2_even, _mm256_slli_epi64(right2_even, 1));
    right2 = _mm256_or_si256(_mm256_srli_epi64(left2_odd, 1), right2_odd);
    
    left2_even = _mm256_and_si256(left2, l_mask_2); // 0x33:0b00110011
    left2_odd = _mm256_and_si256(left2, r_mask_2); // 0xCC:0b11001100
    right2_even = _mm256_and_si256(right2, l_mask_2); // 0x33:0b00110011
    right2_odd = _mm256_and_si256(right2, r_mask_2); // 0xCC:0b11001100
    left2 = _mm256_or_si256(left2_even, _mm256_slli_epi64(right2_even, 2));
    right2 = _mm256_or_si256(_mm256_srli_epi64(left2_odd, 2), right2_odd);
    
    left2_even = _mm256_and_si256(left2, l_mask_4); // 0x0F:0b00001111
    left2_odd = _mm256_and_si256(left2, r_mask_4); // 0xF0:0b11110000
    right2_even = _mm256_and_si256(right2, l_mask_4); // 0x0F:0b00001111
    right2_odd = _mm256_and_si256(right2, r_mask_4); // 0xF0:0b11110000
    left2 = _mm256_or_si256(left2_even, _mm256_slli_epi64(right2_even, 4));
    right2 = _mm256_or_si256(_mm256_srli_epi64(left2_odd, 4), right2_odd);

    __m256i u21_0145 = _mm256_unpacklo_epi8(left2, right2);
    __m256i u22_2367 = _mm256_unpackhi_epi8(left2, right2);
    u21 = _mm256_permute2x128_si256(u21_0145, u22_2367, 0x20); // 0x20:0b00100000
    u22 = _mm256_permute2x128_si256(u21_0145, u22_2367, 0x31); // 0x31:0b00110001

    _mm256_store_si256((__m256i *)(u_8 + 64), u21);
    _mm256_store_si256((__m256i *)(u_8 + 96), u22);
}
void encode_polar_opt(uint64_t *u_64)
{
    int base;
    uint64_t left, right, new_left, new_right;
    uint64_t left_even, left_odd, right_even, right_odd;
    
    // level = 9;
    for (int offset = 0; offset < 8; offset++)
    {
        u_64[offset] = u_64[offset] ^ u_64[offset + 8];
    }
    
    // level = 8;
    for (int block = 0; block < 2; block++)
    {
        base = 8 * block;
        for(int offset = 0; offset < 4; offset++)
        {
            u_64[base + offset] = u_64[base + offset] ^ u_64[base + offset + 4];
        }
    }

    // level = 7;
    for (int block = 0; block < 4; block++)
    {
        base = 4 * block;
        for(int offset = 0; offset < 2; offset++)
        {
            u_64[base + offset] = u_64[base + offset] ^ u_64[base + offset + 2];
        }
    }

    // level = 6;
    for (int block = 0; block < 8; block++)
    {
        base = 2 * block;
        u_64[base] = u_64[base] ^ u_64[base + 1];
    }

    // level = 5;
    for (int block = 0; block < 8; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x00000000FFFFFFFF) | (right << 32);
        new_right = (right & 0xFFFFFFFF00000000) | (left >> 32);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 4;
    for (int block = 0; block < 8; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x0000FFFF0000FFFF) | ((right & 0x0000FFFF0000FFFF) << 16);
        new_right = (right & 0xFFFF0000FFFF0000) | ((left & 0xFFFF0000FFFF0000) >> 16);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 3;
    for (int block = 0; block < 8; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x00FF00FF00FF00FF) | ((right & 0x00FF00FF00FF00FF) << 8);
        new_right = (right & 0xFF00FF00FF00FF00) | ((left & 0xFF00FF00FF00FF00) >> 8);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 2;
    for (int block = 0; block < 8; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x0F0F0F0F0F0F0F0F) | ((right & 0x0F0F0F0F0F0F0F0F) << 4);
        new_right = (right & 0xF0F0F0F0F0F0F0F0) | ((left & 0xF0F0F0F0F0F0F0F0) >> 4);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 1;
    for (int block = 0; block < 8; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x3333333333333333) | ((right & 0x3333333333333333) << 2);
        new_right = (right & 0xCCCCCCCCCCCCCCCC) | ((left & 0xCCCCCCCCCCCCCCCC) >> 2);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;
    }

    // level = 0;
    for (int block = 0; block < 8; block++)
    {
        base = 2 * block;
        left = u_64[base];
        right = u_64[base + 1];
        new_left = (left & 0x5555555555555555) | ((right & 0x5555555555555555) << 1);
        new_right = (right & 0xAAAAAAAAAAAAAAAA) | ((left & 0xAAAAAAAAAAAAAAAA) >> 1);
        u_64[base] = new_left ^ new_right;
        u_64[base + 1] = new_right;

        left_even = u_64[base] & 0x5555555555555555;
        left_odd = u_64[base] & 0xAAAAAAAAAAAAAAAA;
        right_even = u_64[base + 1] & 0x5555555555555555;
        right_odd = u_64[base + 1] & 0xAAAAAAAAAAAAAAAA;
        u_64[base] = left_even | (right_even << 1);
        u_64[base + 1] = (left_odd >> 1) | right_odd;

        left_even = u_64[base] & 0x3333333333333333;
        left_odd = u_64[base] & 0xCCCCCCCCCCCCCCCC;
        right_even = u_64[base + 1] & 0x3333333333333333;
        right_odd = u_64[base + 1] & 0xCCCCCCCCCCCCCCCC;
        u_64[base] = left_even | (right_even << 2);
        u_64[base + 1] = (left_odd >> 2) | right_odd;

        left_even = u_64[base] & 0x0F0F0F0F0F0F0F0F;
        left_odd = u_64[base] & 0xF0F0F0F0F0F0F0F0;
        right_even = u_64[base + 1] & 0x0F0F0F0F0F0F0F0F;
        right_odd = u_64[base + 1] & 0xF0F0F0F0F0F0F0F0;
        u_64[base] = left_even | (right_even << 4);
        u_64[base + 1] = (left_odd >> 4) | right_odd;

        left_even = u_64[base] & 0x00FF00FF00FF00FF;
        left_odd = u_64[base] & 0xFF00FF00FF00FF00;
        right_even = u_64[base + 1] & 0x00FF00FF00FF00FF;
        right_odd = u_64[base + 1] & 0xFF00FF00FF00FF00;
        u_64[base] = left_even | (right_even << 8);
        u_64[base + 1] = (left_odd >> 8) | right_odd;

        left_even = u_64[base] & 0x0000FFFF0000FFFF;
        left_odd = u_64[base] & 0xFFFF0000FFFF0000;
        right_even = u_64[base + 1] & 0x0000FFFF0000FFFF;
        right_odd = u_64[base + 1] & 0xFFFF0000FFFF0000;
        u_64[base] = left_even | (right_even << 16);
        u_64[base + 1] = (left_odd >> 16) | right_odd;

        left_even = u_64[base] & 0x00000000FFFFFFFF;
        left_odd = u_64[base] & 0xFFFFFFFF00000000;
        right_even = u_64[base + 1] & 0x00000000FFFFFFFF;
        right_odd = u_64[base + 1] & 0xFFFFFFFF00000000;
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
    __attribute__((aligned(32))) uint8_t x_cap[polar.N/8];
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
