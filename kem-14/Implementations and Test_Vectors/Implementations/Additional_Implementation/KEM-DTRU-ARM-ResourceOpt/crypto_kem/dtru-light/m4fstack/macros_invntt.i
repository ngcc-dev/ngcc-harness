#ifndef MACROS_INVNTT_I
#define MACROS_INVNTT_I

#include "macros.i"

//a里的两个系数分别和twiddle相乘，再做montgomery
.macro mul_twiddle tb, a, twiddle, tmp, tmp2, q, qinv
        smulb\tb \tmp, \a, \twiddle
        smult\tb \a, \a, \twiddle
        montgomery \q, \qinv, \tmp, \tmp2 // reduce -> result in tmp2
montgomery \q, \qinv, \a, \tmp // reduce -> result in tmp
pkhtb \a, \tmp, \tmp2, asr#16 // combine results from above in one register as 16bit halves
.endm

.macro doubleinvbutterfly_last_level a0, a1, a2, twiddle, tmp, tmp2, q, qinv, tmp3, root3
        usub16 \tmp, \a1, \a2

        smulbb \tmp2, \tmp, \root3
        smultb \tmp, \tmp, \root3
        montgomery \q, \qinv, \tmp2, \tmp3
        montgomery \q, \qinv, \tmp, \tmp2
        pkhtb \tmp, \tmp2, \tmp3, asr#16 //tmp=tpho

usub16 \tmp2, \a0, \a1
        usub16 \tmp2, \tmp2, \tmp //tmp2=tb
usub16 \tmp3, \a0, \a2
        uadd16 \tmp3, \tmp3, \tmp //tmp3=tc

uadd16 \a0, \a0, \a1
        uadd16 \a0, \a0, \a2

        smulbb \tmp, \tmp2, \twiddle
        smultb \a1, \tmp2, \twiddle
        montgomery \q, \qinv, \tmp, \tmp2
        montgomery \q, \qinv, \a1, \tmp
        pkhtb \a1, \tmp, \tmp2, asr#16

smulbt \tmp, \tmp3, \twiddle
        smultt \a2, \tmp3, \twiddle
        montgomery \q, \qinv, \tmp, \tmp2
        montgomery \q, \qinv, \a2, \tmp
        pkhtb \a2, \tmp, \tmp2, asr#16
.endm

.macro doubleinvbutterfly tb, a0, a1, twiddle, tmp, tmp2, q, qinv
        usub16 \tmp, \a0, \a1
        uadd16 \a0, \a0, \a1
        smulb\tb \a1, \tmp, \twiddle
        smult\tb \tmp, \tmp, \twiddle
        montgomery \q, \qinv, \a1, \tmp2
        montgomery \q, \qinv, \tmp, \a1
        pkhtb \a1, \a1, \tmp2, asr#16
.endm

.macro two_doubleinvbutterfly tb1, tb2, a0, a1, a2, a3, twiddle, tmp, tmp2, q, qinv
        doubleinvbutterfly \tb1, \a0, \a1, \twiddle, \tmp, \tmp2, \q, \qinv
        doubleinvbutterfly \tb2, \a2, \a3, \twiddle, \tmp, \tmp2, \q, \qinv
.endm

.macro doubleinvbutterfly_first_level a0, a1, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
        usub16 \tmp, \a0, \a1
        smulbb \tmp2, \tmp, \twiddle
        smultb \tmp, \tmp, \twiddle
        montgomery \q, \qinv, \tmp2, \twiddle // twiddle is used as temp register
montgomery \q, \qinv, \tmp, \tmp2
        pkhtb \tmp2, \tmp2, \twiddle, asr#16 // tmp2 = t
uadd16 \a0, \a0, \a1
        usub16 \a0, \a0, \tmp2
        ldr.w \twiddle, [\twiddle_ptr, #14]
mul_twiddle b, \a0, \twiddle, \tmp, \a1, \q, \qinv
        smulbt \tmp, \tmp2, \twiddle
        smultt \tmp2, \tmp2, \twiddle
        montgomery \q, \qinv, \tmp, \a1
        montgomery \q, \qinv, \tmp2, \tmp
        pkhtb \a1, \tmp, \a1, asr#16
.endm

#endif /* MACROS_INVNTT_I */
