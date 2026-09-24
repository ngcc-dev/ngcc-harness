#ifndef MACROS_NTT_I
#define MACROS_NTT_I

#include "macros.i"

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



#endif /* MACROS_NTT_I */
