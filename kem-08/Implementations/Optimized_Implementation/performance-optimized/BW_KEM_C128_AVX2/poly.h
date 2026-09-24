#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"
#include "align.h"

/*
 * Elements of R_q = Z_q[X]/(X^n + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*coeffs[2] + ... + X^{n-1}*coeffs[n-1]
 */
typedef struct __attribute__((aligned(32))) {
  int16_t coeffs[BWKEM128_N];
} poly;

#define poly_compress BWKEM128_NAMESPACE(poly_compress)
void poly_compress(uint8_t r[BWKEM128_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress BWKEM128_NAMESPACE(poly_decompress)
void poly_decompress(poly *r, const uint8_t a[BWKEM128_POLYCOMPRESSEDBYTES]);

#define poly_tobytes BWKEM128_NAMESPACE(poly_tobytes)
void poly_tobytes(uint8_t r[BWKEM128_POLYBYTES], const poly *a);
#define poly_frombytes BWKEM128_NAMESPACE(poly_frombytes)
void poly_frombytes(poly *r, const uint8_t a[BWKEM128_POLYBYTES]);

#define poly_frommsg BWKEM128_NAMESPACE(poly_frommsg)
void poly_frommsg(poly *r, const uint8_t msg[BWKEM128_INDCPA_MSGBYTES]);
#define poly_tomsg BWKEM128_NAMESPACE(poly_tomsg)
void poly_tomsg(uint8_t msg[BWKEM128_INDCPA_MSGBYTES], const poly *r);

#define poly_getnoise_eta1 BWKEM128_NAMESPACE(poly_getnoise_eta1)
void poly_getnoise_eta1(poly *r, const uint8_t seed[BWKEM128_SYMBYTES], uint8_t nonce);

#define poly_getnoise_eta1_4x BWKEM128_NAMESPACE(poly_getnoise_eta1_4x)
void poly_getnoise_eta1_4x(poly *r0,
                           poly *r1,
                           poly *r2,
                           poly *r3,
                           const uint8_t seed[BWKEM128_SYMBYTES],
                           uint8_t nonce0);

#define poly_getnoise_eta2 BWKEM128_NAMESPACE(poly_getnoise_eta2)
void poly_getnoise_eta2(poly *r, const uint8_t seed[BWKEM128_SYMBYTES], uint8_t nonce);

#define poly_ntt BWKEM128_NAMESPACE(poly_ntt)
void poly_ntt(poly *r);
#define poly_invntt_tomont BWKEM128_NAMESPACE(poly_invntt_tomont)
void poly_invntt_tomont(poly *r);
#define poly_basemul_montgomery BWKEM128_NAMESPACE(poly_basemul_montgomery)
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b);
#define poly_tomont BWKEM128_NAMESPACE(poly_tomont)
void poly_tomont(poly *r);

#define poly_reduce BWKEM128_NAMESPACE(poly_reduce)
void poly_reduce(poly *r);

#define poly_add BWKEM128_NAMESPACE(poly_add)
void poly_add(poly *r, const poly *a, const poly *b);
#define poly_sub BWKEM128_NAMESPACE(poly_sub)
void poly_sub(poly *r, const poly *a, const poly *b);

#endif
