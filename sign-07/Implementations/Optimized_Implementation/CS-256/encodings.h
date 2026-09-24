#ifndef PACKING_H
#define PACKING_H

#include "conversion.h"
#include "bits-hints.h"

void pkEncode(uint8_t pk[PUBLICKEYBYTES],
              const uint8_t rho[SEEDBYTES],
              const poly* t1);

void pkDecode(uint8_t rho[SEEDBYTES],
              poly* t1,
              const uint8_t pk[PUBLICKEYBYTES]);

void skEncode(uint8_t sk[SECRETKEYBYTES],
              const uint8_t rho[SEEDBYTES],
              const uint8_t K[SEEDBYTES],
              const uint8_t tr[HBYTES],
              const poly* s0,
              const poly* e,
              const poly* t0,
              const poly* t1);

void skDecode(uint8_t rho[SEEDBYTES],
              uint8_t K[SEEDBYTES],
              uint8_t tr[HBYTES],
              poly* s,
              poly* t0,
              poly* t1,
              const uint8_t sk[SECRETKEYBYTES]);

int sigEncode(uint8_t sig[SIGNATUREBYTES],
              const uint8_t* cwave,
              const poly* z);

int sigDecode(uint8_t* cwave,
              poly* z,
              const uint8_t sig[SIGNATUREBYTES]);

void w1Encode(uint8_t w1wave[k * POLYW_PACKEDBYTES], const poly* w1);


#endif
