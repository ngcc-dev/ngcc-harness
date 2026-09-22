#ifndef GF_MATH_H
#define GF_MATH_H

#include "params.h" 




// ==========================================
// 基础域运算 (Basic Arithmetic in Fq)
// ==========================================

/**
 * @brief 模加法: (a + b) mod q
 */
gf_elem_t gf_add(gf_elem_t a, gf_elem_t b);

/**
 * @brief 模减法: (a - b) mod q
 */
gf_elem_t gf_sub(gf_elem_t a, gf_elem_t b);

/**
 * @brief 模乘法: (a * b) mod q
 */
gf_elem_t gf_mul(gf_elem_t a, gf_elem_t b);

/**
 * @brief 模逆元: a^(-1) mod q
 * @return 如果 a为0，通常返回0
 */
gf_elem_t gf_inv(gf_elem_t a);

/**
 * @brief 模幂运算: a^exp mod q
 */
gf_elem_t gf_pow(gf_elem_t a, uint32_t exp);


// ==========================================
// 矩阵与向量运算 (Matrix & Vector Operations)
// ==========================================

/**
 * @brief 向量点积: (u . v) mod q
 * @param len 向量长度
 */
gf_elem_t gf_dot_product(const gf_elem_t *u, const gf_elem_t *v, int len);

/**
 * @brief 向量矩阵乘法: out = vec * G
 * @param out 输出向量 (预先分配好内存，长度为 n)
 * @param vec 输入向量 (长度 k)
 * @param G   输入矩阵 (展平的一维数组，大小 k * n)
 * @param k   矩阵行数
 * @param n   矩阵列数
 */
void gf_vec_mat_mul(gf_elem_t *out, const gf_elem_t *vec, const gf_elem_t *G, int k, int n);

#endif // GF_MATH_H