/* Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0"
 *
 * Written by Nir Drucker, Shay Gueron and Dusan Kostic,
 * AWS Cryptographic Algorithms Group.
 */

#pragma once

#include "defs.h"

////////////////////////////////////////////
//             BIKE Parameters
///////////////////////////////////////////
#define N0 2

#if !defined(SECURITY_BITS)
#  if defined(LEVEL)
#    if(LEVEL == 1)
#      define SECURITY_BITS 128
#    elif(LEVEL == 3)
#      define SECURITY_BITS 192
#    elif(LEVEL == 5)
#      define SECURITY_BITS 256
#    elif(LEVEL == 7)
#      define SECURITY_BITS 512
#    else
#      error "Bad internal LEVEL, choose one of 1/3/5/7"
#    endif
#  else
#    define SECURITY_BITS 128
#  endif
#endif

#if !defined(LEVEL)
#  if(SECURITY_BITS == 128)
#    define LEVEL 1
#  elif(SECURITY_BITS == 192)
#    define LEVEL 3
#  elif(SECURITY_BITS == 256)
#    define LEVEL 5
#  elif(SECURITY_BITS == 512)
#    define LEVEL 7
#  else
#    error "Bad SECURITY_BITS, choose 128, 256, or 512. 192 is kept for compatibility."
#  endif
#endif

#if(LEVEL == 1)
// 128-bit security target, internally mapped to the BIKE Level-1 parameter set.
#  define R_BITS 12323
#  define D      71
#  define T      134

#  define THRESHOLD_COEFF0 1353000000ULL
#  define THRESHOLD_COEFF1 697220ULL
#  define THRESHOLD_MUL_CONST 12379400392853802749ULL
#  define THRESHOLD_SHR_CONST 26
#  define THRESHOLD_MIN 36

// When generating an error vector we can't use rejection sampling because of
// constant-time requirements so we generate always the maximum number
// of indices and then use only the first T valid indices, as explained in:
// https://github.com/awslabs/bike-kem/blob/master/BIKE_Rejection_Sampling.pdf
#  define MAX_RAND_INDICES_T 271

// The gf2x code is optimized to a block in this case:
#  define BLOCK_BITS 16384

#elif(LEVEL == 3)
// 192-bit compatibility parameter set. The main supported targets are 128/256/512.
#  define R_BITS 24659
#  define D      103
#  define T      199

#  define THRESHOLD_COEFF0 1525880000ULL
#  define THRESHOLD_COEFF1 526500ULL
#  define THRESHOLD_MUL_CONST 12379400392853802749ULL
#  define THRESHOLD_SHR_CONST 26
#  define THRESHOLD_MIN 52

#  define MAX_RAND_INDICES_T 373

// The gf2m code is optimized to a block in this case:
#  define BLOCK_BITS 32768

#elif (LEVEL == 5)
// 256-bit security target, internally mapped to the BIKE Level-5 parameter set.
#  define R_BITS 40973
#  define D      137
#  define T      264

#  define THRESHOLD_COEFF0 1787850000ULL
#  define THRESHOLD_COEFF1 402312ULL
#  define THRESHOLD_MUL_CONST 12379400392853802749ULL
#  define THRESHOLD_SHR_CONST 26
#  define THRESHOLD_MIN 69

#  define MAX_RAND_INDICES_T 605

#  define BLOCK_BITS 65536
#elif (LEVEL == 7)
// Experimental 512-bit security target.
// This is not a BIKE standard parameter set. The threshold constants are a
// coarse extrapolation for initial decoder experiments only.
#  define R_BITS 150001
#  define D      273
#  define T      524

#  define THRESHOLD_COEFF0 0ULL
#  define THRESHOLD_COEFF1 0ULL
#  define THRESHOLD_MUL_CONST 12379400392853802749ULL
#  define THRESHOLD_SHR_CONST 26
#  if !defined(BIKE_L7_FIXED_THRESHOLD)
#    define BIKE_L7_FIXED_THRESHOLD 137
#  endif
#  define THRESHOLD_MIN BIKE_L7_FIXED_THRESHOLD

#  define MAX_RAND_INDICES_T 1200

#  define BLOCK_BITS 262144
#else
#  error "Bad level, choose one of 1/3/5/7"
#endif

#define NUM_OF_SEEDS 2

// Round the size to the nearest byte.
// SIZE suffix, is the number of bytes (uint8_t).
#define N_BITS   (R_BITS * N0)
#define R_BYTES  DIVIDE_AND_CEIL(R_BITS, 8)
#define R_QWORDS DIVIDE_AND_CEIL(R_BITS, 8 * BYTES_IN_QWORD)
#define R_XMM    DIVIDE_AND_CEIL(R_BITS, 8 * BYTES_IN_XMM)
#define R_YMM    DIVIDE_AND_CEIL(R_BITS, 8 * BYTES_IN_YMM)
#define R_ZMM    DIVIDE_AND_CEIL(R_BITS, 8 * BYTES_IN_ZMM)

#define R_BLOCKS        DIVIDE_AND_CEIL(R_BITS, BLOCK_BITS)
#define R_PADDED        (R_BLOCKS * BLOCK_BITS)
#define R_PADDED_BYTES  (R_PADDED / 8)
#define R_PADDED_QWORDS (R_PADDED / 64)

#define LAST_R_QWORD_LEAD  (R_BITS & MASK(6))
#define LAST_R_QWORD_TRAIL (64 - LAST_R_QWORD_LEAD)
#define LAST_R_QWORD_MASK  MASK(LAST_R_QWORD_LEAD)

#define LAST_R_BYTE_LEAD  (R_BITS & MASK(3))
#define LAST_R_BYTE_TRAIL (8 - LAST_R_BYTE_LEAD)
#define LAST_R_BYTE_MASK  MASK(LAST_R_BYTE_LEAD)

// Data alignement
#define ALIGN_BYTES (BYTES_IN_ZMM)

#if(SECURITY_BITS == 512)
#  define M_BITS 512
#else
#  define M_BITS 256
#endif
#define M_BYTES (M_BITS / 8)

#define SS_BITS  M_BITS
#define SS_BYTES (SS_BITS / 8)

#define SEED_BYTES M_BYTES

//////////////////////////////////
// Parameters for the BGF decoder.
//////////////////////////////////
#define BGF_DECODER
#ifndef DELTA
#  define DELTA 3
#endif
#define SLICES (LOG2_MSB(D) + 1)
