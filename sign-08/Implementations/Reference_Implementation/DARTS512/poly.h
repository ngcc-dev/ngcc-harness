#ifndef POLY_H
#define POLY_H

#include "params.h"
#include "reduce.h"
#include "sampler.h"
#include <stdint.h>

/*
* Elenments of R_q = Z_q[X]/(X^N + 1) represents polynomial
* coeffs[0] + X * coeffs[1] + X^2 * xoeffs[2] + ... + X^{N-1} * coeffs[N-1]
*/

typedef struct{
    int32_t coeffs[N];
} poly;


// 定义多项式的模约化;

#define poly_freeze DARTS_NAMESPACE(poly_freeze)
void poly_freeze(poly *a);


// 定义多项式的压缩与解压缩
#define poly_compress DARTS_NAMESPACE(poly_compress) // 多项式压缩
void poly_compress(poly *a_high, const poly *a);

#define poly_decompress DARTS_NAMESPACE(poly_decompress) // 多项式解压缩
void poly_decompress(poly *a_mid, const poly *a_high);

#define poly_lowbits DARTS_NAMESPACE(poly_lowbits) // 多项式低位
void poly_lowbits(poly *a_low, const poly *a);

#define poly_compose DARTS_NAMESPACE(poly_compose) // 多项式合成
void poly_compose(poly *r, const poly *a_low, const poly *a_high);

// 定义的多项式之间的运算
#define poly_mul_halfq DARTS_NAMESPACE(poly_mul_halfq) // 乘 (Q+1)/2
void poly_mul_halfq(poly *r, const poly *a);

#define poly_equal DARTS_NAMESPACE(poly_equal) // 多项式相等判断
int poly_equal(const poly *a, const poly *b);

#define poly_mul_1_b DARTS_NAMESPACE(poly_mul_1_b) // 乘 1-b
void poly_mul_1_b(poly *r, const poly *a, uint8_t b); 

#define poly_ntt DARTS_NAMESPACE(poly_ntt) // NTT
void poly_ntt(poly *r);

#define poly_invntt_tomont DARTS_NAMESPACE(poly_invntt_tomont) // Inverse NTT
void poly_invntt_tomont(poly *r);

#define poly_mul_mont DARTS_NAMESPACE(poly_mul_mont) // 转换到蒙哥马利域
void poly_mul_mont(poly *r);

#define poly_add DARTS_NAMESPACE(poly_add) // 加法
void poly_add(poly *r, const poly *a, const poly *b);

#define poly_sub DARTS_NAMESPACE(poly_sub) // 减法
void poly_sub(poly *r, const poly *a, const poly *b);

#define poly_caddq DARTS_NAMESPACE(poly_caddq) // 条件加 q
void poly_caddq(poly *r);

#define poly_basemul_montgomery DARTS_NAMESPACE(poly_basemul_montgomery) // 乘法
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b);

#define poly_baseinv DARTS_NAMESPACE(poly_baseinv) // 乘法逆元
int poly_baseinv(poly *b, const poly *a);

#define poly_neg DARTS_NAMESPACE(poly_neg) // 取反
void poly_neg(poly *r);

// 定义多项式的封装
#define poly_q_pack DARTS_NAMESPACE(poly_q_pack) // 模 q 的多项式的封装
void poly_q_pack(uint8_t *r, const poly *a);

#define poly_q_unpack DARTS_NAMESPACE(poly_q_unpack) // 模 q 的多项式的解封装 
void poly_q_unpack(poly *r, const uint8_t *a);

#define poly_p_pack DARTS_NAMESPACE(poly_p_pack) // 秘密向量 s 的封装
void poly_p_pack(uint8_t *r, const poly *a);

#define poly_p_unpack DARTS_NAMESPACE(poly_p_unpack) // 秘密向量 s 的解封装
void poly_p_unpack(poly *r, const uint8_t *a);

#define poly_pack_highbits DARTS_NAMESPACE(poly_pack_highbits) // 多项式系数高位的封装
void poly_pack_highbits(uint8_t *r, const poly *a);

#define poly_pack_compressed DARTS_NAMESPACE(poly_pack_compressed) // 打包已压缩的D位值
void poly_pack_compressed(uint8_t *r, const poly *a);

#define poly_pack_lowbits DARTS_NAMESPACE(poly_pack_lowbits) // 多项式系数低位的封装
void poly_pack_lowbits(uint8_t *r, const poly *a);

#define poly_unpack_lowbits DARTS_NAMESPACE(poly_unpack_lowbits) // 多项式系数低位的解封装
void poly_unpack_lowbits(poly *r, const uint8_t *a);

// 多项式的采样
#define poly_challenge DARTS_NAMESPACE(poly_challenge) // 生成 Challenge 多项式
void poly_challenge(poly *c, const uint8_t highbits[POLYVECK_HIGHBITS_PACKEDBYTES], const uint8_t mu[CRHBYTES]);

#define poly_uniform DARTS_NAMESPACE(poly_uniform) // 生成均匀分布多项式
void poly_uniform(poly *a, const uint8_t seed[SEEDBYTES], uint16_t nonce);

#define poly_ternary_p DARTS_NAMESPACE(poly_ternary_p) // 生成符合概率 P 分布的多项式
void poly_ternary_p(poly *a, const uint8_t seed[CRHBYTES], uint16_t nonce);

#endif
