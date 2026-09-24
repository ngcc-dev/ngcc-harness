#ifndef SPX_PARAMS_H
#define SPX_PARAMS_H

/* Hash output length in bytes. */
#define SPX_N 32
/* Height of the hypertree. */
#define SPX_FULL_HEIGHT 66
/* Number of subtree layer. */
#define SPX_D 14
/* FORS tree dimensions. */
#define SPX_FORS_HEIGHT 8
#define SPX_FORS_TREES 44 
/* Winternitz parameter, */

#define SPX_WOTS_W_ARRAY {8, 8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16}
#define SPX_WOTS_LOGW_ARRAY {3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}
#define SPX_MAX_WOTS_W 16

#define SPX_WOTS_LEN 64
#define WOTS_ZERO_BITS 2
#define SPX_FORS_ZERO_LAST_BITS 10 
#define WANTED_CHECKSUM  472  /*(SUM_W - SPX_WOTS_LEN) / 2)*/

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

/* FORS parameters. */
#define SPX_FORS_MSG_BYTES ((SPX_FORS_HEIGHT * SPX_FORS_TREES + 7) / 8)
#define SPX_FORS_BYTES ((SPX_FORS_HEIGHT + 1) * SPX_FORS_TREES * SPX_N)
#define SPX_FORS_PK_BYTES SPX_N

/* Resulting SPX sizes. */
#define SPX_BYTES ((SPX_N + SPX_FORS_BYTES + SPX_D * SPX_WOTS_BYTES +\
                   SPX_FULL_HEIGHT * SPX_N+(SPX_D*COUNTER_SIZE))+COUNTER_SIZE)
#define SPX_PK_BYTES (2 * SPX_N)
#define SPX_SK_BYTES (2 * SPX_N + SPX_PK_BYTES)

#include "../sm3_offsets.h"

/* custom upgrade parameter definitions */
#define COUNTER_SIZE 4


#endif
