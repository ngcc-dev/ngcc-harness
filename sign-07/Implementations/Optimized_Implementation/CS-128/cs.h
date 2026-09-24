#ifndef SIGN_H
#define SIGN_H

#include "sampling.h"
#include "encodings.h"


void CS_KeyGen(uint8_t pk[PUBLICKEYBYTES],
               uint8_t sk[SECRETKEYBYTES]);

int CS_Sign(uint8_t sig[SIGNATUREBYTES],
            const uint8_t sk[SECRETKEYBYTES],
            const uint8_t* M,
            int len);

bool CS_Verify(const uint8_t pk[PUBLICKEYBYTES],
               const uint8_t* M,
               int len,
               const uint8_t sig[SIGNATUREBYTES]);


#endif
