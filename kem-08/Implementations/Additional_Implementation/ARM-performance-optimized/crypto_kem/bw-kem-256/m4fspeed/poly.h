#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

/*
 * Elements of R_q = Z_q[X]/(X^n + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*coeffs[2] + ... + X^{n-1}*coeffs[n-1]
 */
typedef struct{
  int16_t coeffs[KYBER_N];
} poly;

#define poly_compress KYBER_NAMESPACE(poly_compress)
void poly_compress(uint8_t r[KYBER_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress KYBER_NAMESPACE(poly_decompress)
void poly_decompress(poly *r, const uint8_t a[KYBER_POLYCOMPRESSEDBYTES]);

#define poly_tobytes KYBER_NAMESPACE(poly_tobytes)
void poly_tobytes(uint8_t r[KYBER_POLYBYTES], const poly *a);
#define poly_frombytes KYBER_NAMESPACE(poly_frombytes)
void poly_frombytes(poly *r, const uint8_t a[KYBER_POLYBYTES]);

#define poly_frombytes_mul_16_32 KYBER_NAMESPACE(poly_frombytes_mul_16_32)
void poly_frombytes_mul_16_32(int32_t *r_tmp, const poly *b, const uint8_t a[KYBER_POLYBYTES]);
#define poly_frombytes_mul_32_32 KYBER_NAMESPACE(poly_frombytes_mul_32_32)
void poly_frombytes_mul_32_32(int32_t *r_tmp, const poly *b, const uint8_t a[KYBER_POLYBYTES]);
#define poly_frombytes_mul_32_16 KYBER_NAMESPACE(poly_frombytes_mul_32_16)
void poly_frombytes_mul_32_16(poly *r, const poly *b, const uint8_t a[KYBER_POLYBYTES],
                              const int32_t *r_tmp);

#define poly_frommsg KYBER_NAMESPACE(poly_frommsg)
void poly_frommsg(poly *r, const uint8_t msg[KYBER_INDCPA_MSGBYTES]);
#define poly_tomsg KYBER_NAMESPACE(poly_tomsg)
void poly_tomsg(uint8_t msg[KYBER_INDCPA_MSGBYTES], const poly *r);

#define poly_getnoise_eta1 KYBER_NAMESPACE(poly_getnoise_eta1)
void poly_getnoise_eta1(poly *r, const uint8_t seed[KYBER_SYMBYTES], uint8_t nonce);

#define poly_getnoise_eta2 KYBER_NAMESPACE(poly_getnoise_eta2)
void poly_getnoise_eta2(poly *r, const uint8_t seed[KYBER_SYMBYTES], uint8_t nonce);

#define poly_ntt KYBER_NAMESPACE(poly_ntt)
void poly_ntt(poly *r);
#define poly_invntt KYBER_NAMESPACE(poly_invntt)
void poly_invntt(poly *r);

#define poly_basemul_opt_16_32 KYBER_NAMESPACE(poly_basemul_opt_16_32)
void poly_basemul_opt_16_32(int32_t *r_tmp, const poly *a, const poly *b, const poly *a_prime);
#define poly_basemul_acc_opt_32_32 KYBER_NAMESPACE(poly_basemul_acc_opt_32_32)
void poly_basemul_acc_opt_32_32(int32_t *r, const poly *a, const poly *b, const poly *a_prime);
#define poly_basemul_acc_opt_32_16 KYBER_NAMESPACE(poly_basemul_acc_opt_32_16)
void poly_basemul_acc_opt_32_16(poly *r, const poly *a, const poly *b, const poly *a_prime, const int32_t *r_tmp);
#define poly_reduce KYBER_NAMESPACE(poly_reduce)
void poly_reduce(poly *r);

#define poly_add KYBER_NAMESPACE(poly_add)
void poly_add(poly *r, const poly *a, const poly *b);
#define poly_sub KYBER_NAMESPACE(poly_sub)
void poly_sub(poly *r, const poly *a, const poly *b);

#endif
