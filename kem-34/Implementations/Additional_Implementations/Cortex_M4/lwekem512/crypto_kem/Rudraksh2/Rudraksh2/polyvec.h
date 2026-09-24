#ifndef POLYVEC_H
#define POLYVEC_H

#include "params.h"
#include "poly.h"
#include <stdint.h>

typedef struct
{
	poly vec[KEM_L];
} polyvec;

#define polyvec_tobytes KEM_NAMESPACE (polyvec_tobytes)
void polyvec_tobytes (uint8_t r[KEM_POLYVECBYTES], const polyvec *a);
#define polyvec_frombytes KEM_NAMESPACE (polyvec_frombytes)
void polyvec_frombytes (polyvec *r, const uint8_t a[KEM_POLYVECBYTES]);

#define polyvec_ntt KEM_NAMESPACE (polyvec_ntt)
void polyvec_ntt (polyvec *r);
#define polyvec_invntt_tomont KEM_NAMESPACE (polyvec_invntt_tomont)
void polyvec_invntt_tomont (polyvec *r);

#define polyvec_basemul_acc_montgomery                                        \
	KEM_NAMESPACE (polyvec_basemul_acc_montgomery)
void polyvec_basemul_acc_montgomery (poly *r, const polyvec *a,
																		 const polyvec *b);

#define polyvec_reduce KEM_NAMESPACE (polyvec_reduce)
void polyvec_reduce (polyvec *r);

#define polyvec_add KEM_NAMESPACE (polyvec_add)
void polyvec_add (polyvec *r, const polyvec *a, const polyvec *b);

#endif
