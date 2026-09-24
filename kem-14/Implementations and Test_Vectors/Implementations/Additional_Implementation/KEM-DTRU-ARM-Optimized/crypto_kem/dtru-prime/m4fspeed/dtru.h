#ifndef DTRU_H
#define DTRU_H

#include "params.h"

void pke_keygen(unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                       unsigned char sk[DTRU_PKE_SECRETKEYBYTES],
                       const unsigned char coins[DTRU_COINBYTES_KEYGEN]);

void pke_enc(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC]);

void pke_dec(unsigned char m[DTRU_MSGBYTES],
                    const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char sk[DTRU_PKE_SECRETKEYBYTES]);

#endif
