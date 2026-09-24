#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

/*
 * Elements of R_q = Z_q[X]/(X^n + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*coeffs[2] + ... + X^{n-1}*coeffs[n-1]
 */
#include "align.h" // AVX2 alignment utilities

// Keep struct layout as-is to ensure coeffs alignment
typedef struct {
  int16_t coeffs[COMPASS_KEM_N] __attribute__((aligned(32)));
} poly;
// typedef struct{
//   int16_t coeffs[COMPASS_KEM_N];
// } poly;

#define poly_compress COMPASS_KEM_NAMESPACE(poly_compress)
void poly_compress(uint8_t *r, const poly *a, int8_t d);
#define poly_decompress COMPASS_KEM_NAMESPACE(poly_decompress)
void poly_decompress(poly *r, const uint8_t *a, int8_t d);

#define poly_tobytes COMPASS_KEM_NAMESPACE(poly_tobytes)
void poly_tobytes(uint8_t r[COMPASS_KEM_POLYBYTES], const poly *a);
#define poly_frombytes COMPASS_KEM_NAMESPACE(poly_frombytes)
void poly_frombytes(poly *r, const uint8_t a[COMPASS_KEM_POLYBYTES]);

#define poly_frommsg COMPASS_KEM_NAMESPACE(poly_frommsg)
void poly_frommsg(poly *r, const uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES]);
#define poly_tomsg COMPASS_KEM_NAMESPACE(poly_tomsg)
void poly_tomsg(uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES], const poly *r);

#define poly_getnoise_eta1 COMPASS_KEM_NAMESPACE(poly_getnoise_eta1)
void poly_getnoise_eta1(poly *r, const uint8_t seed[COMPASS_KEM_SYMBYTES], uint8_t nonce);

#define poly_getnoise_eta2 COMPASS_KEM_NAMESPACE(poly_getnoise_eta2)
void poly_getnoise_eta2(poly *r, const uint8_t seed[COMPASS_KEM_SYMBYTES], uint8_t nonce);

#define poly_ntt COMPASS_KEM_NAMESPACE(poly_ntt)
void poly_ntt(poly *r);
#define poly_invntt_tomont COMPASS_KEM_NAMESPACE(poly_invntt_tomont)
void poly_invntt_tomont(poly *r);
#define poly_basemul_montgomery COMPASS_KEM_NAMESPACE(poly_basemul_montgomery)
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b);
#define poly_tomont COMPASS_KEM_NAMESPACE(poly_tomont)
void poly_tomont(poly *r);

#define poly_reduce COMPASS_KEM_NAMESPACE(poly_reduce)
void poly_reduce(poly *r);

#define poly_add COMPASS_KEM_NAMESPACE(poly_add)
void poly_add(poly *r, const poly *a, const poly *b);
#define poly_sub COMPASS_KEM_NAMESPACE(poly_sub)
void poly_sub(poly *r, const poly *a, const poly *b);

#endif
