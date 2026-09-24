#ifndef DKE_CPA_H
#define DKE_CPA_H

#include "parameters.h"
#include <stdint.h>

void DKE_CPA_keygen_derand(uint8_t pk[DKE_PKBYTES],
                           uint8_t sk[DKE_CPA_SKABYTES],
                           const uint8_t coins[DKE_SEEDBYTES]);

void DKE_CPA_enc_derand(uint8_t ct[DKE_CPA_CTBYTES],
                        uint8_t ss[DKE_SSBYTES],
                        const uint8_t pk[DKE_PKBYTES],
                        const uint8_t coins[DKE_SEEDBYTES + DKE_N/8]);

void DKE_CPA_dec(uint8_t ss[DKE_SSBYTES],
                 const uint8_t sk[DKE_CPA_SKABYTES],
                 const uint8_t ct[DKE_CPA_CTBYTES]);

#endif
