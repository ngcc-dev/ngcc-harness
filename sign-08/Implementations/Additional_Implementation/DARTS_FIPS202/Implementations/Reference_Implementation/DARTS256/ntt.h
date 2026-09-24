#ifndef NTT_H
#define NTT_H

#include "params.h"
#include <stdint.h>

#if SIGN_MODE == 128 || SIGN_MODE == 256

#define zetas DARTS_NAMESPACE(zetas)
extern const int32_t zetas[128];

#define zetas_inv DARTS_NAMESPACE(zetas_inv)
extern const int32_t zetas_inv[128];

#elif SIGN_MODE == 512

#define zetas DARTS_NAMESPACE(zetas)
extern const int32_t zetas[256];

#define zetas_inv DARTS_NAMESPACE(zetas_inv)
extern const int32_t zetas_inv[256];

#endif

#define ntt DARTS_NAMESPACE(ntt)
void ntt(int32_t a[N]);

#define invntt_tomont DARTS_NAMESPACE(invntt_tomont)
void invntt_tomont(int32_t a[N]);

#define basemul DARTS_NAMESPACE(basemul)
void basemul(int32_t r[4], const int32_t a[4], const int32_t b[4], int32_t zeta);

#define baseinv DARTS_NAMESPACE(baseinv)
int baseinv(int32_t r[4], const int32_t a[4], int32_t zeta);

#endif                                                                                                                  