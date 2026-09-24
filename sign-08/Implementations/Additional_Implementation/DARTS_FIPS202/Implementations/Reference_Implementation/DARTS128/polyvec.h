#ifndef SIGN_POLYVEC_H
#define SIGN_POLYVEC_H

#include "params.h"
#include "poly.h"
#include <stdint.h>

/* Vectors of polynomials of length K */
typedef struct {
    poly vec[K];
} polyveck;

// 定义 q+1 / 2 beta 多项式
#define polyveck_halfq_beta DARTS_NAMESPACE(polyveck_halfq_beta)
void polyveck_halfq_beta(polyveck *r);

// 定义多项式向量的模约化
#define polyveck_reduce2q DARTS_NAMESPACE(polyveck_reduce2q)
void polyveck_reduce2q(polyveck *a);

#define polyveck_freeze2q DARTS_NAMESPACE(polyveck_freeze2q)
void polyveck_freeze2q(polyveck *a);

#define polyveck_freeze DARTS_NAMESPACE(polyveck_freeze)
void polyveck_freeze(polyveck *a);

// 定义多项式向量的压缩与解压缩
#define polyveck_compress DARTS_NAMESPACE(polyveck_compress) // 多项式向量压缩——取高位
void polyveck_compress(polyveck *a_high, const polyveck *a);

#define polyveck_decompress DARTS_NAMESPACE(polyveck_decompress) // 多项式向量解压缩——还原中位
void polyveck_decompress(polyveck *a_mid, const polyveck *a_high);

#define polyveck_lowbits DARTS_NAMESPACE(polyveck_lowbits) // 多项式向量低位
void polyveck_lowbits(polyveck *a_low, const polyveck *a);

// 定义的多项式向量之间的运算
#define polyveck_add DARTS_NAMESPACE(polyveck_add) // 加法
void polyveck_add(polyveck *r, const polyveck *a, const polyveck *b);

#define polyveck_sub DARTS_NAMESPACE(polyveck_sub) // 减法
void polyveck_sub(polyveck *r, const polyveck *a, const polyveck *b);

#define polyveck_double DARTS_NAMESPACE(polyveck_double) // 乘2
void polyveck_double(polyveck *r);

#define polyveck_neg DARTS_NAMESPACE(polyveck_neg) // 取反
void polyveck_neg(polyveck *r);

#define polyveck_cneg DARTS_NAMESPACE(polyveck_cneg) // 双峰拒绝采样时决定是否取反
void polyveck_cneg(polyveck *v, const uint8_t b);

#define polyveck_frommont DARTS_NAMESPACE(polyveck_frommont) // 乘 MONTSQ
void polyveck_frommont(polyveck *r);

#define polyveck_ntt DARTS_NAMESPACE(polyveck_ntt) // NTT
void polyveck_ntt(polyveck *r);

#define polyveck_invntt_tomont DARTS_NAMESPACE(polyveck_invntt_tomont) // Inverse NTT
void polyveck_invntt_tomont(polyveck *r);

#define polyveck_basemul_montgomery DARTS_NAMESPACE(polyveck_basemul_montgomery) // 乘法
void polyveck_basemul_montgomery(polyveck *r, const polyveck *a, const polyveck *b);

#define polyveck_caddq DARTS_NAMESPACE(polyveck_caddq)
void polyveck_caddq(polyveck *r);

#define polyveck_sqnorm2 DARTS_NAMESPACE(polyveck_sqnorm2)
uint64_t polyveck_sqnorm2(const polyveck *r);

#define polyveck_cadd_2_d DARTS_NAMESPACE(polyveck_cadd_2_d)
void polyveck_cadd_2_d(polyveck *r);


// 定义多项式的封装
#define polyveck_q_pack DARTS_NAMESPACE(polyveck_q_pack)
void polyveck_q_pack(uint8_t *r, const polyveck *a);

#define polyveck_pack_highbits DARTS_NAMESPACE(polyveck_pack_highbits)
void polyveck_pack_highbits(uint8_t *r, const polyveck *a);

#define polyveck_pack_compressed DARTS_NAMESPACE(polyveck_pack_compressed)
void polyveck_pack_compressed(uint8_t *r, const polyveck *a);

// 定义多项式向量的采样
#define polyveck_expand DARTS_NAMESPACE(polyveck_expand)
void polyveck_expand(polyveck *r, const uint8_t seed[SEEDBYTES]);


/* Vectors of polynomials of length L */
typedef struct {
    poly vec[L];
} polyvecl;

// 定义多项式向量的模约化
#define polyvecl_freeze DARTS_NAMESPACE(polyvecl_freeze)
void polyvecl_freeze(polyvecl *r);

// 定义多项式向量的封装
#define polyvecl_pack_lowbits DARTS_NAMESPACE(polyvecl_pack_lowbits)
void polyvecl_pack_lowbits(uint8_t *r, const polyvecl *a);

#define polyvecl_unpack_lowbits DARTS_NAMESPACE(polyvecl_unpack_lowbits)
void polyvecl_unpack_lowbits(polyvecl *r, const uint8_t *a);

// 定义多项式向量的压缩与解压缩
#define polyvecl_compress DARTS_NAMESPACE(polyvecl_compress) // 多项式向量压缩——取高位
void polyvecl_compress(polyvecl *a_high, const polyvecl *a);

#define polyvecl_decompress DARTS_NAMESPACE(polyvecl_decompress) // 多项式向量解压缩——还原中位
void polyvecl_decompress(polyvecl *a_mid, const polyvecl *a_high);

#define polyvecl_lowbits DARTS_NAMESPACE(polyvecl_lowbits) // 多项式向量低位
void polyvecl_lowbits(polyvecl *a_low, const polyvecl *a_high);

#define polyvecl_compose DARTS_NAMESPACE(polyvecl_compose) // 多项式向量合成
void polyvecl_compose(polyvecl *r, const polyvecl *a_low, const polyvecl *a_high);

// 定义多项式向量之间的运算
#define polyvecl_ntt DARTS_NAMESPACE(polyvecl_ntt)
void polyvecl_ntt(polyvecl *r);

#define polyvecl_invntt_tomont DARTS_NAMESPACE(polyvecl_invntt_tomont)
void polyvecl_invntt_tomont(polyvecl *r);

#define polyvecl_basemul_acc_montgomery DARTS_NAMESPACE(polyvecl_basemul_acc_montgomery)
void polyvecl_basemul_acc_montgomery(poly *r, const polyvecl *a, const polyvecl *b);

#define polyvecl_cneg DARTS_NAMESPACE(polyvecl_cneg) // 双峰拒绝采样时决定是否取反
void polyvecl_cneg(polyvecl *v, const uint8_t b);

#define polyvecl_sqnorm2 DARTS_NAMESPACE(polyvecl_sqnorm2)
uint64_t polyvecl_sqnorm2(const polyvecl *r);

/* Vectors of polynomials of length L-1 */
typedef struct {
    poly vec[L-1];
} polyvecl_1;

#define polyvecl_1_ntt DARTS_NAMESPACE(polyvecl_1_ntt)
void polyvecl_1_ntt(polyvecl_1 *r);

#define polyvecl_1_basemul_acc_montgomery DARTS_NAMESPACE(polyvecl_1_basemul_montgomery)
void polyvecl_1_basemul_acc_montgomery(poly *r, const polyvecl_1 *a, const polyvecl_1 *b);

#define polyveckl_sqsing_value DARTS_NAMESPACE(polyveckl_sqsing_value) // 计算 N(s)
int64_t polyveckl_sqsing_value(const poly *s0, const polyvecl_1 *s1, const polyveck *e);

// 定义多项式向量的采样
#define polyveckl_ternary_p DARTS_NAMESPACE(polyveckl_ternary_p) // 采样私钥 s = (s0, s1, e)
void polyveckl_ternary_p(poly *s0, polyvecl_1 *s1, polyveck *e, const uint8_t seed[CRHBYTES], uint16_t nonce);

#endif