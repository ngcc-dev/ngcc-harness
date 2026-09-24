#ifndef DKE_CCA_H
#define DKE_CCA_H

#include "parameters.h"
#include <stdint.h>

void DKE_CCA_keygen_derand(uint8_t pk[DKE_PKBYTES],
                           uint8_t sk[DKE_SKBYTES],
                           const uint8_t coins[DKE_SEEDBYTES + DKE_SSBYTES]);

void DKE_CCA_enc_derand(uint8_t ct[DKE_CTBYTES],
                        uint8_t ss[DKE_SSBYTES],
                        const uint8_t pk[DKE_PKBYTES],
                        const uint8_t coins[DKE_SEEDBYTES]);

void DKE_CCA_dec(uint8_t ss[DKE_SSBYTES],
                 const uint8_t sk[DKE_SKBYTES],
                 const uint8_t ct[DKE_CTBYTES]);

#endif
