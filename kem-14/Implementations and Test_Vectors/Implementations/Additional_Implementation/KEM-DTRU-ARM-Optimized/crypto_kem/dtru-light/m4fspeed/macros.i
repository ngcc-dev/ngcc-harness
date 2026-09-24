#ifndef MACROS_I
#define MACROS_I

.macro load a, a0, a1, a2, a3, mem0, mem1, mem2, mem3
        ldr.w \a0, [\a, \mem0]
ldr.w \a1, [\a, \mem1]
ldr.w \a2, [\a, \mem2]
ldr.w \a3, [\a, \mem3]
.endm

.macro store a, a0, a1, a2, a3, mem0, mem1, mem2, mem3
        str.w \a0, [\a, \mem0]
str.w \a1, [\a, \mem1]
str.w \a2, [\a, \mem2]
str.w \a3, [\a, \mem3]
.endm

.macro loadh a, a0, a1, a2, a3, mem0, mem1, mem2, mem3
        ldrsh \a0, [\a, \mem0]
ldrsh \a1, [\a, \mem1]
ldrsh \a2, [\a, \mem2]
ldrsh \a3, [\a, \mem3]
.endm

.macro storeh a, a0, a1, a2, a3, mem0, mem1, mem2, mem3
        strh \a0, [\a, \mem0]
strh \a1, [\a, \mem1]
strh \a2, [\a, \mem2]
strh \a3, [\a, \mem3]
.endm

// doublebarrett_init need 1 constant
.macro doublebarrett_init a, tmp, tmp2, q, barrettconst
        smulbb \tmp, \a, \barrettconst
        smultb \tmp2, \a, \barrettconst
        asr \tmp, \tmp, #24
        asr \tmp2, \tmp2, #24
        smulbb \tmp, \tmp, \q
        smulbb \tmp2, \tmp2, \q
        pkhbt \tmp, \tmp, \tmp2, lsl#16
        usub16 \a, \a, \tmp
.endm



// doublebarrett_fast need 2 constants
.macro doublebarrett_fast a, tmp, tmp2, q, barrettconst1, barrettconst2
        smlawb \tmp, \barrettconst1, \a, \barrettconst2
        smlabt \tmp, \q, \tmp, \a
        smlawt \tmp2, \barrettconst1, \a, \barrettconst2
        smulbt \tmp2, \q, \tmp2
        add    \tmp2, \a, \tmp2, lsl#16
pkhbt  \a, \tmp, \tmp2
.endm

.macro montgomery q, qinv, a, tmp
        smulbt \tmp, \a, \qinv //-q^(-1)
        smlabb \tmp, \q, \tmp, \a
.endm
.macro doublemontgomery a, tmp, tmp2, q, qinv, montconst
        smulbb \tmp2, \a, \montconst
        montgomery \q, \qinv, \tmp2, \tmp
        smultb \a, \a, \montconst
        montgomery \q, \qinv, \a, \tmp2
        pkhtb \a, \tmp2, \tmp, asr#16
.endm

.macro doubleplant a, tmp, q, qa, plantconst
        smulwb \tmp, \plantconst, \a
        smulwt \a, \plantconst, \a
        smlabt \tmp, \tmp, \q, \qa
        smlabt \a, \a, \q, \qa
        pkhtb \a, \a, \tmp, asr#16
.endm

.macro plant_red q, qa, qinv, tmp
        mul \tmp, \tmp, \qinv
        smlatt \tmp, \tmp, \q, \qa
.endm





#endif /* MACROS_I */
