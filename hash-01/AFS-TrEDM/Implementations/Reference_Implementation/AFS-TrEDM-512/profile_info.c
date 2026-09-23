#include <stdio.h>

#include "CryptHash_AlgorithmInstance.h"
#include "afs_p1600.h"

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(void)
{
    printf("instance=%s\n", ALGORITHM_INSTANCE);
    printf("digest_bits=%d\n", DIGEST_BIT_LENGTH);
    printf("profile=%s\n", AFS_TREDM_PROFILE_NAME);
    printf("split_rounds=%u\n", (unsigned)AFS_P1600_SPLIT_ROUNDS);
    printf("effective_rounds=%u\n", (unsigned)AFS_P1600_EFFECTIVE_ROUNDS);
#ifdef AFS_TREDM_LINEAR_LAYER_NAME
    printf("linear_layer=%s\n", AFS_TREDM_LINEAR_LAYER_NAME);
#endif
    return 0;
}
