#ifndef CCAKEM_H
#define CCAKEM_H

#include <stdint.h>

#include "parameters.h"

void ccakem_keygen(uint8_t pk[CCAKEM_PK_SIZE],
                   uint8_t sk[CCAKEM_SK_SIZE],
                   const unsigned char *seed);
void ccakem_encaps(uint8_t key[CCAKEM_KEY_SIZE],
                   uint8_t ct[CCAKEM_CT_SIZE],
                   const uint8_t pk[CCAKEM_PK_SIZE],
                   const unsigned char *seed);
void ccakem_decaps(uint8_t key[CCAKEM_KEY_SIZE],
                   const uint8_t sk[CCAKEM_SK_SIZE],
                   const uint8_t ct[CCAKEM_CT_SIZE]);

#endif
