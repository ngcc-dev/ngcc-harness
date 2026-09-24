/******************************************************************************
 * Integrating the improved Plantard arithmetic into ML-KEM.
 *
 * Efficient Plantard arithmetic enables a faster ML-KEM implementation with the
 * same stack usage.
 *
 * See the paper at https://eprint.iacr.org/2022/956.pdf for more details.
 *
 * @author   Junhao Huang, BNU-HKBU United International College, Zhuhai, China
 *           jhhuang_nuaa@126.com
 *
 * @date     September 2022
 ******************************************************************************/
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
// [-(1+q)/2,q/2)
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

// [-(1+q)/2,q/2)
.macro fullplant a0, a1, a2, a3, a4, a5, a6, a7, tmp, q, qa, plantconst
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
	doubleplant_mq \a0, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a1, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a2, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a3, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a4, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a5, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a6, \tmp, \q, \qa, \plantconst, \tmp2
	doubleplant_mq \a7, \tmp, \q, \qa, \plantconst, \tmp2
.endm

// q locate in the top half of the register
.macro plant_red q, qa, qinv, tmp
	mul \tmp, \tmp, \qinv     
	//tmp*qinv mod 2^2n/ 2^n; in high half
	smlatt \tmp, \tmp, \q, \qa
	// result in high half
.endm



#endif /* MACROS_I */
