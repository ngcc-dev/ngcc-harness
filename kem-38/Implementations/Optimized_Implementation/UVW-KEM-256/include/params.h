#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>


//=== Level 2 (目标: 256经典 / 128量子) ===
#define UVW_Q 857
#define UVW_Q_BITS 10
#define UVW_N 1708
#define UVW_K 854
#define UVW_K1 427
#define UVW_K2 427
#define UVW_W 232       
#define UVW_M 256

// uint16_t 它严格精确地等于 16 个二进制位
typedef uint16_t gf_elem_t;

#endif // PARAMS_H