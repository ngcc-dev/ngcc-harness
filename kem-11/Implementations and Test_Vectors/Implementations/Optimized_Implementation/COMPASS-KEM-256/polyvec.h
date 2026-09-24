#ifndef POLYVEC_H
#define POLYVEC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

typedef struct{
  poly vec[COMPASS_KEM_K];
} polyvec;

#define polyvec_compress COMPASS_KEM_NAMESPACE(polyvec_compress)
void polyvec_compress(uint8_t *r, const polyvec *a, int8_t d);
#define polyvec_decompress COMPASS_KEM_NAMESPACE(polyvec_decompress)
void polyvec_decompress(polyvec *r, const uint8_t *a, int8_t d);

#define polyvec_tobytes COMPASS_KEM_NAMESPACE(polyvec_tobytes)
void polyvec_tobytes(uint8_t r[COMPASS_KEM_POLYVECBYTES], const polyvec *a);
#define polyvec_frombytes COMPASS_KEM_NAMESPACE(polyvec_frombytes)
void polyvec_frombytes(polyvec *r, const uint8_t a[COMPASS_KEM_POLYVECBYTES]);

#define polyvec_ntt COMPASS_KEM_NAMESPACE(polyvec_ntt)
void polyvec_ntt(polyvec *r);
#define polyvec_invntt_tomont COMPASS_KEM_NAMESPACE(polyvec_invntt_tomont)
void polyvec_invntt_tomont(polyvec *r);

#define polyvec_basemul_acc_montgomery COMPASS_KEM_NAMESPACE(polyvec_basemul_acc_montgomery)
void polyvec_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b);

#define polyvec_reduce COMPASS_KEM_NAMESPACE(polyvec_reduce)
void polyvec_reduce(polyvec *r);

#define polyvec_add COMPASS_KEM_NAMESPACE(polyvec_add)
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b);

#endif
