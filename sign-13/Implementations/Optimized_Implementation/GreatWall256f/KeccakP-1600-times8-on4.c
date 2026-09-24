#include "KeccakP-1600-times4-SnP.h"

#define prefix                          KeccakP1600times8
#define PlSnP_baseParallelism           4
#define PlSnP_targetParallelism         8
#define SnP_laneLengthInBytes           8
#define SnP                             KeccakP1600times4
#define SnP_PermuteAll                  KeccakP1600times4_PermuteAll_24rounds
#define SnP_PermuteAll_12rounds         KeccakP1600times4_PermuteAll_12rounds
#define SnP_PermuteAll_6rounds          KeccakP1600times4_PermuteAll_6rounds
#define SnP_PermuteAll_4rounds          KeccakP1600times4_PermuteAll_4rounds
#define PlSnP_PermuteAll                KeccakP1600times8_PermuteAll_24rounds
#define PlSnP_PermuteAll_12rounds       KeccakP1600times8_PermuteAll_12rounds
#define PlSnP_PermuteAll_6rounds        KeccakP1600times8_PermuteAll_6rounds
#define PlSnP_PermuteAll_4rounds        KeccakP1600times8_PermuteAll_4rounds

#include "PlSnP-Fallback.inc"
