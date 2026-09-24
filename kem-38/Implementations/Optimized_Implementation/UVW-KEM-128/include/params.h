#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>


//=== Level 1 (目标: 128经典 / 80量子) ===
#define UVW_Q 433
#define UVW_Q_BITS 9
#define UVW_N 860
#define UVW_K 430
#define UVW_K1 215
#define UVW_K2 215
#define UVW_W 116       
#define UVW_M 256

// uint16_t 它严格精确地等于 16 个二进制位
typedef uint16_t gf_elem_t;

#endif // PARAMS_H