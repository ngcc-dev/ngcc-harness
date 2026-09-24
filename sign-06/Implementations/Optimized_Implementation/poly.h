#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"
#include "align.h"

typedef struct {
  // 使用属性对齐，不改变数据层级
  int32_t coeffs[N] __attribute__((aligned(32))); 
} poly;

#define rej_uniform_avx COMPASS_SIG_NAMESPACE(rej_uniform_avx)
unsigned int rej_uniform_avx(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen);

#define poly_nttunpack COMPASS_SIG_NAMESPACE(poly_nttunpack)
void poly_nttunpack(poly *a);

#define poly_uniform_eta_4x COMPASS_SIG_NAMESPACE(poly_uniform_eta_4x)
void poly_uniform_eta_4x(poly *a0, poly *a1, poly *a2, poly *a3,
                         const uint8_t seed[CRHBYTES],
                         uint16_t nonce0, uint16_t nonce1, uint16_t nonce2, uint16_t nonce3,
                         int8_t eta) ;

// unsigned int rej_uniform(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen);
#define poly_uniform_gamma1_4x COMPASS_SIG_NAMESPACE(poly_uniform_gamma1_4x)
void poly_uniform_gamma1_4x(poly *a0, poly *a1, poly *a2, poly *a3,
                            const uint8_t seed[CRHBYTES],
                            uint16_t nonce0, uint16_t nonce1, uint16_t nonce2, uint16_t nonce3) ;

#define poly_uniform_4x COMPASS_SIG_NAMESPACE(poly_uniform_4x)
void poly_uniform_4x(poly *a0, poly *a1, poly *a2, poly *a3,
                     const uint8_t seed[SEEDBYTES],
                     uint16_t nonce0, uint16_t nonce1, uint16_t nonce2, uint16_t nonce3);

#define poly_reduce COMPASS_SIG_NAMESPACE(poly_reduce)
void poly_reduce(poly *a);
#define poly_caddq COMPASS_SIG_NAMESPACE(poly_caddq)
void poly_caddq(poly *a);

#define poly_add COMPASS_SIG_NAMESPACE(poly_add)
void poly_add(poly *c, const poly *a, const poly *b);
#define poly_sub COMPASS_SIG_NAMESPACE(poly_sub)
void poly_sub(poly *c, const poly *a, const poly *b);
#define poly_shiftl COMPASS_SIG_NAMESPACE(poly_shiftl)
void poly_shiftl(poly *a);

#define poly_ntt COMPASS_SIG_NAMESPACE(poly_ntt)
void poly_ntt(poly *a);
#define poly_invntt_tomont COMPASS_SIG_NAMESPACE(poly_invntt_tomont)
void poly_invntt_tomont(poly *a);
#define poly_pointwise_montgomery COMPASS_SIG_NAMESPACE(poly_pointwise_montgomery)
void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b);

#define poly_power2round COMPASS_SIG_NAMESPACE(poly_power2round)
void poly_power2round(poly *a1, poly *a0, const poly *a);
#define poly_decompose COMPASS_SIG_NAMESPACE(poly_decompose)
void poly_decompose(poly *a1, poly *a0, const poly *a);

#define poly_chknorm COMPASS_SIG_NAMESPACE(poly_chknorm)
int poly_chknorm(const poly *a, int32_t B);
#define poly_uniform COMPASS_SIG_NAMESPACE(poly_uniform)
void poly_uniform(poly *a,
                  const uint8_t seed[SEEDBYTES],
                  uint16_t nonce);
#define poly_uniform_eta COMPASS_SIG_NAMESPACE(poly_uniform_eta)
void poly_uniform_eta(poly *a,
                      const uint8_t seed[CRHBYTES],
                      uint16_t nonce,
                      int8_t eta);
#define poly_uniform_gamma1 COMPASS_SIG_NAMESPACE(poly_uniform_gamma1)
void poly_uniform_gamma1(poly *a,
                         const uint8_t seed[CRHBYTES],
                         uint16_t nonce);
#define poly_challenge COMPASS_SIG_NAMESPACE(poly_challenge)
void poly_challenge(poly *c, const uint8_t seed[CTILDEBYTES]);

#define polyeta_pack COMPASS_SIG_NAMESPACE(polyeta_pack)
void polyeta_pack(uint8_t *r, const poly *a, int8_t eta);
#define polyeta_unpack COMPASS_SIG_NAMESPACE(polyeta_unpack)
void polyeta_unpack(poly *r, const uint8_t *a, int8_t eta);

#define polyt1_pack COMPASS_SIG_NAMESPACE(polyt1_pack)
void polyt1_pack(uint8_t *r, const poly *a);
#define polyt1_unpack COMPASS_SIG_NAMESPACE(polyt1_unpack)
void polyt1_unpack(poly *r, const uint8_t *a);

#define polyt0_pack COMPASS_SIG_NAMESPACE(polyt0_pack)
void polyt0_pack(uint8_t *r, const poly *a);
#define polyt0_unpack COMPASS_SIG_NAMESPACE(polyt0_unpack)
void polyt0_unpack(poly *r, const uint8_t *a);

#define polyz_pack COMPASS_SIG_NAMESPACE(polyz_pack)
void polyz_pack(uint8_t *r, const poly *a);
#define polyz_unpack COMPASS_SIG_NAMESPACE(polyz_unpack)
void polyz_unpack(poly *r, const uint8_t *a);

#define polyw1_pack COMPASS_SIG_NAMESPACE(polyw1_pack)
void polyw1_pack(uint8_t *r, const poly *a);

#define poly_sqnorm COMPASS_SIG_NAMESPACE(poly_sqnorm)
uint64_t poly_sqnorm(const poly *a);
#endif
