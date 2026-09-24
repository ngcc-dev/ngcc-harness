#ifndef DKE_POLYVEC_H
#define DKE_POLYVEC_H

#include "parameters.h"
#include "poly.h"
#include <stdint.h>

typedef struct {
    poly vec[DKE_K];
} polyvec;

void DKE_polyvec_reduce(polyvec *v);
void DKE_polyvec_add(polyvec *res, const polyvec *a, const polyvec *b);
void DKE_polyvec_sub(polyvec *res, const polyvec *a, const polyvec *b);
void DKE_polyvec_scale2(polyvec *v);

void DKE_polyvec_ntt(polyvec *v);
void DKE_polyvec_invntt_tomont(polyvec *v);
void DKE_polyvec_basemul_acc_montgomery(poly *res, const polyvec *a, const polyvec *b);

void DKE_polyvec_tobytes(uint8_t bytes[DKE_POLYVECBYTES], const polyvec *v);
void DKE_polyvec_frombytes(polyvec *v, const uint8_t bytes[DKE_POLYVECBYTES]);

void DKE_polyvec_compressB(uint8_t bytes[DKE_PBCOMPRESSEDBYTES], const polyvec *v);
void DKE_polyvec_decompressB(polyvec *v, const uint8_t bytes[DKE_PBCOMPRESSEDBYTES]);

#endif
