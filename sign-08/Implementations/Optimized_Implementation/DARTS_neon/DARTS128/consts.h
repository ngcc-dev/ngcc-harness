#ifndef CONSTS_H
#define CONSTS_H

#include "params.h"
#include <stdint.h>

// qdata begins with 8-lane scalar constants to mirror the AVX2 layout.
// The NEON implementation reads these constants either as scalars or 4 lanes.
#define _8XQ             0
#define _8XQINV          8
#define _8XQREC         16
#define _8XMONTSQ        24
#define _8XDQ           32
#define _8XLNBITS        40
#define _8X1_SHL_D      48
#define _8XLNHALF      56

#if DARTS_MODE == 128 || DARTS_MODE == 256
    #define _ZETAS       64
    #define _ZETAS_INV  192
#elif DARTS_MODE == 512
    #define _ZETAS       64
    #define _ZETAS_INV  320
#endif


extern const int32_t qdata[];

#define zetas (&qdata[_ZETAS])
#define zetas_inv (&qdata[_ZETAS_INV])

#endif
