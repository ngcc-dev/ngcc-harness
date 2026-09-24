#ifndef CONSTS_H
#define CONSTS_H

#include "params.h"

#define _16XQ            0
#define _16XQINV        16
#define _ZETAS_EXP     48
#define _16XONE 32
#define _ZETAS_QINV_EXP 816
/* The C ABI on MacOS exports all symbols with a leading
 * underscore. This means that any symbols we refer to from
 * C files (functions) can't be found, and all symbols we
 * refer to from ASM also can't be found.
 *
 * This define helps us get around this
 */
#define cdecl(s) s

#ifndef __ASSEMBLER__
#include "align.h"


#endif

//crt参数
#define M2 942   //7681^-1 mod ± 3457
#define M2_MONT2 -194 //3310*942 mod ± 3457
#define M2_MONT2_Q2INV -17858

#endif
