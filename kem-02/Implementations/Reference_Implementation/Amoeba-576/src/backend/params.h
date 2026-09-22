#ifndef PARAM_H
#define PARAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RLWE_M (int32_t)864
#define RLWE_N (int32_t)288
#define RLWE_Q (int32_t)3457
#define RLWE_LogQ 12

#ifndef SECURITY_LEVEL
#define SECURITY_LEVEL 128
#endif

#if SECURITY_LEVEL == 128
#define RLWE_K 2
#define RLWE_ETA1 2
#define RLWE_ETA2 2
#define RLWE_D0 10
#define RLWE_D1 10
#define RLWE_D2 5
#endif

#if SECURITY_LEVEL == 192
#define RLWE_K 3
#define RLWE_ETA1 1
#define RLWE_ETA2 2
#define RLWE_D0 10
#define RLWE_D1 11
#define RLWE_D2 6
#endif

#if SECURITY_LEVEL == 256
#define RLWE_K 4
#define RLWE_ETA1 1
#define RLWE_ETA2 1
#define RLWE_D0 11
#define RLWE_D1 10
#define RLWE_D2 6
#endif

#if SECURITY_LEVEL == 384
#define RLWE_K 6
#define RLWE_ETA1 1
#define RLWE_ETA2 1
#define RLWE_D0 11
#define RLWE_D1 11
#define RLWE_D2 6
#endif

#if SECURITY_LEVEL == 512
#define RLWE_K 8
#define RLWE_ETA1 1
#define RLWE_ETA2 1
#define RLWE_D0 12
#define RLWE_D1 12
#define RLWE_D2 7
#endif

#define RLWE_ECC_R 8
#define RLWE_ECC_N 523
#define RLWE_ECC_L 512

#define RLWE_MSG_LEN 64
#define RLWE_KEY_LEN 64
#define RLWE_SEED_LEN 64

#define BIT_TO_BYTE(x) (((x) + 7) / 8)

#define RLWE_CPA_SK_LEN BIT_TO_BYTE(RLWE_LogQ * RLWE_K * RLWE_N)
#define RLWE_CPA_PK_LEN (RLWE_SEED_LEN + BIT_TO_BYTE(RLWE_D0 * RLWE_K * RLWE_N))
#define RLWE_CPA_CT_LEN (BIT_TO_BYTE(RLWE_D1 * RLWE_K * RLWE_N) + BIT_TO_BYTE(RLWE_D2 * RLWE_ECC_N))

#define RLWE_CCA_SK_LEN RLWE_CPA_SK_LEN + RLWE_CPA_PK_LEN + RLWE_SEED_LEN
#define RLWE_CCA_PK_LEN RLWE_CPA_PK_LEN
#define RLWE_CCA_CT_LEN RLWE_CPA_CT_LEN

#ifdef __cplusplus
}
#endif

#endif  // PARAM_H