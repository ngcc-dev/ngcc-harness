#ifndef MACROS_I
#define MACROS_I

// general macros
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
// (-(q+1)/2,q/2)
.macro doubleplant a, tmp, q, qa, plantconst
  smulwb \tmp, \plantconst, \a
  smulwt \a, \plantconst, \a
  smlabt \tmp, \tmp, \q, \qa
  smlabt \a, \a, \q, \qa
  pkhtb \a, \a, \tmp, asr#16
.endm

//[0,q)?
.macro doubleplant_mq a, tmp, q, qa, plantconst, tmp2
  smulwb \tmp, \plantconst, \a
  smulwt \a, \plantconst, \a
  smlabt \tmp, \tmp, \q, \qa
  and \tmp2, \q, \tmp, asr #31
  sadd16 \tmp, \tmp, \tmp2  
  smlabt \a, \a, \q, \qa
  and \tmp2, \q, \a, asr #31
  sadd16 \a, \a, \tmp2
  pkhtb \a, \a, \tmp, asr#16
.endm

.macro doublebarrett a, tmp, tmp2, q, barrettconst
  smulbb \tmp, \a, \barrettconst
  smultb \tmp2, \a, \barrettconst
  asr \tmp, \tmp, #26
  asr \tmp2, \tmp2, #26
  smulbb \tmp, \tmp, \q
  smulbb \tmp2, \tmp2, \q
  pkhbt \tmp, \tmp, \tmp2, lsl#16
  usub16 \a, \a, \tmp
.endm

// q locate in the top half of the register
.macro plant_red q, qa, qinv, tmp
  mul \tmp, \tmp, \qinv     
  //tmp*qinv mod 2^2n/ 2^n; in high half
  smlatt \tmp, \tmp, \q, \qa
  // result in high half
.endm

// res= a*b mod q.
.macro plant_mul q, qa, qinv, pos1, pos2, a, b, res
  smul\pos1\pos2 \res, \a, \b
  mul \res, \res, \qinv
  //res*a*qinv mod 2^2n/ 2^n; in
  smlatt \res, \res, \q, \qa
  // result in high half
.endm

.macro mul_twiddle_plant a, twiddle, tmp, q, qa
	smulwb \tmp, \twiddle, \a
	smulwt \a,   \twiddle, \a
	smlabt \tmp, \tmp, \q, \qa
	smlabt \a, \a, \q, \qa
	pkhtb \a, \a, \tmp, asr#16
.endm

.macro doublebutterfly_plant a0, a1, twiddle, tmp, q, qa
	smulwb \tmp, \twiddle, \a1
	smulwt \a1, \twiddle, \a1
	smlabt \tmp, \tmp, \q, \qa
	smlabt \a1, \a1, \q, \qa
	pkhtb \tmp, \a1, \tmp, asr#16
	usub16 \a1, \a0, \tmp
	uadd16 \a0, \a0, \tmp
.endm

.macro two_doublebutterfly_plant a0, a1, a2, a3, twiddle0, twiddle1, tmp, q, qa
	doublebutterfly_plant \a0, \a1, \twiddle0, \tmp, \q, \qa
	doublebutterfly_plant \a2, \a3, \twiddle1, \tmp, \q, \qa
.endm

// [-(q+1)/2,q/2)
.macro fullplant a0, a1, a2, a3, a4, a5, a6, a7, tmp, q, qa, plantconst
	movw \plantconst, #14574
	movt \plantconst, #85
	doubleplant \a0, \tmp, \q, \qa, \plantconst
	doubleplant \a1, \tmp, \q, \qa, \plantconst
	doubleplant \a2, \tmp, \q, \qa, \plantconst
	doubleplant \a3, \tmp, \q, \qa, \plantconst
	doubleplant \a4, \tmp, \q, \qa, \plantconst
	doubleplant \a5, \tmp, \q, \qa, \plantconst
	doubleplant \a6, \tmp, \q, \qa, \plantconst
	doubleplant \a7, \tmp, \q, \qa, \plantconst
.endm

// [0,q)
.macro fullplant_mq a0, a1, a2, a3, a4, a5, a6, a7, tmp, q, qa, plantconst, tmp2
	movw \plantconst, #14574
	movt \plantconst, #85 //-2^32*qinv mod 2^32
	doubleplant_mq \a0, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a1, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a2, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a3, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a4, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a5, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a6, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a7, \tmp, \q, \qa, \plantconst, \tmp2
.endm

// Input lanes are signed 16-bit values in (-(q+1)/2, q/2).
// Packed analogue of: a += (a >> 15) & q
// q is expected in the top halfword of \q.
.macro cadd a, tmp, qtmp, q
	pkhtb \qtmp, \q, \q, asr #16
	sxth \tmp, \a
	and \tmp, \qtmp, \tmp, asr #31
	uxth \tmp, \tmp
	sxth \qtmp, \a, ror #16
	and \qtmp, \q, \qtmp, asr #31
	orr \tmp, \tmp, \qtmp
	uadd16 \a, \a, \tmp
.endm

#endif
