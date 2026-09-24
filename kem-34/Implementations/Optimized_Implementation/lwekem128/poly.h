#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"
#include "consts.h"

/*
 * Elements of R_q = Z_q[X]/(X^n + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*coeffs[2] + ... + X^{n-1}*coeffs[n-1]
 */
typedef struct{
  __attribute__((aligned(32)))
  uint16_t coeffs[KEM_N];
} poly;

#define poly_compress KEM_NAMESPACE(poly_compress)
void poly_compress(uint8_t r[KEM_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress KEM_NAMESPACE(poly_decompress)
void poly_decompress(poly *r, const uint8_t a[KEM_POLYCOMPRESSEDBYTES]);

#define poly_tobytes KEM_NAMESPACE(poly_tobytes)
void poly_tobytes(uint8_t r[KEM_POLYBYTES], const poly *a);
#define poly_frombytes KEM_NAMESPACE(poly_frombytes)
void poly_frombytes(poly *r, const uint8_t a[KEM_POLYBYTES]);

#define poly_frommsg KEM_NAMESPACE(poly_frommsg)
void poly_frommsg(poly *r, const uint8_t msg[KEM_INDCPA_MSGBYTES]);
#define poly_tomsg KEM_NAMESPACE(poly_tomsg)
void poly_tomsg(uint8_t msg[KEM_INDCPA_MSGBYTES], const poly *r);

#define poly_getnoise_eta KEM_NAMESPACE(poly_getnoise_eta)
void poly_getnoise_eta(poly *r, const uint8_t seed[KEM_SYMBYTES], uint8_t nonce);

#define poly_ntt KEM_NAMESPACE(poly_ntt)
void poly_ntt(poly *r);
#define poly_invntt_tomont KEM_NAMESPACE(poly_invntt_tomont)
void poly_invntt_tomont(poly *r);
#define poly_basemul_montgomery KEM_NAMESPACE(poly_basemul_montgomery)
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b);

#define poly_tomont KEM_NAMESPACE(poly_tomont)
void poly_tomont(poly *r);

#define poly_reduce KEM_NAMESPACE(poly_reduce)
void poly_reduce(poly *r);

#define poly_add KEM_NAMESPACE(poly_add)
void poly_add(poly *r, const poly *a, const poly *b);
#define poly_sub KEM_NAMESPACE(poly_sub)
void poly_sub(poly *r, const poly *a, const poly *b);

#define poly_freeze KEM_NAMESPACE(poly_freeze)
void poly_freeze(poly *r);
// void poly_freeze(poly *r);

void poly_decompress_file(poly *r, const uint8_t a[KEM_POLYCOMPRESSEDBYTES]);


void basemul_mont_avx(poly *r, const poly *a, const poly *b, const uint16_t *qdata);

void polyadd_avx(poly *r, const poly *a, const poly *b, const uint16_t *qdata);

void polysub_avx(poly *r, const poly *a, const poly *b, const uint16_t *qdata);

void polyreduce_avx(poly *r, const uint16_t *qdata);

#endif
