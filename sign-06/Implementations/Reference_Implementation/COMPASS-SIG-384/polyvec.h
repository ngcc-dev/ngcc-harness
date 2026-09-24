#ifndef POLYVEC_H
#define POLYVEC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

/* Vectors of polynomials of length L */
typedef struct {
  poly vec[L];
} polyvecl;

#define polyvecl_uniform_eta COMPASS_SIG_NAMESPACE(polyvecl_uniform_eta)
void polyvecl_uniform_eta(polyvecl *v, const uint8_t seed[CRHBYTES], uint16_t nonce);

#define polyvecl_uniform_eta_s COMPASS_SIG_NAMESPACE(polyvecl_uniform_eta_s)
void polyvecl_uniform_eta_s(polyvecl *v, const uint8_t seed[CRHBYTES], uint16_t nonce);

#define polyvecl_uniform_gamma1 COMPASS_SIG_NAMESPACE(polyvecl_uniform_gamma1)
void polyvecl_uniform_gamma1(polyvecl *v, const uint8_t seed[CRHBYTES], uint16_t nonce);

#define polyvecl_reduce COMPASS_SIG_NAMESPACE(polyvecl_reduce)
void polyvecl_reduce(polyvecl *v);

#define polyvecl_add COMPASS_SIG_NAMESPACE(polyvecl_add)
void polyvecl_add(polyvecl *w, const polyvecl *u, const polyvecl *v);

#define polyvecl_ntt COMPASS_SIG_NAMESPACE(polyvecl_ntt)
void polyvecl_ntt(polyvecl *v);
#define polyvecl_invntt_tomont COMPASS_SIG_NAMESPACE(polyvecl_invntt_tomont)
void polyvecl_invntt_tomont(polyvecl *v);
#define polyvecl_pointwise_poly_montgomery COMPASS_SIG_NAMESPACE(polyvecl_pointwise_poly_montgomery)
void polyvecl_pointwise_poly_montgomery(polyvecl *r, const poly *a, const polyvecl *v);
#define polyvecl_pointwise_acc_montgomery \
        COMPASS_SIG_NAMESPACE(polyvecl_pointwise_acc_montgomery)
void polyvecl_pointwise_acc_montgomery(poly *w,
                                       const polyvecl *u,
                                       const polyvecl *v);


#define polyvecl_chknorm COMPASS_SIG_NAMESPACE(polyvecl_chknorm)
int polyvecl_chknorm(const polyvecl *v, int32_t B);



/* Vectors of polynomials of length K */
typedef struct {
  poly vec[K];
} polyveck;

#define polyveck_uniform_eta COMPASS_SIG_NAMESPACE(polyveck_uniform_eta)
void polyveck_uniform_eta(polyveck *v, const uint8_t seed[CRHBYTES], uint16_t nonce);

#define polyveck_reduce COMPASS_SIG_NAMESPACE(polyveck_reduce)
void polyveck_reduce(polyveck *v);
#define polyveck_caddq COMPASS_SIG_NAMESPACE(polyveck_caddq)
void polyveck_caddq(polyveck *v);

#define polyveck_add COMPASS_SIG_NAMESPACE(polyveck_add)
void polyveck_add(polyveck *w, const polyveck *u, const polyveck *v);
#define polyveck_sub COMPASS_SIG_NAMESPACE(polyveck_sub)
void polyveck_sub(polyveck *w, const polyveck *u, const polyveck *v);
#define polyveck_shiftl COMPASS_SIG_NAMESPACE(polyveck_shiftl)
void polyveck_shiftl(polyveck *v);

#define polyveck_ntt COMPASS_SIG_NAMESPACE(polyveck_ntt)
void polyveck_ntt(polyveck *v);
#define polyveck_invntt_tomont COMPASS_SIG_NAMESPACE(polyveck_invntt_tomont)
void polyveck_invntt_tomont(polyveck *v);
#define polyveck_pointwise_poly_montgomery COMPASS_SIG_NAMESPACE(polyveck_pointwise_poly_montgomery)
void polyveck_pointwise_poly_montgomery(polyveck *r, const poly *a, const polyveck *v);

#define polyveck_chknorm COMPASS_SIG_NAMESPACE(polyveck_chknorm)
int polyveck_chknorm(const polyveck *v, int32_t B);

#define polyveck_power2round COMPASS_SIG_NAMESPACE(polyveck_power2round)
void polyveck_power2round(polyveck *v1, polyveck *v0, const polyveck *v);
#define polyveck_decompose COMPASS_SIG_NAMESPACE(polyveck_decompose)
void polyveck_decompose(polyveck *v1, polyveck *v0, const polyveck *v);
#define polyveck_make_hint COMPASS_SIG_NAMESPACE(polyveck_make_hint)
unsigned int polyveck_make_hint(polyveck *h,
                                const polyveck *v0,
                                const polyveck *v1);
#define polyveck_use_hint COMPASS_SIG_NAMESPACE(polyveck_use_hint)
void polyveck_use_hint(polyveck *w, const polyveck *v, const polyveck *h);

#define polyvec_matrix_expand COMPASS_SIG_NAMESPACE(polyvec_matrix_expand)
void polyvec_matrix_expand(polyvecl mat[K], const uint8_t rho[SEEDBYTES]);

#define polyvec_matrix_pointwise_montgomery COMPASS_SIG_NAMESPACE(polyvec_matrix_pointwise_montgomery)
void polyvec_matrix_pointwise_montgomery(polyveck *t, const polyvecl mat[K], const polyvecl *v);

// 新增：计算 L 维向量缩放后的欧氏范数平方
#define polyvecl_sqnorm COMPASS_SIG_NAMESPACE(polyvecl_sqnorm)
uint64_t polyvecl_sqnorm(const polyvecl *v);

// 新增：计算 K 维向量缩放后的欧氏范数平方
#define polyveck_sqnorm COMPASS_SIG_NAMESPACE(polyveck_sqnorm)
uint64_t polyveck_sqnorm(const polyveck *v);

#define polyvec_check_L2_bound COMPASS_SIG_NAMESPACE(polyvec_check_L2_bound)
int polyvec_check_L2_bound(const polyvecl *z, const polyveck *w0);

#define polyveck_uniform_eta_e COMPASS_SIG_NAMESPACE(polyveck_uniform_eta_e)
void polyveck_uniform_eta_e(polyveck *v, const uint8_t seed[CRHBYTES], uint16_t nonce);

#define polyveck_pack_w1 COMPASS_SIG_NAMESPACE(polyveck_pack_w1)
void polyveck_pack_w1(uint8_t *r, const polyveck *w1);
#endif
