/*
 * AFS-TrEDM reference implementation.
 *
 * This file implements the selected 64-bit AFS S-box instance:
 *   structural encoding A8 = 11000011
 *   rotation sequence K8 = [17, 24, 1, 1, 16, 31, 24, 0]
 *
 * The code is an ISO C99 reference implementation written for the AFS-TrEDM
 * submission package.  It follows the operation sequence of the AFS-64 t5/k2
 * instance described in the AFS paper and in the uploaded AFS reference
 * material, but is organized independently for this hash submission.
 */

#ifndef AFS_SBOX64_H
#define AFS_SBOX64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t afs64_t5_k2(uint64_t in, uint32_t c);

#ifdef __cplusplus
}
#endif

#endif /* AFS_SBOX64_H */
