#ifndef CONSTS_H
#define CONSTS_H

#include "params.h"

/* Offsets into qdata[] for 16x replicated constants */
#define _16XQ            0
#define _16XQINV        16
#define _16XV           32
#define _ZETAS_EXP      48

/* Assembly symbol decoration */
#ifdef __ASSEMBLER__
#if defined(__WIN32__) || defined(__APPLE__)
#define decorate(s) _##s
#define cdecl2(s) decorate(s)
#define cdecl(s) cdecl2(KEM_NAMESPACE(_##s))
#else
#define cdecl(s) KEM_NAMESPACE(_##s)
#endif
#endif

#ifndef __ASSEMBLER__
#include <stdint.h>
#define qdata KEM_NAMESPACE(_qdata)
extern const int16_t qdata[] __attribute__((aligned(64)));
/* Precomputed zeta_inv pairs for invntt_avx:
 * 128 entries × 8 int16: [zeta*QINV ×4, zeta ×4] */
#define qdata_inv KEM_NAMESPACE(_qdata_inv)
extern const int16_t qdata_inv[] __attribute__((aligned(64)));
#endif

#endif
