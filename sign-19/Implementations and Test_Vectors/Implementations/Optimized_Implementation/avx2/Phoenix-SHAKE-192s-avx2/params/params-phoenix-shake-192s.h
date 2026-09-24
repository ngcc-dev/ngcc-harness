/* Phoenix-SHAKE-192s parameter definitions */

#ifndef PH_PARAMS_H
#define PH_PARAMS_H

#define SPX_NAMESPACE(s) PH_##s

/* Hash output length in bytes. */
#define SPX_N 24
/* Height of the hypertree. */
#define SPX_FULL_HEIGHT 63
/* Number of subtree layer. */
#define SPX_D 9
/* TFORS tree dimensions. */
#define SPX_TFORS_A 12
#define SPX_TFORS_K_PRIME 32
#define SPX_TFORS_LOG_K_PRIME 5
#define SPX_TFORS_K 19
#define SPX_TFORS_HEIGHT (SPX_TFORS_LOG_K_PRIME + SPX_TFORS_A)
#define SPX_TFORS_T (1 << SPX_TFORS_A)
/* Winternitz parameter. */
#define SPX_GWOTS_W1 64
#define SPX_GWOTS_LOGW1 6
#define SPX_GWOTS_W2 128
#define SPX_GWOTS_LOGW2 7

/* Address length in bytes. */
#define SPX_ADDR_BYTES 32

/* GWOTS chain lengths. */
#define SPX_GWOTS_W1_LEN 11
#define SPX_GWOTS_W2_LEN 18
#define SPX_GWOTS_LEN1 (SPX_GWOTS_W1_LEN + SPX_GWOTS_W2_LEN)

/* Checksum chain length is fixed. */
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
#define SPX_TFORS_SIG_MAX 5274
#define SPX_TFORS_PK_BYTES SPX_N

/* Counter size for GWOTS. */
#define COUNTER_SIZE 4

/* GWOTS checksum parameter. */
#define SPX_GWOTS_CHECKSUM_W 32
#define SPX_GWOTS_CHECKSUM_LOGW 5

/* Expected average sum of message chains. */
#define GWOTS_EXPECTED_SUM (                   \
    ((SPX_GWOTS_W1_LEN * (SPX_GWOTS_W1 - 1)) +  \
     (SPX_GWOTS_W2_LEN * (SPX_GWOTS_W2 - 1))) / \
    2)

/* Acceptable sum range for counter search. */
#define GWOTS_SUM_BASE (GWOTS_EXPECTED_SUM - (SPX_GWOTS_CHECKSUM_W / 2))
#define GWOTS_SUM_RANGE ((SPX_GWOTS_CHECKSUM_W) - 1)

/* Resulting Phoenix signature sizes. */
#define SPX_BYTES (SPX_N + COUNTER_SIZE + SPX_TFORS_SIG_MAX + SPX_D * (SPX_GWOTS_BYTES + COUNTER_SIZE) + \
                   SPX_FULL_HEIGHT * SPX_N)
#define SPX_PK_BYTES (2 * SPX_N)
#define SPX_SK_BYTES (2 * SPX_N + SPX_PK_BYTES)

#include "../shake_offsets.h"

#endif /* PH_PARAMS_H */