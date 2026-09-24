#if (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD)   /* DKE config gating (auto-added) */
#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb


.global poly_reduce_mq_asm
.type poly_reduce_mq_asm,%function
.align 2
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

	movt q, #7681
	movw qa, #30724
	movw plantconst, #34880
	movt plantconst, #8 // (-2^32 mod q) * (q^-1 mod 2^32) mod 2^32
	add.w endptr, poly, #512*2
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

.global poly_reduce_asm
.type poly_reduce_asm,%function
.align 2
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

	movt q, #7681
	movw qa, #30724
	movw plantconst, #34880
	movt plantconst, #8 // (-2^32 mod q) * (q^-1 mod 2^32) mod 2^32
	add.w endptr, poly, #512*2

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

	movt q, #7681
	movw qa, #30724

	// plant_constant = (Plant_const^2 mod 2^32) * p^-1 mod 2^32
	movw plantconst, #3516      // 0x0dbc
	movt plantconst, #18020     // 0x4664

	movw loop, #32
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

#endif /* (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD) -- DKE config gating (auto-added) */
