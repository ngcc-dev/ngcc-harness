#ifndef SIGN_POLYFIX_H
#define SIGN_POLYFIX_H

#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "reduce.h"
#include <stdint.h>

typedef struct {
    int32_t coeffs[N];
} polyfix;

#define polyfix_norm2 DARTS_NAMESPACE(polyfix_norm2)
double polyfix_norm2(const polyfix *a);

#define polyfix_round DARTS_NAMESPACE(polyfix_round)
void polyfix_round(poly *a, const polyfix *b);

#define polyfix_add DARTS_NAMESPACE(polyfix_add)
void polyfix_add(polyfix *c, const polyfix *a, const poly *b);

#define polyfixfix_sub DARTS_NAMESPACE(polyfixfix_sub)
void polyfixfix_sub(polyfix *c, const polyfix *a, const polyfix *b);

typedef struct {
    polyfix vec[K];
} polyfixveck;

#define polyfixveck_add DARTS_NAMESPACE(polyfixveck_add)
void polyfixveck_add(polyfixveck *w, const polyfixveck *u, const polyveck *v);

#define polyfixfixveck_sub DARTS_NAMESPACE(polyfixfixveck_sub)
void polyfixfixveck_sub(polyfixveck *w, const polyfixveck *u,
                        const polyfixveck *v);
#define polyfixveck_double DARTS_NAMESPACE(polyfixveck_double)
void polyfixveck_double(polyfixveck *b, const polyfixveck *a);

#define polyfixveck_round DARTS_NAMESPACE(polyfixveck_round)
void polyfixveck_round(polyveck *a, const polyfixveck *b);

typedef struct {
    polyfix vec[L];
} polyfixvecl;

#define polyfixvecl_add DARTS_NAMESPACE(polyfixvecl_add)
void polyfixvecl_add(polyfixvecl *w, const polyfixvecl *u, const polyvecl *v);

#define polyfixfixvecl_sub DARTS_NAMESPACE(polyfixfixvecl_sub)
void polyfixfixvecl_sub(polyfixvecl *w, const polyfixvecl *u,
                        const polyfixvecl *v);
#define polyfixvecl_double DARTS_NAMESPACE(polyfixvecl_double)
void polyfixvecl_double(polyfixvecl *b, const polyfixvecl *a);

#define polyfixvecl_round DARTS_NAMESPACE(polyfixvecl_round)
void polyfixvecl_round(polyvecl *a, const polyfixvecl *b);

#define polyfixveclk_sqnorm2 DARTS_NAMESPACE(polyfixveclk_sqnorm2) // z = (Z_1, z_2) 的 l2 范数
uint64_t polyfixveclk_sqnorm2(const polyfixvecl *a, const polyfixveck *b);

#define polyfixveclk_sample_hyperball DARTS_NAMESPACE(polyfixveclk_sample_hyperball)
uint16_t polyfixveclk_sample_hyperball(polyfixvecl *y1, polyfixveck *y2, uint8_t *b, const uint8_t seed[CRHBYTES], const uint16_t nonce);


#endif