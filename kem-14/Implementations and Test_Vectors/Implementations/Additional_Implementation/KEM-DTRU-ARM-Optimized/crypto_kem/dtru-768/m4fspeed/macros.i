#ifndef MACROS_I
#define MACROS_I

.macro load a, a0, a1, a2, a3, mem0, mem1, mem2, mem3
  ldr.w \a0, [\a, \mem0]
  ldr.w \a1, [\a, \mem1]
  ldr.w \a2, [\a, \mem2]
  ldr.w \a3, [\a, \mem3]
.endm

.macro load6 a, a0, a1, a2, a3,a4,a5, mem0, mem1, mem2, mem3,mem4,mem5  
  ldr.w \a0, [\a, \mem0]
  ldr.w \a1, [\a, \mem1]
  ldr.w \a2, [\a, \mem2]
  ldr.w \a3, [\a, \mem3]
  ldr.w \a4, [\a, \mem4]
  ldr.w \a5, [\a, \mem5]
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
  asr \tmp, \tmp, #26
  asr \tmp2, \tmp2, #26
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

// .macro montgomery q, qinv, a, tmp
//   smulbt \tmp, \a, \qinv //q^(-1)
//   smulbb \tmp, \q, \tmp
//   sub \tmp, \a, \tmp
// .endm

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
	//tmp*qinv mod 2^2n/ 2^n; in high half
	smlatt \tmp, \tmp, \q, \qa
	// result in high half
.endm


// //need one more register, but we don't have such reg for barrett because of its frequent use
// .macro doublebarrett7681 a, tmp, tmp2, tmp3
//   and.w \tmp, \a, #0x1FFF1FFF
//   sbfx \tmp2, \a, #13, #3 // tmp2=a_b>>13
//   sbfx \tmp3, \a, #29, #3 // tmp3=a_t>>13
//   pkhbt \a, \tmp2, \tmp3, lsl#16
//   usub16 \a, \tmp, \a
//   lsl \tmp2, #9
//   pkhbt \tmp, \tmp2, \tmp3, lsl#25
//   uadd16 \a, \tmp, \a
// .endm

// .macro doublebarrett7681 a, tmp, tmp2
//   and.w \tmp, \a, #0x1FFF1FFF // tmp=t
//   sbfx \tmp2, \a, #13, #3 // tmp2=a_b>>13
//   asr \a, #29 // tmp3=a_t>>13
//   pkhbt \a, \tmp2, \a, lsl#16
//   usub16 \tmp, \tmp, \a
//   lsl \tmp2, #9
//   bfc \a, #0, #16 // clear bit
//   pkhbt \a, \tmp2, \a, lsl#9
//   uadd16 \a, \tmp, \a
// .endm

// .macro doublebarrett_init7681 a, tmp, tmp2, q, barrettconst
//   smulbb \tmp, \a, \barrettconst //17474
//   smultb \tmp2, \a, \barrettconst
//   asr \tmp, \tmp, #27
//   asr \tmp2, \tmp2, #27
//   smulbb \tmp, \tmp, \q
//   smulbb \tmp2, \tmp2, \q
//   pkhbt \tmp, \tmp, \tmp2, lsl#16
//   usub16 \a, \a, \tmp
// .endm



#endif /* MACROS_I */
