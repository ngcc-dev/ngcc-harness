#ifndef OPSSIG_CONSTS_H
#define OPSSIG_CONSTS_H

#define _8XQ 0
#define _8XQINV 8
#define _POINTWISE_NBLOCKS 128

#if defined(__WIN32__) || defined(__APPLE__)
#define decorate(s) _##s
#define cdecl(s) decorate(s)
#else
#define cdecl(s) s
#endif

#ifndef __ASSEMBLER__

#include "params.h"
#include "align.h"

typedef ALIGNED_INT32(16) qdata_t;

extern const qdata_t qdata;

#endif

#endif
