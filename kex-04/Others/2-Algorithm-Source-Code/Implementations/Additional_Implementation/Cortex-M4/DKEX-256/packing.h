#ifndef DKE_PACKING_H
#define DKE_PACKING_H

#include "parameters.h"
#include "polyvec.h"
#include <stdint.h>

void DKE_packpk(uint8_t bytes[DKE_PKBYTES],
                const polyvec *pk,
                const uint8_t seed[DKE_SEEDBYTES]);

void DKE_unpackpk(polyvec *pk,
                  uint8_t seed[DKE_SEEDBYTES],
                  const uint8_t bytes[DKE_PKBYTES]);

void DKE_CPA_packsk(uint8_t bytes[DKE_CPA_SKABYTES], polyvec *sk);
void DKE_CPA_unpacksk(polyvec *sk, const uint8_t bytes[DKE_CPA_SKABYTES]);

void DKE_CPA_packciphertext(uint8_t bytes[DKE_CPA_CTBYTES],
                            polyvec *pb,
                            uint8_t sig[DKE_SIGNALBYTES]);

void DKE_CPA_unpackciphertext(polyvec *pb,
                              uint8_t sig[DKE_SIGNALBYTES],
                              const uint8_t bytes[DKE_CPA_CTBYTES]);

#endif
