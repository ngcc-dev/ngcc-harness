#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"
#include "consts.h"

#define zetas KYBER_NAMESPACE(zetas)
extern const int16_t zetas[128];

#define ntt KYBER_NAMESPACE(ntt)
void ntt(int16_t poly[256]);

#define invntt KYBER_NAMESPACE(invntt)
void invntt(int16_t poly[256]);

#define basemul KYBER_NAMESPACE(basemul)
void basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta);

#define ntt_avx KYBER_NAMESPACE(ntt_avx)
void ntt_avx(int16_t r[256], const int16_t *qdata);

#define invntt_avx KYBER_NAMESPACE(invntt_avx)
void invntt_avx(int16_t r[256], const int16_t *qdata);

#define basemul_avx KYBER_NAMESPACE(basemul_avx)
void basemul_avx(int16_t r[256], const int16_t a[256], const int16_t b[256], const int16_t *qdata);

#define ntttobytes_avx KYBER_NAMESPACE(ntttobytes_avx)
void ntttobytes_avx(uint8_t r[KYBER_POLYBYTES], const int16_t a[256], const int16_t *qdata);

#define nttfrombytes_avx KYBER_NAMESPACE(nttfrombytes_avx)
void nttfrombytes_avx(int16_t r[256], const uint8_t a[KYBER_POLYBYTES], const int16_t *qdata);

#define nttunpack_avx KYBER_NAMESPACE(nttunpack_avx)
void nttunpack_avx(int16_t r[256], const int16_t *qdata);

#define reduce_avx KYBER_NAMESPACE(reduce_avx)
void reduce_avx(int16_t r[256], const int16_t *qdata);

#define tomont_avx KYBER_NAMESPACE(tomont_avx)
void tomont_avx(int16_t r[256], const int16_t *qdata);

#endif
