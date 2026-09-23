/*
 * AFS-p-S6[1600,nr] reference permutation family for AFS-TrEDM-S6.
 *
 * Default split profile follows the AFS-LMDS-1600-S6 design report:
 *   AFS-TrEDM-512  :  6 + 6  = 12 rounds
 *   AFS-TrEDM-768  : 10 + 10 = 20 rounds
 *   AFS-TrEDM-1024 : 12 + 12 = 24 rounds
 */

#ifndef AFS_P1600_H
#define AFS_P1600_H

#include <stdint.h>

#ifndef DIGEST_BIT_LENGTH
# include "CryptHash_AlgorithmInstance.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AFS_P1600_LANES 25U
#define AFS_P1600_MAX_ROUNDS 24U

#if defined(AFS_TREDM_TIERED_ROUNDS) && !defined(AFS_TREDM_PERFORMANCE_PROFILE)
# error "AFS_TREDM_TIERED_ROUNDS is a legacy alias and requires explicit AFS_TREDM_PERFORMANCE_PROFILE"
#endif

#if defined(AFS_TREDM_TIERED_SPLIT_ROUNDS)
# define AFS_P1600_SPLIT_ROUNDS AFS_TREDM_TIERED_SPLIT_ROUNDS
#elif defined(DIGEST_BIT_LENGTH)
# if DIGEST_BIT_LENGTH == 512
#  define AFS_P1600_SPLIT_ROUNDS 6U
# elif DIGEST_BIT_LENGTH == 768
#  define AFS_P1600_SPLIT_ROUNDS 10U
# elif DIGEST_BIT_LENGTH == 1024
#  define AFS_P1600_SPLIT_ROUNDS 12U
# else
#  define AFS_P1600_SPLIT_ROUNDS 12U
# endif
#else
# define AFS_P1600_SPLIT_ROUNDS 12U
#endif

#if (AFS_P1600_SPLIT_ROUNDS < 1U) || (AFS_P1600_SPLIT_ROUNDS > 12U)
# error "AFS_P1600_SPLIT_ROUNDS must be in [1, 12]"
#endif

#define AFS_P1600_EFFECTIVE_ROUNDS (2U * AFS_P1600_SPLIT_ROUNDS)
#define AFS_P1600_ROUNDS AFS_P1600_EFFECTIVE_ROUNDS

#if AFS_P1600_EFFECTIVE_ROUNDS > AFS_P1600_MAX_ROUNDS
# error "AFS_P1600_EFFECTIVE_ROUNDS exceeds AFS_P1600_MAX_ROUNDS"
#endif

#define AFS_TREDM_PROFILE_NAME "S6-tiered"
#define AFS_TREDM_LINEAR_LAYER_NAME "AFS-LMDS-1600-S6"

void afs_p1600_permute(uint64_t A[AFS_P1600_LANES], unsigned first_round, unsigned nr);
void afs_p1600_g(uint64_t A[AFS_P1600_LANES]);
void afs_p1600_h(uint64_t A[AFS_P1600_LANES]);

#ifdef __cplusplus
}
#endif

#endif /* AFS_P1600_H */
