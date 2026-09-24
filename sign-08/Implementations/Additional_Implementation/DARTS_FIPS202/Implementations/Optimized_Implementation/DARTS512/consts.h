#ifndef CONSTS_H
#define CONSTS_H

#include "params.h"
#include <stdint.h>

// 这些是 qdata 数组的下标索引，每个向量跨越 8 个 int32_t (即 32 字节)
#define _8XQ             0
#define _8XQINV          8
#define _8XQREC         16
#define _8XMONTSQ        24
#define _8XDQ           32
#define _8XLNBITS        40
#define _8X1_SHL_D      48
#define _8XLNHALF      56

#if SIGN_MODE == 128 || SIGN_MODE == 256
    #define _ZETAS       64
    #define _ZETAS_INV  192
#elif SIGN_MODE == 512
    #define _ZETAS       64
    #define _ZETAS_INV  320
#endif


extern const int32_t qdata[];

#endif