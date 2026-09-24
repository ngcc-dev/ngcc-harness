#ifndef SIGN_POLYMAT_H
#define SIGN_POLYMAT_H

#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include <stdint.h>


#define polymatkl_expand DARTS_NAMESPACE(polymatkl_expand)
void polymatkl_expand(polyvecl mat[K], const uint8_t seed[SEEDBYTES]);

#define polymatkl_1_expand DARTS_NAMESPACE(polymatkl_1_expand)
void polymatkl_1_expand(polyvecl_1 mat[K], const uint8_t seed[SEEDBYTES]);

#define polymatkl_1_pointwise_montgomery DARTS_NAMESPACE(polymatkl_1_pointwise_montgomery)
void polymatkl_1_pointwise_montgomery(polyveck *r, const polyvecl_1 mat[K], const polyvecl_1 *a);

#define polymatkl_pointwise_montgomery DARTS_NAMESPACE(polymatkl_pointwise_montgomery)
void polymatkl_pointwise_montgomery(polyveck *r, const polyvecl mat[K], const polyvecl *a);

#define polymatk1_pointwise_montgomery DARTS_NAMESPACE(polymatk1_pointwise_montgomery)
void polymatk1_pointwise_montgomery(polyveck *r, const polyveck *a, const poly *b);

#endif