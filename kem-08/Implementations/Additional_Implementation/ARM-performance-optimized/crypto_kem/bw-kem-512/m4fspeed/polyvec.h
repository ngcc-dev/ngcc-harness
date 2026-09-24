#ifndef POLYVEC_H
#define POLYVEC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

typedef struct{
  poly vec[KYBER_K];
} polyvec;

#define polyvec_compress KYBER_NAMESPACE(polyvec_compress)
void polyvec_compress(uint8_t r[KYBER_POLYVECCOMPRESSEDBYTES], const polyvec *a);
#define polyvec_decompress KYBER_NAMESPACE(polyvec_decompress)
void polyvec_decompress(polyvec *r, const uint8_t a[KYBER_POLYVECCOMPRESSEDBYTES]);

#define polyvec_tobytes KYBER_NAMESPACE(polyvec_tobytes)
void polyvec_tobytes(uint8_t r[KYBER_POLYVECBYTES], const polyvec *a);
#define polyvec_frombytes KYBER_NAMESPACE(polyvec_frombytes)
void polyvec_frombytes(polyvec *r, const uint8_t a[KYBER_POLYVECBYTES]);

#define polyvec_ntt KYBER_NAMESPACE(polyvec_ntt)
void polyvec_ntt(polyvec *r);
#define polyvec_invntt KYBER_NAMESPACE(polyvec_invntt)
void polyvec_invntt(polyvec *r);

#define polyvec_basemul_acc KYBER_NAMESPACE(polyvec_basemul_acc)
void polyvec_basemul_acc(poly *r, const polyvec *a, const polyvec *b);

#define polyvec_cache_prime KYBER_NAMESPACE(polyvec_cache_prime)
void polyvec_cache_prime(polyvec *b_prime, const polyvec *b);
#define polyvec_basemul_acc_cached KYBER_NAMESPACE(polyvec_basemul_acc_cached)
void polyvec_basemul_acc_cached(poly *r, const polyvec *a, const polyvec *b,
                                const polyvec *a_prime);

#define polyvec_reduce KYBER_NAMESPACE(polyvec_reduce)
void polyvec_reduce(polyvec *r);

#define polyvec_add KYBER_NAMESPACE(polyvec_add)
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b);

#endif
