#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

#define zetas KEM_NAMESPACE(zetas)
extern const int16_t zetas[128];
#define zetas_inv KEM_NAMESPACE(zetas_inv)
extern const int16_t zetas_inv[128];

#define ntt KEM_NAMESPACE(ntt)
void ntt(int16_t poly[N]);
#define invntt_tomont KEM_NAMESPACE(invntt_tomont)
void invntt_tomont(int16_t poly[N]);

#define fqinv KEM_NAMESPACE(fqinv)
int16_t fqinv(int16_t a);

#if KEM_MODE == 128
#define basemul KEM_NAMESPACE(basemul)
void basemul(int16_t r[4],
             const int16_t a[4],
             const int16_t b[4],
             int16_t zeta);
#define baseinv KEM_NAMESPACE(baseinv)
int baseinv(int16_t r[4], const int16_t a[4], int16_t zeta);

#elif KEM_MODE == 256 
#define basemul KEM_NAMESPACE(basemul)
void basemul(int16_t r[8],
             const int16_t a[8],
             const int16_t b[8],
             int16_t zeta);
#define baseinv KEM_NAMESPACE(baseinv)
int baseinv(int16_t r[8], const int16_t a[8], int16_t zeta);

#elif KEM_MODE == 512
#define basemul KEM_NAMESPACE(basemul)
void basemul(int16_t r[16],
             const int16_t a[16],
             const int16_t b[16],
             int16_t zeta);
#define baseinv KEM_NAMESPACE(baseinv)
int baseinv(int16_t r[16], const int16_t a[16], int16_t zeta);

#endif

#endif