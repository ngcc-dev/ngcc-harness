#ifndef NTT_H
#define NTT_H

#include <stdint.h>

#define ROOT_DIMENSION 8

extern int16_t zetas[128];
extern int16_t zetas_inv[129];

void ntt(int16_t *a);
void invntt(int16_t *a);
void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta);

// AVX2 implementations
extern int16_t zetas1024[128];
extern int16_t zetas1024_inv[128];
extern int16_t zetas1024_base[128];
extern int16_t zetas1024_base_nomont[128];

int  baseinv(int16_t b[], const int16_t a[], int16_t zeta);

void ntt_avx(int16_t a[],const int16_t *qdata);
void invntt_avx(int16_t a[],const int16_t *qdata);
void polydouble_avx(int16_t b[], const int16_t a[]);
void polyadd_avx(int16_t c[], const int16_t a[], const int16_t b[]);
void freeze_avx(int16_t b[],const int16_t *qdata);
void tomont_avx(int16_t b[],const int16_t *qdata);
void barret_avx(int16_t b[],const int16_t *qdata);
void basemul_avx(int16_t c[], const int16_t a[], const int16_t b[], const int16_t *qdata);

int  baseinv_avx(int16_t b[], const int16_t a[], const int16_t *qdata);
void fqred16_avx(int16_t b[]);
void shuffle_avx(int16_t b[], const int16_t a[]);
void ishuffle_avx(int16_t b[], const int16_t a[]);

#endif