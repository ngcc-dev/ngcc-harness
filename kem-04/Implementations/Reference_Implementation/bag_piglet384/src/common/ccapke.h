#ifndef CCAPKE_H
#define CCAPKE_H

#include <stdint.h>

#include "parameters.h"

void ccapke_keygen(uint8_t pk[CCAPKE_PK_SIZE],
                   uint8_t sk[CCAPKE_SK_SIZE],
                   const uint8_t coin[PARAMS_SEED_SIZE]);
void ccapke_encrypt(uint8_t **ct, int *ctlen,
                    const uint8_t pk[CCAPKE_PK_SIZE],
                    const uint8_t *msg, int msglen,
                    const uint8_t coin[PARAMS_RAND_SIZE]);
void ccapke_decrypt(uint8_t **msg, int *msglen,
                    const uint8_t sk[CCAPKE_SK_SIZE],
                    const uint8_t *ct, int ctlen);

#endif
