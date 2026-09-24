#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"
#if (COMPASS_KEM_Q == 7681)
  #include "zetas_7681_avx.h"
  #define Q_7681    7681
#define QINV_7681 -7679
#define V_7681    8737
#endif



#define zetas COMPASS_KEM_NAMESPACE(zetas)
#if (COMPASS_KEM_Q == 3329) && (COMPASS_KEM_N == 256)
extern const int16_t zetas[128];
#elif (COMPASS_KEM_Q == 7681) && (COMPASS_KEM_N == 512)
extern const int16_t zetas[256];
#endif

#define ntt_avx_7681 COMPASS_KEM_NAMESPACE(ntt_avx_7681)
void ntt_avx_7681(int16_t *r);

#define invntt_avx_7681 COMPASS_KEM_NAMESPACE(invntt_avx_7681)
void invntt_avx_7681(int16_t *r);

#define ntt COMPASS_KEM_NAMESPACE(ntt)
void ntt(int16_t *poly);

#define invntt COMPASS_KEM_NAMESPACE(invntt)
void invntt(int16_t *poly);

#define basemul COMPASS_KEM_NAMESPACE(basemul)
void basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta);

#endif
