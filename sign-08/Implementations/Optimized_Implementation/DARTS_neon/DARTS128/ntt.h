#ifndef NTT_H
#define NTT_H

#include "params.h"
#include "consts.h"
#include <stdint.h>

#define ntt DARTS_NAMESPACE(ntt)
void ntt(int32_t a[N]);

#define invntt_tomont DARTS_NAMESPACE(invntt_tomont)
void invntt_tomont(int32_t a[N]);

#define basemul DARTS_NAMESPACE(basemul)
void basemul(int32_t r[4], const int32_t a[4], const int32_t b[4], int32_t zeta);

#define baseinv DARTS_NAMESPACE(baseinv)
int baseinv(int32_t r[4], const int32_t a[4], int32_t zeta);

#endif                                                                                                                  
