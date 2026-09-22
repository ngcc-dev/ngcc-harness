#ifndef CCAKEM_H
#define CCAKEM_H

#include "cpapke.h"

#ifdef __cplusplus
extern "C" {
#endif

inline void CCAKEM_Init() {
    CPAPKE_Init();
}

void CCAKEM_KeyGen(uint8_t sk[RLWE_CCA_SK_LEN], uint8_t pk[RLWE_CCA_PK_LEN]);

void CCAKEM_Encaps(uint8_t ct[RLWE_CCA_CT_LEN], uint8_t key[RLWE_KEY_LEN], const uint8_t pk[RLWE_CCA_PK_LEN]);

int CCAKEM_Decaps(uint8_t key[RLWE_KEY_LEN],
                  const uint8_t sk[RLWE_CCA_SK_LEN],
                  const uint8_t ct[RLWE_CCA_CT_LEN]);

#ifdef __cplusplus
}
#endif

#endif  // CCAKEM_H
