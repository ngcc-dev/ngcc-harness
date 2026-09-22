#ifndef CPAPKE_H
#define CPAPKE_H

#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

void CPAPKE_Init();

void CPAPKE_KeyGen(uint8_t sk[RLWE_CPA_SK_LEN], uint8_t pk[RLWE_CPA_PK_LEN]);

void CPAPKE_Encrypt(uint8_t ct[RLWE_CPA_CT_LEN], const uint8_t pk[RLWE_CPA_PK_LEN], const uint8_t msg[RLWE_MSG_LEN], const uint8_t coin[RLWE_SEED_LEN]);

int CPAPKE_Decrypt(uint8_t msg[RLWE_MSG_LEN],
                   const uint8_t sk[RLWE_CPA_SK_LEN],
                   const uint8_t ct[RLWE_CPA_CT_LEN]);

#ifdef __cplusplus
}
#endif

#endif  // CPAPKE_H