#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

#if WEAVER_N == 128
#define zetas WEAVER_NAMESPACE(_zetas)
extern const int16_t zetas[128];
#elif WEAVER_N == 256 || WEAVER_N == 512
#define zetas WEAVER_NAMESPACE(_zetas)
extern const int16_t zetas[256];
#endif

//#define zetas_inv WEAVER_NAMESPACE(_zetas_inv)
//extern const int16_t zetas_inv[128];

#define ntt WEAVER_NAMESPACE(_ntt)
void ntt(int16_t poly[WEAVER_N]);

#define invntt WEAVER_NAMESPACE(_invntt)
void invntt(int16_t poly[WEAVER_N]);

#define basemul WEAVER_NAMESPACE(_basemul)
void basemul(int16_t r[2],
             const int16_t a[2],
             const int16_t b[2],
             int16_t zeta);

//void basemul_degree4(int16_t r[4], const int16_t a[4], const int16_t b[4], int16_t zeta);

#endif

