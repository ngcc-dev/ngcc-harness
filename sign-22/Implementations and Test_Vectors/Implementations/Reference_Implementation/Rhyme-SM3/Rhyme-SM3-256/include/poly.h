#ifndef RHYME_POLY_H
#define RHYME_POLY_H
#include <stdint.h>
#include "params.h"

typedef struct { int32_t coeffs[N]; } poly;

#define poly_zero RHYME_NAMESPACE(poly_zero)
#define poly_add RHYME_NAMESPACE(poly_add)
#define poly_sub RHYME_NAMESPACE(poly_sub)
#define poly_ntt RHYME_NAMESPACE(poly_ntt)
#define poly_invntt_tomont RHYME_NAMESPACE(poly_invntt_tomont)
#define poly_basemul RHYME_NAMESPACE(poly_basemul)
#define poly_basemul_acc RHYME_NAMESPACE(poly_basemul_acc)
#define poly_freeze RHYME_NAMESPACE(poly_freeze)
#define poly_freeze2q RHYME_NAMESPACE(poly_freeze2q)
#define poly_creduce RHYME_NAMESPACE(poly_creduce)
#define poly_chknorm RHYME_NAMESPACE(poly_chknorm)
#define poly_sqnorm_acc RHYME_NAMESPACE(poly_sqnorm_acc)
#define poly_compare RHYME_NAMESPACE(poly_compare)
#define poly_mul_small_acc RHYME_NAMESPACE(poly_mul_small_acc)
#define poly_uniform_ntt RHYME_NAMESPACE(poly_uniform_ntt)

void poly_zero(poly *p);
void poly_add(poly *c, const poly *a, const poly *b);
void poly_sub(poly *c, const poly *a, const poly *b);
void poly_ntt(poly *p);
void poly_invntt_tomont(poly *p);
void poly_basemul(poly *c, const poly *a, const poly *b);
void poly_basemul_acc(poly *c, const poly *a, const poly *b);
void poly_freeze(poly *p);
void poly_freeze2q(poly *p);
void poly_creduce(poly *p);
int  poly_chknorm(const poly *p, int32_t bound);     /* 1 if some |coeff| > bound */
void poly_sqnorm_acc(uint64_t *acc, const poly *p);  /* acc += sum coeff^2 */
int  poly_compare(const poly *a, const poly *b);
/* exact negacyclic c += a*b over Z (int64 accumulation, caller bounds output) */
void poly_mul_small_acc(poly *c, const poly *a, const poly *b);
void poly_uniform_ntt(poly *p, const uint8_t seed[SEEDBYTES], uint16_t nonce);

#endif
