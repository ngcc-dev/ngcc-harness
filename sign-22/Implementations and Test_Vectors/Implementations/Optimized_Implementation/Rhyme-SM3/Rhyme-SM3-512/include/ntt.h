#ifndef RHYME_NTT_H
#define RHYME_NTT_H
#include <stdint.h>
#include "params.h"

#define ntt RHYME_NAMESPACE(ntt)
#define invntt_tomont RHYME_NAMESPACE(invntt_tomont)
#define basemul RHYME_NAMESPACE(basemul)
#define ntt_pointwise_acc RHYME_NAMESPACE(ntt_pointwise_acc)

/* in-place forward incomplete NTT (degree-4 blocks), coefficients int32 (any range) */
void ntt(int32_t a[N]);
/* in-place inverse; output multiplied by 2^32 (Montgomery domain compensation) */
void invntt_tomont(int32_t a[N]);
/* c = a*b for NTT-domain vectors (deg-4 blockwise), Montgomery semantics:
 * if a,b in standard NTT domain, output = a*b * R^{-1}.  */
void basemul(int32_t c[N], const int32_t a[N], const int32_t b[N]);
/* c += a*b (blockwise), same Montgomery semantics */
void ntt_pointwise_acc(int32_t c[N], const int32_t a[N], const int32_t b[N]);
#endif
