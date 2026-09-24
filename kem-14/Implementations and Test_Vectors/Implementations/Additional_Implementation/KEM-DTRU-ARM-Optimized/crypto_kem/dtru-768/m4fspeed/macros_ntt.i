#ifndef MACROS_NTT_I
#define MACROS_NTT_I

#include "macros.i"
#include "params.h"

.macro doublebutterfly tb, a0, a1, twiddle, tmp, tmp2, q, qinv
  smulb\tb \tmp, \a1, \twiddle
  smult\tb \a1, \a1, \twiddle
  montgomery \q, \qinv, \tmp, \tmp2
  montgomery \q, \qinv, \a1, \tmp
  pkhtb \tmp2, \tmp, \tmp2, asr#16
  usub16 \a1, \a0, \tmp2
  uadd16 \a0, \a0, \tmp2
.endm

.macro two_doublebutterfly tb1, tb2, a0, a1, a2, a3, twiddle, tmp, tmp2, q, qinv
  doublebutterfly \tb1, \a0, \a1, \twiddle, \tmp, \tmp2, \q, \qinv
  doublebutterfly \tb2, \a2, \a3, \twiddle, \tmp, \tmp2, \q, \qinv
.endm

// the first level
.macro doublebutterfly_first_level a0, a1, twiddle, tmp, tmp2, q, qinv
  smulbb \tmp, \a1, \twiddle
  smultb \tmp2, \a1, \twiddle
  montgomery \q, \qinv, \tmp, \twiddle // twiddle is used as temp register
  montgomery \q, \qinv, \tmp2, \tmp
  pkhtb \tmp2, \tmp, \twiddle, asr#16
  uadd16 \a1, \a1, \a0
  usub16 \a1, \a1, \tmp2
  uadd16 \a0, \a0, \tmp2
.endm

// the last level
.macro doublebutterfly_last_level a0, a1, a2, twiddle, tmp, tmp2, q, qinv, tmp3, root3
  smulbb \tmp, \a1, \twiddle
  smultb \tmp2, \a1, \twiddle
  montgomery \q, \qinv, \tmp, \tmp3
  montgomery \q, \qinv, \tmp2, \tmp
  pkhtb \a1, \tmp, \tmp3, asr#16 //a1=tb
  smulbt \tmp, \a2, \twiddle
  smultt \tmp2, \a2, \twiddle
  montgomery \q, \qinv, \tmp, \tmp3
  montgomery \q, \qinv, \tmp2, \tmp
  pkhtb \a2, \tmp, \tmp3, asr#16 //a2=tc
  usub16 \tmp, \a1, \a2 
  smulbb \tmp2, \tmp, \root3
  smultb \tmp3, \tmp, \root3
  montgomery \q, \qinv, \tmp2, \tmp
  montgomery \q, \qinv, \tmp3, \tmp2
  pkhtb \tmp, \tmp2, \tmp, asr#16 //tmp=tpho
  uadd16 \tmp2, \a1, \tmp //tmp2=tb+tpho
  usub16 \tmp3, \a2, \tmp //tmp3=tc-tpho
  uadd16 \tmp, \a1, \a2 //tmp=tb+tc
  usub16 \a2, \a0, \tmp2
  usub16 \a1, \a0, \tmp3
  uadd16 \a0, \a0, \tmp
.endm


// #ifndef OPTIMIZE_STACK
// .macro doublebutterfly_no_montgomery a0, a1, twiddle, tmp, tmp2
//   smulbb \tmp, \a1, \twiddle
//   smultb \a1, \a1, \twiddle
//   pkhbt \tmp2, \tmp, \a1, lsl#16
//   usub16 \a1, \a0, \tmp2
//   uadd16 \a0, \a0, \tmp2
// .endm

// .macro two_doublebutterfly_no_montgomery a0, a1, a2, a3, twiddle, tmp, tmp2
//   doublebutterfly_no_montgomery \a0, \a1, \twiddle, \tmp, \tmp2
//   doublebutterfly_no_montgomery \a2, \a3, \twiddle, \tmp, \tmp2
// .endm
// #endif

#endif /* MACROS_NTT_I */
