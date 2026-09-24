#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"
#include "consts.h"

#define zetas BWKEM128_NAMESPACE(zetas)
extern const int16_t zetas[128];

#define ntt BWKEM128_NAMESPACE(ntt)
void ntt(int16_t poly[BWKEM128_N]);

#define invntt BWKEM128_NAMESPACE(invntt)
void invntt(int16_t poly[BWKEM128_N]);

#define basemul BWKEM128_NAMESPACE(basemul)
void basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta);

#define ntt_avx BWKEM128_NAMESPACE(ntt_avx)
void ntt_avx(int16_t r[BWKEM128_N], const int16_t *qdata);

#define invntt_avx BWKEM128_NAMESPACE(invntt_avx)
void invntt_avx(int16_t r[BWKEM128_N], const int16_t *qdata);

#define basemul_avx BWKEM128_NAMESPACE(basemul_avx)
void basemul_avx(int16_t r[BWKEM128_N],
                 const int16_t a[BWKEM128_N],
                 const int16_t b[BWKEM128_N],
                 const int16_t *qdata);

#define ntttobytes_avx BWKEM128_NAMESPACE(ntttobytes_avx)
void ntttobytes_avx(uint8_t r[BWKEM128_POLYBYTES],
                    const int16_t a[BWKEM128_N],
                    const int16_t *qdata);

#define nttfrombytes_avx BWKEM128_NAMESPACE(nttfrombytes_avx)
void nttfrombytes_avx(int16_t r[BWKEM128_N],
                      const uint8_t a[BWKEM128_POLYBYTES],
                      const int16_t *qdata);

#define nttunpack_avx BWKEM128_NAMESPACE(nttunpack_avx)
void nttunpack_avx(int16_t r[BWKEM128_N], const int16_t *qdata);

#define reduce_avx BWKEM128_NAMESPACE(reduce_avx)
void reduce_avx(int16_t r[BWKEM128_N], const int16_t *qdata);

#define tomont_avx BWKEM128_NAMESPACE(tomont_avx)
void tomont_avx(int16_t r[BWKEM128_N], const int16_t *qdata);

#endif
