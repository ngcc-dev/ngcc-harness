#ifndef DKE_UTILS_H
#define DKE_UTILS_H

#include "parameters.h"
#include "poly.h"
#include <stdint.h>

void DKE_signal(uint8_t sig[DKE_SIGNALBYTES],
                const poly *k,
                const uint8_t coins[DKE_N/8]);

void DKE_derive_ss(uint8_t ss[DKE_SSBYTES],
                   poly *k,
                   const uint8_t sig[DKE_SIGNALBYTES]);

#endif
