/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef __DEFS_H_INCLUDED__
#define __DEFS_H_INCLUDED__

////////////////////////////////////////////
//         BIKE main parameters
///////////////////////////////////////////

// Select the public security-size parameter set. Prefer BIKE_SECURITY_128,
// BIKE_SECURITY_256 and BIKE_SECURITY_512 in new code and commands.
// PARAM64/PARAM96/PARAM128/PARAM256 remain as compatibility aliases for older
// scripts and pretrained model metadata.
#if !defined(BIKE_SECURITY_128) && !defined(BIKE_SECURITY_192) && \
    !defined(BIKE_SECURITY_256) && !defined(BIKE_SECURITY_512) && \
    !defined(PARAM64) && !defined(PARAM96) && !defined(PARAM128) && \
    !defined(PARAM256)
#define BIKE_SECURITY_128
#endif

#if defined(PARAM64)
#define BIKE_SECURITY_128
#endif
#if defined(PARAM96)
#define BIKE_SECURITY_192
#endif
#if defined(PARAM128)
#define BIKE_SECURITY_256
#endif
#if defined(PARAM256)
#define BIKE_SECURITY_512
#endif

#if (defined(BIKE_SECURITY_128) + defined(BIKE_SECURITY_192) + \
     defined(BIKE_SECURITY_256) + defined(BIKE_SECURITY_512)) != 1
#error "Define exactly one of BIKE_SECURITY_128, BIKE_SECURITY_192, BIKE_SECURITY_256 or BIKE_SECURITY_512"
#endif

// UNCOMMENT TO ENABLE BANDWIDTH OPTIMISATION FOR BIKE-3:
//#define BANDWIDTH_OPTIMIZED

#ifndef ELL_BITS
#ifdef BIKE_SECURITY_512
#define ELL_BITS  512ULL
#else
#define ELL_BITS  256ULL
#endif
#endif
#define ELL_SIZE (ELL_BITS/8)

#if (ELL_BITS % 8ULL) != 0
#error "ELL_BITS must be byte-aligned"
#endif

#if (ELL_BITS % 64ULL) != 0
#error "ELL_BITS must be 64-bit aligned"
#endif

#ifndef BIKE_MLTHRE_ENABLED
#define BIKE_MLTHRE_ENABLED 1
#endif

#ifndef LA_TS_ENABLED
#define LA_TS_ENABLED BIKE_MLTHRE_ENABLED
#endif

#ifndef BIKE_MLTHRE_512_MODEL_ENABLED
#define BIKE_MLTHRE_512_MODEL_ENABLED 0
#endif

////////////////////////////////////////////
// Implicit Parameters (do NOT edit below)
///////////////////////////////////////////

// select the max between a and b:
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

// 512-bit-classical candidate parameters:
#ifdef BIKE_SECURITY_512
#ifndef BIKE_512_R_BITS
#if defined(PARAM256_R_BITS)
#define BIKE_512_R_BITS PARAM256_R_BITS
#else
#define BIKE_512_R_BITS 150001ULL
#endif
#endif
#ifndef BIKE_512_DV
#if defined(PARAM256_DV)
#define BIKE_512_DV PARAM256_DV
#else
#define BIKE_512_DV 273ULL
#endif
#endif
#ifndef BIKE_512_T1
#if defined(PARAM256_T1)
#define BIKE_512_T1 PARAM256_T1
#else
#define BIKE_512_T1 524ULL
#endif
#endif
#ifndef BIKE_512_TH_A
#if defined(PARAM256_TH_A)
#define BIKE_512_TH_A PARAM256_TH_A
#else
#define BIKE_512_TH_A 0.0
#endif
#endif
#ifndef BIKE_512_TH_B
#if defined(PARAM256_TH_B)
#define BIKE_512_TH_B PARAM256_TH_B
#else
#define BIKE_512_TH_B 0.0
#endif
#endif
#ifndef BIKE_512_TH_MIN
#if defined(PARAM256_TH_MIN)
#define BIKE_512_TH_MIN PARAM256_TH_MIN
#else
#define BIKE_512_TH_MIN 0
#endif
#endif
#ifndef BIKE_512_TAU
#if defined(PARAM256_TAU)
#define BIKE_512_TAU PARAM256_TAU
#else
#define BIKE_512_TAU 3
#endif
#endif
#ifndef BIKE_512_NBITER
#if defined(PARAM256_NBITER)
#define BIKE_512_NBITER PARAM256_NBITER
#else
#define BIKE_512_NBITER 5
#endif
#endif
#define BIKE_SECURITY_BITS 512
#define BIKE_MODEL_SECURITY_LEVEL 7
#define R_BITS BIKE_512_R_BITS
#define DV     BIKE_512_DV
#define T1     BIKE_512_T1
#define VAR_TH_FCT(x) (MAX(BIKE_512_TH_A + BIKE_512_TH_B * (x), BIKE_512_TH_MIN))
#define tau BIKE_512_TAU
#define NbIter BIKE_512_NBITER
// 256-bit security parameters:
#elif defined(BIKE_SECURITY_256)
#define BIKE_SECURITY_BITS 256
#define BIKE_MODEL_SECURITY_LEVEL 5
#define R_BITS 40973ULL
#define DV     137ULL
#define T1     264ULL
#define VAR_TH_FCT(x) (MAX(17.8785 + 0.00402312 * (x), 69))
// Parameters for BGF Decoder:
#define tau 3
#ifndef MLTHRE_NBITER_OVERRIDE
#define NbIter 5
#else
#define NbIter MLTHRE_NBITER_OVERRIDE
#endif
// Legacy middle parameter set. Kept for compatibility; prefer 128/256/512.
#elif defined(BIKE_SECURITY_192)
#define BIKE_SECURITY_BITS 192
#define BIKE_MODEL_SECURITY_LEVEL 3
#define R_BITS 24659ULL
#define DV     103ULL
#define T1     199ULL
#define VAR_TH_FCT(x) (MAX(15.2588 + 0.005265 * (x), 52))
// Parameters for BGF Decoder:
#define tau 3
#ifndef MLTHRE_NBITER_OVERRIDE
#define NbIter 5
#else
#define NbIter MLTHRE_NBITER_OVERRIDE
#endif
// 128-bit security parameters:
#elif defined(BIKE_SECURITY_128)
#define BIKE_SECURITY_BITS 128
#define BIKE_MODEL_SECURITY_LEVEL 1
#define R_BITS 12323ULL
#define DV     71ULL
#define T1     134ULL
#define VAR_TH_FCT(x) (MAX(13.530 + 0.0069722 * (x), 36))
// Parameters for BGF Decoder:
#define tau 3
#ifndef MLTHRE_NBITER_OVERRIDE
#define NbIter 5
#else
#define NbIter MLTHRE_NBITER_OVERRIDE
#endif

// Backward-compatible name for code that only displays the public level.
#define BIKE_SECURITY_LEVEL BIKE_SECURITY_BITS
#endif

// Divide by the divider and round up to next integer:
#define DIVIDE_AND_CEIL(x, divider)  ((x/divider) + (x % divider == 0 ? 0 : 1ULL))

// Round the size to the nearest byte.
// SIZE suffix, is the number of bytes (uint8_t).
#define N_BITS   (R_BITS*2)
#define R_SIZE   DIVIDE_AND_CEIL(R_BITS, 8ULL)
#define N_SIZE   DIVIDE_AND_CEIL(N_BITS, 8ULL)
#define R_DQWORDS DIVIDE_AND_CEIL(R_SIZE, 16ULL)

////////////////////////////////////////////
//             Debug
///////////////////////////////////////////

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if (VERBOSE == 3)
#define MSG(...)     { printf(__VA_ARGS__); }
#define DMSG(...)    MSG(__VA_ARGS__)
#define EDMSG(...)   MSG(__VA_ARGS__)
#define SEDMSG(...)  MSG(__VA_ARGS__)
#elif (VERBOSE == 2)
#define MSG(...)     { printf(__VA_ARGS__); }
#define DMSG(...)    MSG(__VA_ARGS__)
#define EDMSG(...)   MSG(__VA_ARGS__)
#define SEDMSG(...)
#elif (VERBOSE == 1)
#define MSG(...)     { printf(__VA_ARGS__); }
#define DMSG(...)    MSG(__VA_ARGS__)
#define EDMSG(...)
#define SEDMSG(...)
#else
#define MSG(...)     { printf(__VA_ARGS__); }
#define DMSG(...)
#define EDMSG(...)
#define SEDMSG(...)
#endif

////////////////////////////////////////////
//              Printing
///////////////////////////////////////////

// Show timer results in cycles.
#define RDTSC

//#define PRINT_IN_BE
//#define NO_SPACE
//#define NO_NEWLINE

////////////////////////////////////////////
//              Testing
///////////////////////////////////////////
#ifndef NUM_OF_CODE_TESTS
#define NUM_OF_CODE_TESTS       100ULL
#endif

#ifndef NUM_OF_ENCRYPTION_TESTS
#define NUM_OF_ENCRYPTION_TESTS 100ULL
#endif

#endif //__TYPES_H_INCLUDED__
