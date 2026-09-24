#if (DKE_MODE != 512) && defined(DKE_USE_CORTEX_M4_PLANTARD)   /* DKE config gating (auto-added) */
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

#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

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

.global asm_barrett_reduce
.type asm_barrett_reduce,%function
.align 2
asm_barrett_reduce:
	push    {r4-r11, r14}

	poly        .req r0
	poly0       .req r1
	poly1       .req r2
	poly2       .req r3
	poly3       .req r4
	poly4       .req r5
	poly5       .req r6
	poly6       .req r7
	poly7       .req r8
	loop        .req r9
	barrettconst .req r10
	q           .req r11
	tmp         .req r12
	tmp2        .req r14

	movw barrettconst, #20159
	movw q, #3329

	movw loop, #16
	1:
		ldm poly, {poly0-poly7}

		doublebarrett poly0, tmp, tmp2, q, barrettconst
		doublebarrett poly1, tmp, tmp2, q, barrettconst
		doublebarrett poly2, tmp, tmp2, q, barrettconst
		doublebarrett poly3, tmp, tmp2, q, barrettconst
		doublebarrett poly4, tmp, tmp2, q, barrettconst
		doublebarrett poly5, tmp, tmp2, q, barrettconst
		doublebarrett poly6, tmp, tmp2, q, barrettconst
		doublebarrett poly7, tmp, tmp2, q, barrettconst

		stm poly!, {poly0-poly7}

	subs.w loop, #1
	bne.w 1b

	.unreq poly        
	.unreq poly0       
	.unreq poly1       
	.unreq poly2       
	.unreq poly3       
	.unreq poly4       
	.unreq poly5       
	.unreq poly6       
	.unreq poly7       
	.unreq loop        
	.unreq barrettconst
	.unreq q           
	.unreq tmp         
	.unreq tmp2        

	pop     {r4-r11, pc}
	
// reduce(int16_t a[256])
// reduce to [0,q)
.align 2
.global poly_reduce_mq_asm
.type poly_reduce_mq_asm, %function
poly_reduce_mq_asm:
	push {r4-r11, lr}

	poly       .req r0
	poly0      .req r1
	poly1      .req r2
	poly2      .req r3
	poly3      .req r4
	poly4      .req r5
	poly5      .req r6
	poly6      .req r7
	poly7      .req r8
	q          .req r9
	qa         .req r10
	tmp        .req r11
	plantconst .req r12
	endptr     .req lr

	movt q, #3329
	movw qa, #26632
	movw plantconst, #44984
	movt plantconst, #19 //-2^32*qinv mod 2^32
	add.w endptr, poly, #256*2
1:
	ldr.w poly0, [poly, #4*0]
	ldr.w poly1, [poly, #4*1]
	ldr.w poly2, [poly, #4*2]
	ldr.w poly3, [poly, #4*3]
	ldr.w poly4, [poly, #4*4]
	ldr.w poly5, [poly, #4*5]
	ldr.w poly6, [poly, #4*6]
	ldr.w poly7, [poly, #4*7]
	
	vmov s0, poly
	fullplant_mq poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, tmp, q, qa, plantconst, poly
	vmov poly, s0

	store poly, poly4, poly5, poly6, poly7, #16, #20, #24, #28
	str.w poly1, [poly, #4]
	str.w poly2, [poly, #8]
	str.w poly3, [poly, #12]
	str.w poly0, [poly], #4*8

	cmp.w poly, endptr
	bne.w 1b

	pop {r4-r11, pc}

.unreq poly
.unreq poly0
.unreq poly1
.unreq poly2
.unreq poly3
.unreq poly4
.unreq poly5
.unreq poly6
.unreq poly7
.unreq q
.unreq qa
.unreq tmp
.unreq plantconst
.unreq endptr

.align 2
.global poly_reduce_asm
.type poly_reduce_asm, %function
poly_reduce_asm:
	push {r4-r11, lr}

	poly       .req r0
	poly0      .req r1
	poly1      .req r2
	poly2      .req r3
	poly3      .req r4
	poly4      .req r5
	poly5      .req r6
	poly6      .req r7
	poly7      .req r8
	q          .req r9
	qa         .req r10
	tmp        .req r11
	plantconst .req r12
	endptr     .req lr

	movt q, #3329
	movw qa, #26632
	movw plantconst, #44984
	movt plantconst, #19 //-2^32*qinv mod 2^32
	add.w endptr, poly, #256*2

1:
	ldr.w poly0, [poly, #4*0]
	ldr.w poly1, [poly, #4*1]
	ldr.w poly2, [poly, #4*2]
	ldr.w poly3, [poly, #4*3]
	ldr.w poly4, [poly, #4*4]
	ldr.w poly5, [poly, #4*5]
	ldr.w poly6, [poly, #4*6]
	ldr.w poly7, [poly, #4*7]
	
	fullplant poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, tmp, q, qa, plantconst

	store poly, poly4, poly5, poly6, poly7, #16, #20, #24, #28
	str.w poly1, [poly, #4]
	str.w poly2, [poly, #8]
	str.w poly3, [poly, #12]
	str.w poly0, [poly], #4*8

	cmp.w poly, endptr
	bne.w 1b

	pop {r4-r11, pc}

.unreq poly
.unreq poly0
.unreq poly1
.unreq poly2
.unreq poly3
.unreq poly4
.unreq poly5
.unreq poly6
.unreq poly7
.unreq q
.unreq qa
.unreq tmp
.unreq plantconst
.unreq endptr

.global asm_fromplant
.type asm_fromplant,%function
.align 2
asm_fromplant:
	push    {r4-r11, r14}

	poly       .req r0
	poly0      .req r1
	poly1      .req r2
	poly2      .req r3
	poly3      .req r4
	poly4      .req r5
	poly5      .req r6
	poly6      .req r7
	poly7      .req r8
	loop       .req r9
	plantconst .req r10
	q          .req r11
	qa         .req r12
	tmp        .req r14

	movw qa, #26632
	movt q, #3329

	// plant_constant = (Plant_const^2 mod 2^32) * p^-1 mod 2^32
	movw plantconst, #20396
	movt plantconst, #38900
	movw loop, #16
1:
	ldm poly, {poly0-poly7}

	doubleplant poly0, tmp, q, qa, plantconst
	doubleplant poly1, tmp, q, qa, plantconst
	doubleplant poly2, tmp, q, qa, plantconst
	doubleplant poly3, tmp, q, qa, plantconst
	doubleplant poly4, tmp, q, qa, plantconst
	doubleplant poly5, tmp, q, qa, plantconst
	doubleplant poly6, tmp, q, qa, plantconst
	doubleplant poly7, tmp, q, qa, plantconst

	stm poly!, {poly0-poly7}

	subs.w loop, #1
	bne.w 1b

	.unreq poly
	.unreq poly0
	.unreq poly1
	.unreq poly2
	.unreq poly3
	.unreq poly4
	.unreq poly5
	.unreq poly6
	.unreq poly7
	.unreq loop
	.unreq plantconst
	.unreq q
	.unreq qa
	.unreq tmp

	pop     {r4-r11, pc}

#endif /* (DKE_MODE != 512) && defined(DKE_USE_CORTEX_M4_PLANTARD) -- DKE config gating (auto-added) */
