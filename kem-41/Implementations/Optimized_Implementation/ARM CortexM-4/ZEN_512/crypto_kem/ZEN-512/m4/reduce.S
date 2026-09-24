/**
 * Copyright (c) 2023 Junhao Huang (jhhuang_nuaa@126.com)
 *
 * Licensed under the Apache License, Version 2.0(the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

// reduce(int16_t* a, int len)
// reduce to [-(q+1)/2,q/2)
.align 2
.global poly_reduce_asm
.type poly_reduce_asm, %function
poly_reduce_asm:
	push {r4-r11, lr}

	poly       .req r0
	len        .req r1
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
	poly0      .req len

	movt q, #769
	movw qa, #24608
	add.w endptr, poly, len, lsl #1

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
.unreq len
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

// reduce(int16_t* a, int len)
// reduce to [0,q)
.align 2
.global poly_reduce_mq_asm
.type poly_reduce_mq_asm, %function
poly_reduce_mq_asm:
	push {r4-r11, lr}

	poly       .req r0
	len        .req r1
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
	poly0      .req len

	movt q, #769
	movw qa, #24608
	add.w endptr, poly, len, lsl #1

2:
	ldr.w poly0, [poly, #4*0]
	ldr.w poly1, [poly, #4*1]
	ldr.w poly2, [poly, #4*2]
	ldr.w poly3, [poly, #4*3]
	ldr.w poly4, [poly, #4*4]
	ldr.w poly5, [poly, #4*5]
	ldr.w poly6, [poly, #4*6]
	ldr.w poly7, [poly, #4*7]
	
	vmov.w s0, poly
	fullplant_mq poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, tmp, q, qa, plantconst, poly
	vmov.w poly, s0

	store poly, poly4, poly5, poly6, poly7, #16, #20, #24, #28
	str.w poly1, [poly, #4]
	str.w poly2, [poly, #8]
	str.w poly3, [poly, #12]
	str.w poly0, [poly], #4*8

	cmp.w poly, endptr
	bne.w 2b

	pop {r4-r11, pc}

.unreq poly
.unreq len
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
