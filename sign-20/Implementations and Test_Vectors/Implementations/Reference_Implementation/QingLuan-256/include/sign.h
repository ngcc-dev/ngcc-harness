/*
 * QingLuan Digital Signature Scheme
 * sign.h - Signature byte layout (CROSS-RSDP, fast-style direct Path/Proof)
 *
 *   [ Salt            | PARAM_SALT_BYTES ]
 *   [ digest_cmt      | PARAM_HASH_BYTES ]
 *   [ digest_chall2   | PARAM_HASH_BYTES ]
 *   [ Path  : w seeds | PARAM_W * PARAM_SEED_BYTES ]      (rounds chall2[i]=1)
 *   [ Proof : w cmt0  | PARAM_W * PARAM_HASH_BYTES ]      (rounds chall2[i]=1)
 *   [ resp  : (t-w) * { pack(y) | pack(v) | cmt1 } ]      (rounds chall2[i]=0)
 *
 * Total = QINGLUAN_SIG_BYTES (fixed). chall2 has fixed weight w, so the split
 * w / (t-w) is constant and the layout is unambiguous once chall2 is derived.
 */

#ifndef QINGLUAN_SIGN_H
#define QINGLUAN_SIGN_H

#include "params.h"
#include <stddef.h>
#include <stdint.h>

#define SIG_OFF_SALT          0
#define SIG_OFF_DIGEST_CMT    (SIG_OFF_SALT + PARAM_SALT_BYTES)
#define SIG_OFF_DIGEST_CHALL2 (SIG_OFF_DIGEST_CMT + PARAM_HASH_BYTES)
#define SIG_OFF_PATH          (SIG_OFF_DIGEST_CHALL2 + PARAM_HASH_BYTES)
#define SIG_OFF_PROOF         (SIG_OFF_PATH + PARAM_W * PARAM_SEED_BYTES)
#define SIG_OFF_RESP          (SIG_OFF_PROOF + PARAM_W * PARAM_HASH_BYTES)
/* SIG_RESP0_BYTES and QINGLUAN_SIG_BYTES are defined in params.h */

#endif /* QINGLUAN_SIGN_H */
