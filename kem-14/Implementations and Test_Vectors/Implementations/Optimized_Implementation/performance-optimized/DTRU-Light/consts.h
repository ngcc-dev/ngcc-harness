#ifndef CONSTS_H
#define CONSTS_H
#include<stdint.h>
#include "params.h"

#ifndef __ASSEMBLER__
#include "align.h"
typedef ALIGNED_INT16(6400) qdata_t;

extern const qdata_t qdata;
#endif
#endif