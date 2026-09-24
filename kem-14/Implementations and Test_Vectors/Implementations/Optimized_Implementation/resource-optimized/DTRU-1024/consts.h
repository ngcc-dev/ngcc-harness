#ifndef CONSTS_H
#define CONSTS_H
#include<stdint.h>
#include "params.h"

#define M2 942   //7681^-1 mod ± 3457
#define M2_MONT2 -194 //3310*942 mod ± 3457
#define M2_MONT2_Q2INV -17858

/* The C ABI on MacOS exports all symbols with a leading
 * underscore. This means that any symbols we refer to from
 * C files (functions) can't be found, and all symbols we
 * refer to from ASM also can't be found.
 *
 * This define helps us get around this
 */



#ifndef __ASSEMBLER__
#include"align.h"
typedef ALIGNED_INT16(10048) qdata_t;
extern const qdata_t qdata;
#endif
#endif


