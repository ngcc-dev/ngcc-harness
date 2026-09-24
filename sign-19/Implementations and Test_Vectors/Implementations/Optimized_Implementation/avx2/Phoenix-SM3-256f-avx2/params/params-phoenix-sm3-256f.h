#ifndef SPX_PARAMS_H
#define SPX_PARAMS_H

#define SPX_NAMESPACE(s) SPX_##s

/* Hash output length in bytes. */
#define SPX_N 32
/* Height of the hypertree. */
#define SPX_FULL_HEIGHT 64
/* Number of subtree layer. */
#define SPX_D 16
/* TFORS tree dimensions. */
#define SPX_TFORS_A 8
#define SPX_TFORS_K_PRIME 64
#define SPX_TFORS_LOG_K_PRIME 6
#define SPX_TFORS_K 47
#define SPX_TFORS_HEIGHT (SPX_TFORS_LOG_K_PRIME + SPX_TFORS_A)
#define SPX_TFORS_T (1 << SPX_TFORS_A)
/* Winternitz parameter, */
#define SPX_GWOTS_W1 16
#define SPX_GWOTS_LOGW1 4
#define SPX_GWOTS_W2 32
#define SPX_GWOTS_LOGW2 5

/* The hash function is defined by linking a different hash.c file, as opposed
   to setting a #define constant. */

/* For clarity */
#define SPX_ADDR_BYTES 32

/* GWOTS parameters. */
#define SPX_GWOTS_W1_LEN 39
#define SPX_GWOTS_W2_LEN 20
#define SPX_GWOTS_LEN1 (SPX_GWOTS_W1_LEN + SPX_GWOTS_W2_LEN)

/* SPX_GWOTS_LEN2 is fixed */
#define SPX_GWOTS_LEN2 1

#define SPX_GWOTS_LEN (SPX_GWOTS_LEN1 + SPX_GWOTS_LEN2)
#define SPX_GWOTS_BYTES (SPX_GWOTS_LEN * SPX_N)
#define SPX_GWOTS_PK_BYTES SPX_GWOTS_BYTES

/* Subtree size. */
#define SPX_TREE_HEIGHT (SPX_FULL_HEIGHT / SPX_D)

#if SPX_TREE_HEIGHT * SPX_D != SPX_FULL_HEIGHT
#error SPX_D should always divide SPX_FULL_HEIGHT
#endif

/* TFORS parameters. */
#define SPX_TFORS_MSG_BYTES ((SPX_TFORS_A * SPX_TFORS_K + 7) / 8 + SPX_N)
#define SPX_TFORS_BYTES (SPX_TFORS_K * SPX_N + \
                         SPX_TFORS_K * SPX_TFORS_HEIGHT * SPX_N)
#define SPX_TFORS_SIG_MAX 12036
#define SPX_TFORS_PK_BYTES SPX_N

/* custom upgrade parameter definitions */
#define COUNTER_SIZE 4


/* --- GWOTS+C Automatic Parameter Calculation --- */
/* Winternitz parameter for the single checksum chain */
#define SPX_GWOTS_CHECKSUM_W 16
#define SPX_GWOTS_CHECKSUM_LOGW 4

/*
 * Calculate the expected average sum (E) of the message chains.
 * Formula: E = Sum of (Length_i * (W_i - 1) / 2)
 * We multiply by Length first to avoid floating point issues in macros.
 */
#define GWOTS_EXPECTED_SUM (                   \
    ((SPX_GWOTS_W1_LEN * (SPX_GWOTS_W1 - 1)) +  \
     (SPX_GWOTS_W2_LEN * (SPX_GWOTS_W2 - 1))) / \
    2)

/*
 * GWOTS_SUM_BASE: The starting point of the acceptable sum range.
 * We center the range around the expected average sum.
 */
#define GWOTS_SUM_BASE (GWOTS_EXPECTED_SUM - (SPX_GWOTS_CHECKSUM_W / 2))

/*
 * GWOTS_SUM_RANGE: The total number of sum values covered by one checksum chain.
 * For a chain with Winternitz parameter W, it can represent W distinct values (0 to W-1).
 */
#define GWOTS_SUM_RANGE ((SPX_GWOTS_CHECKSUM_W) - 1)

/*
 * Note: The valid sum interval is [GWOTS_SUM_BASE, GWOTS_SUM_BASE + GWOTS_SUM_RANGE)
 * For n=256, W1=16, W2=32, W1_LEN=39, W2_LEN=20, CHECKSUM_W=16:
 * E = (39*15 + 20*31) / 2 = 585 + 620) / 2 = 602
 * BASE = 602 - 8 = 594
 * RANGE = 16 - 1 = 15
 */


/* Resulting SPX sizes. */
#define SPX_BYTES (SPX_N + COUNTER_SIZE + 2 + SPX_TFORS_SIG_MAX + SPX_D * (SPX_GWOTS_BYTES + COUNTER_SIZE) + \
                   SPX_FULL_HEIGHT * SPX_N)
#define SPX_PK_BYTES (2 * SPX_N)
#define SPX_SK_BYTES (2 * SPX_N + SPX_PK_BYTES)

#include "../sm3_offsets.h"

#endif
