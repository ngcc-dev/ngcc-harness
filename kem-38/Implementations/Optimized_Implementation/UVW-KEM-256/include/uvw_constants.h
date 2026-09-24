// uvw_constants.h
#ifndef UVW_CONSTANTS_H
#define UVW_CONSTANTS_H

#include "params.h"  
#include "gf_math.h" 

// ====================================================================
// UVW 方案全局预计算常量表
// ====================================================================

//  有限域 GF(UVW_Q) 的乘法逆元表
extern const gf_elem_t gf_inv_table[UVW_Q];

//  RS 码的 Vandermonde 生成矩阵 (固定求值点 1 到 UVW_N / 2)
extern const gf_elem_t FIXED_G_RS[UVW_K2 * UVW_N / 2];

#endif // UVW_CONSTANTS_H