#ifndef SPX_PARAMS_H
#define SPX_PARAMS_H

/* Hash output length in bytes. */
#define SPX_N 32
/* Height of the hypertree. */
#define SPX_FULL_HEIGHT 67
/* Number of subtree layer. */
#define SPX_D 9
/* FORS tree dimensions. */
#define SPX_FORS_HEIGHT 12
#define SPX_FORS_TREES 23
/* Winternitz parameter, */
#define SPX_WOTS_W_ARRAY {46, 46, 46, 46, 46, 46, 46, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45}

/* The hash function is defined by linking a different hash.c file, as opposed
   to setting a #define constant. */

/* For clarity */
#define SPX_ADDR_BYTES 32

#define SPX_MAX_WOTS_W 46
#define SPX_WOTS_LEN 48

#define SPX_WOTS_S 1059

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

#define SPX_FORS_W 4
#define SPX_FORS_LOGW 2


#define SPX_FORS_MSG_BYTES (( (SPX_FORS_HEIGHT+SPX_FORS_LOGW) * SPX_FORS_TREES + 7) / 8)
#define SPX_FORS_BYTES ((SPX_FORS_HEIGHT + 1) * SPX_FORS_TREES * SPX_N)
#define SPX_FORS_PK_BYTES SPX_N

/* Resulting SPX sizes. */
#define SPX_BYTES (SPX_N + SPX_FORS_BYTES + SPX_D * SPX_WOTS_BYTES +\
                   SPX_FULL_HEIGHT * SPX_N)
#define SPX_PK_BYTES (2 * SPX_N)
#define SPX_SK_BYTES (2 * SPX_N + SPX_PK_BYTES)

/* Optionally, signing can be made non-deterministic using optrand.
   This can help counter side-channel attacks that would benefit from
   getting a large number of traces when the signer uses the same nodes. */
#define SPX_OPTRAND_BYTES 32

#include "../sm3_offsets.h"

#endif
