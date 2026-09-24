#if (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD)   /* DKE config gating (auto-added) */
/******************************************************************************
* Integrating the improved Plantard arithmetic into NTTRU.
*
* Efficient Plantard arithmetic enables a faster NTTRU implementation with the 
* same stack usage.
*
* See the paper at https://eprint.iacr.org/2022/956.pdf for more details.
*
* @author   Junhao Huang, BNU-HKBU United International College, Zhuhai, China
*           jhhuang_nuaa@126.com
*
* @date     September 2022
******************************************************************************/

#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

.macro fullplant_4 a0, a1, a2, a3, tmp, q, qa, plantconst
	doubleplant \a0, \tmp, \q, \qa, \plantconst
	doubleplant \a1, \tmp, \q, \qa, \plantconst
	doubleplant \a2, \tmp, \q, \qa, \plantconst
	doubleplant \a3, \tmp, \q, \qa, \plantconst
.endm

.macro doubleinvbutterfly_plant a0, a1, twiddle, tmp, q, qa
	usub16 \tmp, \a0, \a1
	uadd16 \a0, \a0, \a1

	smulwb \a1, \twiddle, \tmp
	smulwt \tmp, \twiddle, \tmp
	smlabt \a1, \a1, \q, \qa
	smlabt \tmp, \tmp, \q, \qa
	pkhtb \a1, \tmp, \a1, asr#16
.endm

.macro two_doubleinvbutterfly_plant a0, a1, a2, a3, twiddle1, twiddle2, tmp, q, qa
	doubleinvbutterfly_plant \a0, \a1, \twiddle1, \tmp, \q, \qa
	doubleinvbutterfly_plant \a2, \a3, \twiddle2, \tmp, \q, \qa
.endm

// input: 2q
.macro _3_layer_double_GS_16_plant_fp_last_layer c0, c1, c2, c3, c4, c5, c6, c7, xi1, xi2, xi3, xi4, xi5, xi6, xi7, xi8, twiddle1, twiddle2, q, qa, tmp
	// layer 1 
	vmov \twiddle1, \xi1
	vmov \twiddle2, \xi2
	two_doubleinvbutterfly_plant \c0, \c1, \c2, \c3, \twiddle1, \twiddle2, \tmp, \q, \qa

	vmov \twiddle1, \xi3
	vmov \twiddle2, \xi4
	two_doubleinvbutterfly_plant \c4, \c5, \c6, \c7, \twiddle1, \twiddle2, \tmp, \q, \qa

	movw \twiddle1, #34880
	movt \twiddle1, #8
	fullplant_4 \c0, \c2, \c4, \c6, \tmp, \q, \qa, \twiddle1

	// layer 2
	vmov \twiddle1, \xi5
	vmov \twiddle2, \xi6
	two_doubleinvbutterfly_plant \c0, \c2, \c1, \c3, \twiddle1, \twiddle1, \tmp, \q, \qa

	two_doubleinvbutterfly_plant \c4, \c6, \c5, \c7, \twiddle2, \twiddle2, \tmp, \q, \qa

	// layer 3
	vmov \twiddle1, \xi7
	vmov \twiddle2, \xi8
	two_doubleinvbutterfly_plant \c0, \c4, \c1, \c5, \twiddle1, \twiddle1, \tmp, \q, \qa
	two_doubleinvbutterfly_plant \c2, \c6, \c3, \c7, \twiddle1, \twiddle1, \tmp, \q, \qa

	doubleplant \c0, \tmp, \q, qa, \twiddle2
	doubleplant \c1, \tmp, \q, qa, \twiddle2
	doubleplant \c2, \tmp, \q, qa, \twiddle2
	doubleplant \c3, \tmp, \q, qa, \twiddle2
.endm

// input: <1q;  output: <1q; modular reduction after the second layer
.macro _3_layer_double_GS_16_plant_fp_reduce2 c0, c1, c2, c3, c4, c5, c6, c7, xi1, xi2, xi3, xi4, xi5, xi6, xi7, twiddle1, twiddle2, q, qa, tmp
	vmov \twiddle1, \xi1
	vmov \twiddle2, \xi2
	two_doubleinvbutterfly_plant \c0, \c1, \c2, \c3, \twiddle1, \twiddle2, \tmp, \q, \qa
	// c0, c2: 2q; c1, c3: 0.5q
	vmov \twiddle1, \xi3
	vmov \twiddle2, \xi4
	two_doubleinvbutterfly_plant \c4, \c5, \c6, \c7, \twiddle1, \twiddle2, \tmp, \q, \qa
	// c4, c6: 2q; c5, c7: 0.5q
	// layer 2
	vmov \twiddle1, \xi5
	vmov \twiddle2, \xi6
	two_doubleinvbutterfly_plant \c0, \c2, \c1, \c3, \twiddle1, \twiddle1, \tmp, \q, \qa

	two_doubleinvbutterfly_plant \c4, \c6, \c5, \c7, \twiddle2, \twiddle2, \tmp, \q, \qa
	//c0, c1, c4, c5: 4q

	movw \twiddle1, #34880
	movt \twiddle1, #8
	fullplant_4 \c0, \c1, \c4, \c5, \tmp, \q, \qa, \twiddle1

	// layer 3
	vmov \twiddle1, \xi7
	two_doubleinvbutterfly_plant \c0, \c4, \c1, \c5, \twiddle1, \twiddle1, \tmp, \q, \qa
	two_doubleinvbutterfly_plant \c2, \c6, \c3, \c7, \twiddle1, \twiddle1, \tmp, \q, \qa
.endm

//input: <0.5q; output: <0.5q;  modular reduction after the third layer
.macro _3_layer_double_GS_16_plant_fp_reduce3 c0, c1, c2, c3, c4, c5, c6, c7, xi1, xi2, xi3, xi4, xi5, xi6, xi7, twiddle1, twiddle2, q, qa, tmp
	vmov \twiddle1, \xi1
	vmov \twiddle2, \xi2
	two_doubleinvbutterfly_plant \c0, \c1, \c2, \c3, \twiddle1, \twiddle2, \tmp, \q, \qa

	vmov \twiddle1, \xi3
	vmov \twiddle2, \xi4
	two_doubleinvbutterfly_plant \c4, \c5, \c6, \c7, \twiddle1, \twiddle2, \tmp, \q, \qa

	// layer 2
	vmov \twiddle1, \xi5
	vmov \twiddle2, \xi6
	two_doubleinvbutterfly_plant \c0, \c2, \c1, \c3, \twiddle1, \twiddle1, \tmp, \q, \qa

	two_doubleinvbutterfly_plant \c4, \c6, \c5, \c7, \twiddle2, \twiddle2, \tmp, \q, \qa

	// layer 3
	vmov \twiddle1, \xi7
	two_doubleinvbutterfly_plant \c0, \c4, \c1, \c5, \twiddle1, \twiddle1, \tmp, \q, \qa
	two_doubleinvbutterfly_plant \c2, \c6, \c3, \c7, \twiddle1, \twiddle1, \tmp, \q, \qa
	
	movw \twiddle1, #34880
	movt \twiddle1, #8
	fullplant_4 \c0, \c1, \c2, \c3, \tmp, \q, \qa, \twiddle1
.endm


//input: -0.5q-0.5q
.global invntt_fast
.type invntt_fast, %function
.align 2
invntt_fast:
	push {r4-r11, lr}
	vpush.w {s16-s26}
	poly        .req r0
	twiddle_ptr .req r1
	poly0       .req r2
	poly1       .req r3
	poly2       .req r4
	poly3       .req r5
	poly4       .req r6
	poly5       .req r7
	poly6       .req r8
	poly7       .req r9
	twiddle1    .req r10
	twiddle2    .req r11
	// plantconst  .req r10
	### plantconst=((-2^2n mod M)*qinv mod 2^2n) mod 2^2n
	q           .req r12
	### at the top of r12
	qa          .req r1
	### qa=2^a q;a=2; at the bottom of r1
	tmp         .req r14

	movt q, #7681

	.equ distance, 32
	.equ offset, 4
	.equ strincr, 64

	add.w tmp, poly, #strincr*16
	vmov s24, tmp
	### LAYER 0+1+2+3
	1:
		vldm twiddle_ptr!, {s8-s22}
		vmov s23, twiddle_ptr
		movw qa, #30724

		load poly, poly2, poly0, poly3, poly1, #0, #offset, #distance/4, #distance/4+offset

		vmov twiddle1, s8
		vmov twiddle2, s9
		two_doubleinvbutterfly_plant poly2, poly0, poly3, poly1, twiddle1, twiddle2, tmp, q, qa
		vmov s0, poly2
		vmov s1, poly3

		load poly, poly4, poly2, poly5, poly3, #2*distance/4, #2*distance/4+offset, #3*distance/4, #3*distance/4+offset
		
		vmov twiddle1, s10
		vmov twiddle2, s11
		two_doubleinvbutterfly_plant poly4, poly2, poly5, poly3, twiddle1, twiddle2, tmp, q, qa
		vmov s2, poly4
		vmov s3, poly5

		load poly, poly6, poly4, poly7, poly5, #distance, #distance+offset, #5*distance/4, #5*distance/4+offset
		vmov twiddle1, s12
		vmov twiddle2, s13
		two_doubleinvbutterfly_plant poly6, poly4, poly7, poly5, twiddle1, twiddle2, tmp, q, qa
		vmov s4, poly6
		vmov s5, poly7

		ldr.w poly7, [poly, #6*distance/4]
		ldr.w poly6, [poly, #6*distance/4+offset]
		vmov twiddle1, s14
		doubleinvbutterfly_plant poly7, poly6, twiddle1, tmp, q, qa
		vmov s6, poly7

		ldr.w twiddle2, [poly, #7*distance/4]
		ldr.w poly7, [poly, #7*distance/4+offset]
		vmov twiddle1, s15
		doubleinvbutterfly_plant twiddle2, poly7, twiddle1, tmp, q, qa
		vmov s7, twiddle2
		
		// poly0-poly7: 0.5q
		_3_layer_double_GS_16_plant_fp_reduce3 poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s16, s17, s18, s19, s20, s21, s22, twiddle1, twiddle2, q, qa, tmp

		store poly, poly0, poly1, poly2, poly3, #offset, #distance/4+offset, #2*distance/4+offset, #3*distance/4+offset
		store poly, poly4, poly5, poly6, poly7, #distance+offset, #5*distance/4+offset, #6*distance/4+offset, #7*distance/4+offset

		vmov poly0, s0
		vmov poly1, s1
		vmov poly2, s2
		vmov poly3, s3
		vmov poly4, s4
		vmov poly5, s5
		vmov poly6, s6
		vmov poly7, s7

		// poly0-poly7: 1q
		_3_layer_double_GS_16_plant_fp_reduce2 poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s16, s17, s18, s19, s20, s21, s22, twiddle1, twiddle2, q, qa, tmp

		store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
		str.w poly1, [poly, #distance/4]
		str.w poly2, [poly, #2*distance/4]
		str.w poly3, [poly, #3*distance/4]
		str.w poly0, [poly], #strincr

		vmov twiddle_ptr, s23
		vmov tmp, s24
		cmp.w poly, tmp
	bne.w 1b
	sub.w poly, #strincr*16

	.equ distance, 512
	.equ offset, 64
	.equ strincr, 4 //4*16=64
	// pre-load 16 twiddle factors to 16 FPU registers
	// s0-s7 used to temporary store 16 16-bit polys.
	vldm twiddle_ptr!, {s8-s23}
	movw qa, #30724

	add.w tmp, poly, #strincr*16
	### LAYER 4+5+6+7
	vmov s26, tmp
	2:
		load poly, poly2, poly0, poly3, poly1, #0, #offset, #distance/4, #distance/4+offset
		
		vmov twiddle1, s8
		vmov twiddle2, s9
		two_doubleinvbutterfly_plant poly2, poly0, poly3, poly1, twiddle1, twiddle2, tmp, q, qa
		vmov s0, poly2
		vmov s1, poly3

		load poly, poly4, poly2, poly5, poly3, #2*distance/4, #2*distance/4+offset, #3*distance/4, #3*distance/4+offset
		
		vmov twiddle1, s10
		vmov twiddle2, s11
		two_doubleinvbutterfly_plant poly4, poly2, poly5, poly3, twiddle1, twiddle2, tmp, q, qa
		vmov s2, poly4
		vmov s3, poly5

		load poly, poly6, poly4, poly7, poly5, #distance, #distance+offset, #5*distance/4, #5*distance/4+offset
		vmov twiddle1, s12
		vmov twiddle2, s13
		two_doubleinvbutterfly_plant poly6, poly4, poly7, poly5, twiddle1, twiddle2, tmp, q, qa
		vmov s4, poly6
		vmov s5, poly7

		ldr.w poly7, [poly, #6*distance/4]
		ldr.w poly6, [poly, #6*distance/4+offset]
		vmov twiddle1, s14
		doubleinvbutterfly_plant poly7, poly6, twiddle1, tmp, q, qa
		vmov s6, poly7

		ldr.w twiddle2, [poly, #7*distance/4]
		ldr.w poly7, [poly, #7*distance/4+offset]
		vmov twiddle1, s15
		doubleinvbutterfly_plant twiddle2, poly7, twiddle1, tmp, q, qa
		vmov s7, twiddle2
		
		_3_layer_double_GS_16_plant_fp_last_layer poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s16, s17, s18, s19, s20, s21, s22, s23, twiddle1, twiddle2, q, qa, tmp

		store poly, poly0, poly1, poly2, poly3, #offset, #distance/4+offset, #2*distance/4+offset, #3*distance/4+offset
		store poly, poly4, poly5, poly6, poly7, #distance+offset, #5*distance/4+offset, #6*distance/4+offset, #7*distance/4+offset

		vmov poly0, s0
		vmov poly1, s1
		vmov poly2, s2
		vmov poly3, s3
		vmov poly4, s4
		vmov poly5, s5
		vmov poly6, s6
		vmov poly7, s7

		_3_layer_double_GS_16_plant_fp_last_layer poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s16, s17, s18, s19, s20, s21, s22, s23, twiddle1, twiddle2, q, qa, tmp

		store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
		str.w poly1, [poly, #distance/4]
		str.w poly2, [poly, #2*distance/4]
		str.w poly3, [poly, #3*distance/4]
		str.w poly0, [poly], #4

		vmov tmp, s26
		cmp.w poly, tmp
	bne.w 2b

	vpop.w {s16-s26}
	pop {r4-r11, pc}

#endif /* (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD) -- DKE config gating (auto-added) */
