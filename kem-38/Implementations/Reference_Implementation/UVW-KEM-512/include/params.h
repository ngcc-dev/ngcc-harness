#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>


//=== Level 3 (目标: 512经典 / 256量子) ===
#define UVW_Q 1709
#define UVW_Q_BITS 11
#define UVW_N 3412
#define UVW_K 1706
#define UVW_K1 853
#define UVW_K2 853
#define UVW_W 463       
#define UVW_M 512

// uint16_t 它严格精确地等于 16 个二进制位
typedef uint16_t gf_elem_t;

#endif // PARAMS_H