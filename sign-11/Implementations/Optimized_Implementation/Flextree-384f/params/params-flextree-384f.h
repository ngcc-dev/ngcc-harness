#ifndef SPX_PARAMS_H
#define SPX_PARAMS_H

/* Hash output length in bytes. */
#define SPX_N 48
/* Height of the hypertree. */
#define SPX_FULL_HEIGHT 64
/* Number of subtree layer. */
#define SPX_D 12
/* PORS+FP dimensions. */
#define SPX_PORS_FP_T 71828
#define SPX_PORS_FP_K 53
/* Winternitz parameter, */

#define SPX_WOTS_W_ARRAY {16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32}
#define SPX_WOTS_LOGW_ARRAY {4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5}

#define SPX_WOTS_LEN 77
#define WOTS_ZERO_BITS 2
#define WANTED_CHECKSUM  1169  /*(SUM_W - SPX_WOTS_LEN) / 2)*/

/* The hash function is defined by linking a different hash.c file, as opposed
   to setting a #define constant. */

/* For clarity */
#define SPX_ADDR_BYTES 32

#define SPX_WOTS_BYTES (SPX_WOTS_LEN * SPX_N)
#define SPX_WOTS_PK_BYTES SPX_WOTS_BYTES

/* Subtree size. */
#define SPX_TREE_HEIGHT (SPX_FULL_HEIGHT / SPX_D)

#if SPX_TREE_HEIGHT * SPX_D != SPX_FULL_HEIGHT
    #define SPX_BOTTOM_TREE_HEIGHT  SPX_TREE_HEIGHT + 1
#else
    #define SPX_BOTTOM_TREE_HEIGHT  SPX_TREE_HEIGHT 
#endif

/* PORS+FP parameters. */
#define SPX_PORS_FP_MSG_BYTES SPX_N
#define SPX_PORS_FP_MAX_AUTH_NODES 486
#define SPX_PORS_FP_BYTES ((COUNTER_SIZE + (SPX_PORS_FP_K + SPX_PORS_FP_MAX_AUTH_NODES) * SPX_N))
#define SPX_PORS_FP_PK_BYTES SPX_N

/* Resulting SPX sizes. */
#define SPX_BYTES (SPX_N + SPX_PORS_FP_BYTES + SPX_D * SPX_WOTS_BYTES +\
                   SPX_FULL_HEIGHT * SPX_N + (SPX_D * COUNTER_SIZE))
#define SPX_PK_BYTES (2 * SPX_N)
#define SPX_SK_BYTES (2 * SPX_N + SPX_PK_BYTES)

#include "../sm3_offsets.h"

/* custom upgrade parameter definitions */
#define COUNTER_SIZE 4


#endif
