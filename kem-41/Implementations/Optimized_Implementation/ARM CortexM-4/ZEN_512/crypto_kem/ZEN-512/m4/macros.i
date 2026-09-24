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

// res = a*b mod q, with res kept in the high halfword Plantard form.
// a and b are signed 16-bit values in the low halfword.
.macro plant_mul_hh q, qa, qinv, a, b, res
  smulbb \res, \a, \b
  plant_red \q, \qa, \qinv, \res
.endm

// a = a*zeta mod q, where a is a high-half Plantard coefficient and zeta is
// the 32-bit precomputed word from zetas_769.
.macro mul_zeta_hh a, zeta, q, qa
  smulwt \a, \zeta, \a
  smlabt \a, \a, \q, \qa
.endm

// Compute a 4x4 product into seven packed signed 16-bit high-half Plantard
// coefficients.
// The dataflow mirrors karatsuba_mul4() in ntt.c.  roff/aoff/boff are byte
// offsets; out stores int16_t coefficients.
.macro karatsuba_mul4_plant_hh out, roff, a, aoff, b, boff, q, qa, qinv, t0, t1, t2, t3, t4
  ldr.w \t0, [\a, #(\aoff + 0)]
  ldr.w \t1, [\b, #(\boff + 0)]
  smulbb \t2, \t0, \t1
  vmov s0, \t2 // p0[0] in s0
  plant_red \q, \qa, \qinv, \t2
  smuadx \t3, \t0, \t1
  vmov s1, \t3 // p0[1] in s1
  plant_red \q, \qa, \qinv, \t3
  pkhtb \t4, \t3, \t2, asr#16
  str.w \t4, [\out, #(\roff + 0)] //r0||r1
  smultt \t2, \t0, \t1 //a1*b1
  vmov s2, \t2 // p0[2] in s2

  ldr.w \t2, [\a, #(\aoff + 4)]
  ldr.w \t3, [\b, #(\boff + 4)]
  smulbb \t4, \t2, \t3
  vmov s3, \t4 // p1[0] in s3
  smuadx \t4, \t2, \t3
  vmov s4, \t4 // p1[1] in s4
  plant_red \q, \qa, \qinv, \t4
  vmov s6, \t4 // reduced p1[1]

  smultt \t4, \t2, \t3 // a3*b3
  vmov s5, \t4 // p1[2] in s5
  plant_red \q, \qa, \qinv, \t4

  // middle product
  sadd16 \t0, \t0, \t2 //a0+a2
  sadd16 \t1, \t1, \t3 //a1+a3
  vmov \t2, s6 // reduced p1[1]
  pkhtb \t2, \t4, \t2, asr#16
  strh.w \t2, [\out, #(\roff + 10)] //r5
  asr.w \t2, \t2, #16
  strh.w \t2, [\out, #(\roff + 12)] //r6

  vmov \t2, s2 //p0[2]
  vmov \t3, s0 //p0[0]
  sub.w \t2, \t2, \t3 //p0[2]-p0[0]
  vmov \t3, s3 //p1[0]
  sub.w \t2, \t2, \t3 //p0[2]-p0[0]-p1[0]
  smlabb \t2, \t0, \t1, \t2 // (a0+a2)*(b0+b2) + p0[2]-p0[0]-p1[0]
  plant_red \q, \qa, \qinv, \t2

  vmov \t3, s1 //p0[1]
  vmov \t4, s4 //p1[1]
  add.w \t3, \t3, \t4 //p0[1]+p1[1]
  neg.w \t3, \t3 //-(p0[1]+p1[1])
  smladx \t3, \t0, \t1, \t3 // (a0+a2)*(b1+b3) + (a1+a3)*(b0+b2) - (p0[1]+p1[1])
  plant_red \q, \qa, \qinv, \t3
  pkhtb \t4, \t3, \t2, asr#16
  str.w \t4, [\out, #(\roff + 4)] //r2||r3

  vmov \t2, s3 //p1[0]
  vmov \t3, s5 //p1[2]
  sub.w \t2, \t2, \t3 //p1[0]-p1[2]
  vmov \t3, s2 //p0[2]
  sub.w \t2, \t2, \t3 //p1[0]-p1[2]-p0[2]
  smlatt \t2, \t0, \t1, \t2 // (a1+a3)*(b1+b3) + p1[0]-p1[2]-p0[2]
  plant_red \q, \qa, \qinv, \t2
  asr.w \t2, \t2, #16
  strh.w \t2, [\out, #(\roff + 8)] //r4
.endm

.macro mul8_recompose_pair01 out, tmp, t0, t1, t2, t3, t4
  ldr.w \t0, [\out, #(4 * 2)]
  ldr.w \t1, [\out, #(8 * 2)]
  ldr.w \t2, [\tmp, #(0 * 2)]
  sadd16 \t2, \t2, \t0
  ldr.w \t3, [\out, #(0 * 2)]
  ssub16 \t2, \t2, \t3
  ssub16 \t2, \t2, \t1
  ldr.w \t3, [\tmp, #(4 * 2)]
  sadd16 \t3, \t3, \t1
  ssub16 \t3, \t3, \t0
  ldr.w \t4, [\out, #(12 * 2)]
  ssub16 \t3, \t3, \t4
  str.w \t2, [\out, #(4 * 2)]
  str.w \t3, [\out, #(8 * 2)]
.endm

.macro mul8_recompose_tail out, tmp, t0, t1, t2, t3
  ldrsh.w \t0, [\out, #(6 * 2)]   // p0[6]
  ldrsh.w \t1, [\out, #(10 * 2)]  // p1[2]
  ldrsh.w \t2, [\tmp, #(2 * 2)]   // pm[2]
  add.w \t2, \t2, \t0
  ldrsh.w \t3, [\out, #(2 * 2)]   // p0[2]
  sub.w \t2, \t2, \t3
  sub.w \t2, \t2, \t1
  strh.w \t2, [\out, #(6 * 2)]

  ldrsh.w \t2, [\tmp, #(6 * 2)]   // pm[6]
  add.w \t2, \t2, \t1
  sub.w \t2, \t2, \t0
  ldrsh.w \t3, [\out, #(14 * 2)]  // p1[6]
  sub.w \t2, \t2, \t3
  strh.w \t2, [\out, #(10 * 2)]
.endm

// Compute an 8x8 product into fifteen packed signed 16-bit high-half Plantard
// coefficients using three karatsuba_mul4_plant_hh calls.
.macro mul8_plant_hh out, a, b, tmp, as, bs, q, qa, qinv, t0, t1, t2, t3, t4
  karatsuba_mul4_plant_hh \out, 0, \a, 0, \b, 0, \q, \qa, \qinv, \t0, \t1, \t2, \t3, \t4
  karatsuba_mul4_plant_hh \out, 16, \a, 8, \b, 8, \q, \qa, \qinv, \t0, \t1, \t2, \t3, \t4

  ldr.w \t0, [\a, #0+4*0]
  ldr.w \t1, [\a, #8+4*0]
  sadd16 \t0, \t0, \t1
  str.w \t0, [\as, #0]
  ldr.w \t0, [\b, #0+4*0]
  ldr.w \t1, [\b, #8+4*0]
  sadd16 \t0, \t0, \t1
  str.w \t0, [\bs, #0]

  ldr.w \t0, [\a, #0+4*1]
  ldr.w \t1, [\a, #8+4*1]
  sadd16 \t0, \t0, \t1
  str.w \t0, [\as, #4]
  ldr.w \t0, [\b, #0+4*1]
  ldr.w \t1, [\b, #8+4*1]
  sadd16 \t0, \t0, \t1
  str.w \t0, [\bs, #4]

  karatsuba_mul4_plant_hh \tmp, 0, \as, 0, \bs, 0, \q, \qa, \qinv, \t0, \t1, \t2, \t3, \t4

  mul8_recompose_pair01 \out, \tmp, \t0, \t1, \t2, \t3, \t4
  mul8_recompose_tail \out, \tmp, \t0, \t1, \t2, \t3

  ldrsh.w \t0, [\tmp, #(3 * 2)]
  ldrsh.w \t1, [\out, #(3 * 2)]
  sub.w \t0, \t0, \t1
  ldrsh.w \t1, [\out, #(11 * 2)]
  sub.w \t0, \t0, \t1
  strh.w \t0, [\out, #(7 * 2)]
.endm

.macro mul_twiddle_plant a, twiddle, tmp, q, qa
	smulwb \tmp, \twiddle, \a
	smulwt \a,   \twiddle, \a
	smlabt \tmp, \tmp, \q, \qa
	smlabt \a, \a, \q, \qa
	pkhtb \a, \a, \tmp, asr#16
.endm

.macro mul_twiddle_plant_mq a, twiddle, tmp, q, qa, tmp2
	smulwb \tmp, \twiddle, \a
	smulwt \a,   \twiddle, \a
	smlabt \tmp, \tmp, \q, \qa
  and \tmp2, \q, \tmp, asr #31
  sadd16 \tmp, \tmp, \tmp2  
	smlabt \a, \a, \q, \qa
  and \tmp2, \q, \a, asr #31
  sadd16 \a, \a, \tmp2  
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
