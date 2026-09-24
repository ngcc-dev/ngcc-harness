// Adapt from https://github.com/pq-crystals/mlkem/tree/main/avx2
#ifndef CONSTS_H
#define CONSTS_H

#include "p512_params.h"

#define _16XQ 0
#define _16XQINV 16
#define _16XV 32
#define _16XFLO 48
#define _16XFHI 64
#define _16XMONTSQLO 80
#define _16XMONTSQHI 96
#define _16XMASK 112
#define _REVIDXB 128
#define _REVIDXD 144
#define _ZETAS_EXP 160
#define _16XSHIFT 1120
#define _16XTWO9 1136

#ifdef __ASSEMBLER__
#if defined(__WIN32__) || defined(__APPLE__)
#define decorate(s) _##s
#define cdecl2(s) decorate(s)
#define cdecl(s) cdecl2(DKE_NAMESPACE(##s))
#else
#define cdecl(s) DKE_NAMESPACE(##s)
#endif
#endif

#ifndef __ASSEMBLER__
#include "align.h"
typedef ALIGNED_INT16(1152) qdata_t;
extern const qdata_t dke_p512_qdata;
#endif

#endif
