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
// for DIM_N=512
###################################
#### small point-multiplication####
#### r0: out; r1: in1; r2: in2; r3: zetas####
## (a0,a1,a2,a3)*(b0,b1,b2,b3)-> (c0,c1,c2,c3)##
###################################
.align 2
.global basemul_asm
.type basemul_asm, %function
basemul_asm:
    push {r4-r11, lr}

	rptr    .req r0
	aptr    .req r1
	bptr    .req r2
	zetaptr .req r3
	poly0   .req r4
	poly1   .req r6
	poly2   .req r5
	poly3   .req r7
	q       .req r8
	qa      .req r14
	qinv    .req r9
	tmp     .req r10
	tmp2    .req r11
	zeta    .req r12
	loop    .req r14

    movt q, #769  // q
    movw qinv, #64769
	movt qinv, #58632 // qinv

    movw loop, #64
    1:
    vmov.w s0, loop
    movw qa, #24608 // qa
    ldr.w zeta, [zetaptr], #4

    .rept 2
    ldr.w poly0, [aptr], #4 // a0||a1
    ldr.w poly1, [aptr], #4 // a2||a3
    ldr.w poly2, [bptr], #4 // b0||b1
    ldr.w poly3, [bptr], #4 // b2||b3

    // c1= a0b1+a1b0+(a2b3+a3b2)zeta 
    smuadx tmp, poly0, poly2 // a0b1+a1b0
    plant_red q, qa, qinv, tmp // res in high-half
    smuadx tmp2, poly1, poly3 //a2b3+a3b2
    plant_red q, qa, qinv, tmp2 //high-half of tmp2 
    smulwt tmp2, zeta, tmp2 
    smlabt tmp2, tmp2, q, qa // (a2b3+a3b2)*zeta mod q; in high-half
    sadd16 tmp, tmp, tmp2 // c1 in high-half
    vmov s1, tmp //store c1 in s1
    
    //c3=a0b3+a1b2+a2b1+a3b0
    smuadx tmp, poly0, poly3 // a0b3+a1b2
    smladx tmp, poly1, poly2, tmp // a0b3+a1b2+a2b1+a3b0
    plant_red q, qa, qinv, tmp // c3 in high-half
    vmov s2, tmp // store c3 in s2
    
    // shuffle poly0-poly3
    pkhbt tmp, poly0, poly1, lsl #16 // a0||a2
    pkhtb poly0, poly1, poly0, asr #16 // a1||a3
    pkhbt poly1, poly2, poly3, lsl #16 // b0||b2
    pkhtb poly2, poly3, poly2, asr #16 // b1||b3

    // c2
    smulbb tmp2, poly0, poly2 //a1b1
    smladx tmp2, tmp, poly1, tmp2 //a0b2+a1b1+a2b0
    // plant_red q, qa, qinv, tmp2
    smulwt poly3, zeta, poly2 //b3zeta
    smlabt poly3, poly3, q, qa // b3zeta mod q
    smlatt poly3, poly0, poly3, tmp2 //a0b2+a1b1+a2b0+a3*b3zeta
    plant_red q, qa, qinv, poly3 

    // c0
    smultt tmp2, tmp, poly1 //a2b2
    smladx tmp2, poly0, poly2, tmp2 //a1b3+a2b2+a3b1; poly0 not used anymore
    plant_red q, qa, qinv, tmp2
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa //(a1b3+a2b2+a3b1)zeta mod q in high-half
    smulbb poly0, tmp, poly1
    plant_red q, qa, qinv, poly0
    sadd16 poly0, tmp2, poly0 //c0 in high half
    
    //combine
    vmov tmp, s1 //c1
    vmov tmp2, s2 // c3
    pkhtb poly0, tmp, poly0, asr #16 // c0||c1
    pkhtb poly3, tmp2, poly3, asr #16 //c2||c3
    str.w poly0, [rptr], #4
    str.w poly3, [rptr], #4
    neg zeta, zeta 
    .endr

    vmov.w loop, s0
    subs.w loop, #1
    bne.w 1b

    pop.w {r4-r11, pc}

// results multiplied by beta^2 *qinv mod q so that the results in normal domain instead of plantard domain. Output in [0,q).
.align 2
.global basemul_mq_asm
.type basemul_mq_asm, %function
basemul_mq_asm:
    push {r4-r11, lr}

	rptr    .req r0
	aptr    .req r1
	bptr    .req r2
	zetaptr .req r3
	poly0   .req r4
	poly1   .req r6
	poly2   .req r5
	poly3   .req r7
	q       .req r8
	qa      .req r14
	qinv    .req r9
	tmp     .req r10
	tmp2    .req r11
	zeta    .req r12
	loop    .req r14

    movt q, #769  // q
    movw qinv, #64769
	movt qinv, #58632 // qinv

    movw loop, #64
    1:
    vmov.w s0, loop
    movw qa, #24608 // qa
    ldr.w zeta, [zetaptr], #4
    vmov.w s3, zetaptr

    movw zetaptr, #50793
    movt zetaptr, #63916
    // multiplied by (-2^32)^2*qinv to recover in normal domain

    .rept 2
    ldr.w poly0, [aptr], #4 // a0||a1
    ldr.w poly1, [aptr], #4 // a2||a3
    ldr.w poly2, [bptr], #4 // b0||b1
    ldr.w poly3, [bptr], #4 // b2||b3

    // c1= a0b1+a1b0+(a2b3+a3b2)zeta 
    smuadx tmp, poly0, poly2 // a0b1+a1b0
    plant_red q, qa, qinv, tmp // res in high-half
    smuadx tmp2, poly1, poly3 //a2b3+a3b2
    plant_red q, qa, qinv, tmp2 //high-half of tmp2 
    smulwt tmp2, zeta, tmp2 
    smlabt tmp2, tmp2, q, qa // (a2b3+a3b2)*zeta mod q; in high-half
    sadd16 tmp, tmp, tmp2 // c1 in high-half
    smulwt tmp, zetaptr, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2  
    vmov s1, tmp //store c1 in s1
    
    //c3=a0b3+a1b2+a2b1+a3b0
    smuadx tmp, poly0, poly3 // a0b3+a1b2
    smladx tmp, poly1, poly2, tmp // a0b3+a1b2+a2b1+a3b0
    plant_red q, qa, qinv, tmp // c3 in high-half
    smulwt tmp, zetaptr, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2  
    vmov s2, tmp // store c3 in s2
    
    // shuffle poly0-poly3
    pkhbt tmp, poly0, poly1, lsl #16 // a0||a2
    pkhtb poly0, poly1, poly0, asr #16 // a1||a3
    pkhbt poly1, poly2, poly3, lsl #16 // b0||b2
    pkhtb poly2, poly3, poly2, asr #16 // b1||b3

    // c2
    smulbb tmp2, poly0, poly2 //a1b1
    smladx tmp2, tmp, poly1, tmp2 //a0b2+a1b1+a2b0
    // plant_red q, qa, qinv, tmp2
    smulwt poly3, zeta, poly2 //b3zeta
    smlabt poly3, poly3, q, qa // b3zeta mod q
    smlatt poly3, poly0, poly3, tmp2 //a0b2+a1b1+a2b0+a3*b3zeta
    plant_red q, qa, qinv, poly3 
    smulwt poly3, zetaptr, poly3
    smlabt poly3, poly3, q, qa
    and tmp2, q, poly3, asr #31
    sadd16 poly3, poly3, tmp2  

    // c0
    smultt tmp2, tmp, poly1 //a2b2
    smladx tmp2, poly0, poly2, tmp2 //a1b3+a2b2+a3b1; poly0 not used anymore
    plant_red q, qa, qinv, tmp2
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa //(a1b3+a2b2+a3b1)zeta mod q in high-half
    smulbb poly0, tmp, poly1
    plant_red q, qa, qinv, poly0
    sadd16 poly0, tmp2, poly0 //c0 in high half
    smulwt poly0, zetaptr, poly0
    smlabt poly0, poly0, q, qa
    and tmp2, q, poly0, asr #31
    sadd16 poly0, poly0, tmp2  
    
    //combine
    vmov tmp, s1 //c1
    vmov tmp2, s2 // c3
    pkhtb poly0, tmp, poly0, asr #16 // c0||c1
    pkhtb poly3, tmp2, poly3, asr #16 //c2||c3
    
    str.w poly0, [rptr], #4
    str.w poly3, [rptr], #4
    neg zeta, zeta 
    .endr

    vmov.w zetaptr, s3
    vmov.w loop, s0
    subs.w loop, #1
    bne.w 1b

    pop.w {r4-r11, pc}

.unreq rptr
.unreq aptr
.unreq zetaptr
.unreq poly0
.unreq poly1
.unreq poly2
.unreq poly3
.unreq q
.unreq qa
.unreq qinv
.unreq tmp
.unreq tmp2
.unreq zeta
.unreq loop




###################################
#### small base_inv################
#### r0: out; r1: in; r2: zetas; r3: qinv_table####
## (a0,a1,a2,a3)^-1-> (c0,c1,c2,c3)##
###################################
.align 2
.global baseinv_asm
.type baseinv_asm, %function
baseinv_asm:
    push {r4-r11, lr}

	rptr    .req r0
	aptr    .req r1
	zetaptr .req r2
	tmp3    .req r3
	poly0   .req r4
	poly1   .req r6
	poly2   .req r5
	poly3   .req r7
	q       .req r8
	qa      .req r14
	qinv    .req r9
	tmp     .req r10
	tmp2    .req r11
	zeta    .req r12
	loop    .req r14

    movt q, #769  // q
    movw qinv, #64769
	movt qinv, #58632 // qinv
    vmov.w s7, tmp3 // qinv_table

    movw loop, #64
    1:
    vmov.w s0, loop
    movw qa, #24608 // qa
    ldr.w zeta, [zetaptr], #4
    vmov.w s1, zetaptr
    
    movw zetaptr, #14574
    movt zetaptr, #85 //-2^32*qinv to reduce det to (-q/2,q/2)
    
    .rept 2
    
    ldr.w poly0, [aptr], #4 // a0||a1
    ldr.w poly1, [aptr], #4 // a2||a3

    //poly0=a0  ||a1
    //poly1=a2  ||a3
    //poly2=a0^2||a1^2
    //poly3=a2^2||a3^2
    smulbb tmp, poly0, poly0
    plant_red q, qa, qinv, tmp //a0^2
    smultt tmp2, poly0, poly0
    plant_red q, qa, qinv, tmp2 //a1^2
    pkhtb poly2, tmp2, tmp, asr #16 //a0^2||a1^2
    smulbb tmp, poly1, poly1
    plant_red q, qa, qinv, tmp //a2^2
    smultt tmp2, poly1, poly1
    plant_red q, qa, qinv, tmp2 //a3^2
    pkhtb poly3, tmp2, tmp, asr #16 //a2^2||a3^2

    //r0=(a0(a2^2+2a1a3)-a1^2*a2)zeta-(a0^3 + a2*(a3^2 zeta^2))
    smultt tmp, poly0, poly1 //a1a3
    lsl tmp, tmp, #1 //2a1a3
    smlabb tmp, poly1, poly1, tmp //a2^2+2a1a3
    plant_red q, qa, qinv, tmp // in high-half
    smulbt tmp, poly0, tmp //a0(a2^2+2a1a3)

    smulbt tmp2, poly1, poly2 //a1^2*a2
    sub tmp, tmp, tmp2
    plant_red q, qa, qinv, tmp //a0(a2^2+2a1a3)-a1^2*a2
    smulwt tmp, zeta, tmp  
    smlabt tmp, tmp, q, qa //(a0(a2^2+2a1a3)-a1^2*a2)zeta in high-half

    smulwt tmp2, zeta, poly3 
    smlabt tmp2, tmp2, q, qa //a3^2 zeta mod q
    smulwt tmp2, zeta, tmp2  
    smlabt tmp2, tmp2, q, qa //a3^2 zeta^2 mod q 
    smulbt tmp2, poly1, tmp2 //a2*(a3^2 zeta^2 mod q)
    smlabb tmp2, poly0, poly2, tmp2 //a0^3 + a2*(a3^2 zeta^2 mod q)
    neg.w tmp2, tmp2 //-
    plant_red q, qa, qinv, tmp2 //-(a0^3 + a2*(a3^2 zeta^2 mod q)) mod q
    sadd16 tmp, tmp, tmp2 // r0 in high-half.
    vmov s2, tmp // r0 in s2

    //r1=(a2(a1a2-2a0a3)-a1^2*a3)zeta+a3^3*zeta^2+a0^2*a1
    smulbt tmp, poly0, poly1 //a0a3
    lsl tmp, tmp, #1 //2a0a3
    smultb tmp2, poly0, poly1 //a1a2
    sub tmp, tmp2, tmp //a1a2-2a0a3
    plant_red q, qa, qinv, tmp
    smulbt tmp, poly1, tmp //a2(a1a2-2a0a3)

    smultt tmp2, poly2, poly1 //a1^2*a3
    sub tmp, tmp, tmp2 //a2(a1a2-2a0a3)-a1^2*a3
    plant_red q, qa, qinv, tmp
    smulwt tmp, zeta, tmp // (a2(a1a2-2a0a3)-a1^2*a3)*zeta
    smlabt tmp, tmp, q, qa // (a2(a1a2-2a0a3)-a1^2*a3)*zeta mod q in high-half

    smulwt tmp2, zeta, poly1 //a3zeta
    smlabt tmp2, tmp2, q, qa 
    smulwt tmp2, zeta, tmp2 //a3zeta^2
    smlabt tmp2, tmp2, q, qa 
    smultt tmp2, tmp2, poly3 //a3^3 zeta^2

    smlabt tmp2, poly2, poly0, tmp2 //a0^2a1+a3^3 zeta^2
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2
    vmov s3, tmp // r1 in s3

    //r2=(a2(2a1a3-a2^2)-a0a3^2)zeta+a0^2*a2-a0a1^2
    smultt tmp, poly0, poly1  
    lsl tmp, tmp, #1 //2a1a3
    smulbb tmp2, poly1, poly1 //a2^2
    sub tmp, tmp, tmp2
    plant_red q, qa, qinv, tmp // (2a1a3-a2^2)
    smulbt tmp, poly1, tmp // a2(2a1a3-a2^2)

    smulbt tmp2, poly0, poly3 //a0a3^2
    sub tmp, tmp, tmp2 //a2(2a1a3-a2^2)-a0a3^2
    plant_red q, qa, qinv, tmp
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa //(a2(2a1a3-a2^2)-a0a3^2)zeta in high half

    smulbb tmp2, poly2, poly1 //a0^2a2
    smulbt tmp3, poly0, poly2 //a0a1^2
    sub tmp2, tmp2, tmp3
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2// r2 in high-half
    vmov s4, tmp

    //r3 
    smultt tmp, poly0, poly0 //a1^2
    smulbb tmp2, poly0, poly1 //a0a2
    sub tmp, tmp, tmp2, lsl #1 //a1^2-2a0a2
    plant_red q, qa, qinv, tmp
    smultt tmp, poly0, tmp //a1(a1^2-2a0a2)
    smlabt tmp, poly2, poly1, tmp //a1(a1^2-2a0a2)+a0^2a3
    //plant_red q, qa, qinv, tmp

    smulbb tmp2, poly1, poly1 //a2^2
    smultt tmp3, poly0, poly1 //a1a3
    sub tmp2, tmp2, tmp3
    plant_red q, qa, qinv, tmp2 //(a2^2-a1a3)
    //smultt tmp2, poly1, tmp2 //a3(a2^2-a1a3)
    smulwt tmp2, zeta, tmp2 //(a2^2-a1a3)zeta
    smlabt tmp2, tmp2, q, qa
    smlatt tmp, poly1, tmp2, tmp // a1(a1^2-2a0a2)+a0^2a3+a3(a2^2-a1a3)zeta
    plant_red q, qa, qinv, tmp //r3 in high-half
    vmov s5, tmp

    //det=a3^4*zeta^3-a0^4-(a2^2(a2^2-4a1a3)+2(a1^2+2a0a2)a3^2)zeta^2+(a1^2(a1^2-4a0a2)+2a0^2(a2^2+2a1a3))zeta
    smulbb tmp, poly3, poly3 //a2^4
    smultt tmp2, poly0, poly1 //a1a3
    neg.w tmp2, tmp2
    lsl tmp2, tmp2, #2 //-4a1a3
    plant_red q, qa, qinv, tmp2 
    smlabt tmp, poly3, tmp2, tmp //a2^4+(-4a1a3a2^2)
    plant_red q, qa, qinv, tmp //a2^4+(-4a1a3a2^2) mod q in high-half

    smultt tmp2, poly2, poly3 //a1^2a3^2
    smulbb tmp3, poly0, poly1 //a0a2
    lsl tmp3, tmp3, #1
    plant_red q, qa, qinv, tmp3 //2a0a2 mod q
    smlatt tmp2, tmp3, poly3, tmp2 //a1^2a3^2+2a0a2*a3^2
    lsl tmp2, tmp2, #1
    plant_red q, qa, qinv, tmp2 //2(a1^2a3^2+2a0a2*a3^2)
    sadd16 tmp, tmp, tmp2 //a2^4+(-4a1a3a2^2)+2(a1^2a3^2+2a0a2*a3^2)

    smulwt tmp, zeta, tmp //(a2^4+(-4a1a3a2^2)+2(a1^2a3^2+2a0a2*a3^2))zeta
    smlabt tmp, tmp, q, qa
    smulwt tmp, zeta, tmp //(a2^4+(-4a1a3a2^2)+2(a1^2a3^2+2a0a2*a3^2))zeta^2
    smlabt tmp, tmp, q, qa //in high-half
    vmov s6, tmp //part of it store at s6

    smultt tmp, poly2, poly2 //a1^4
    smulbb tmp2, poly0, poly1 //a0a2
    neg.w tmp2, tmp2
    lsl tmp2, tmp2, #2 //-4a0a2
    plant_red q, qa, qinv, tmp2
    smlatt tmp, poly2, tmp2, tmp //a1^4-4a0a2a1^2
    plant_red q, qa, qinv, tmp

    smulbb tmp2, poly2, poly3 //a0^2*a2^2
    smultt tmp3, poly0, poly1 //a1a3
    lsl tmp3, tmp3, #1
    plant_red q, qa, qinv, tmp3 //2a1a3
    smlabt tmp2, poly2, tmp3, tmp2 // a0^2*a2^2+2a1a3*a0^2
    lsl tmp2, tmp2, #1
    plant_red q, qa, qinv, tmp2 //2(a0^2*a2^2+2a1a3*a0^2)
    sadd16 tmp, tmp, tmp2 //a1^4-4a0a2a1^2+2(a0^2*a2^2+2a1a3*a0^2)
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa //(a1^4-4a0a2a1^2+2(a0^2*a2^2+2a1a3*a0^2))zeta
    vmov tmp2, s6
    ssub16 tmp, tmp, tmp2 // in high-half

    smulwt tmp2, zeta, poly3 // a3^2*zeta
    smlabt tmp2, tmp2, q, qa 
    smulwt tmp2, zeta, tmp2 // a3^2*zeta^2
    smlabt tmp2, tmp2, q, qa 
    smulwt tmp2, zeta, tmp2 // a3^2*zeta^3
    smlabt tmp2, tmp2, q, qa 
    smultt tmp2, poly3, tmp2 //a3^4*zeta^3

    smulbb tmp3, poly2, poly2 //a0^4
    sub tmp2, tmp2, tmp3 //a3^4*zeta^3-a0^4
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2 //det

    // det reduce to [0,q)
    smulwt tmp, zetaptr, tmp
    smlabt tmp, tmp, q, qa //det*beta^-3 as the index of qinv_table

    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2 
    lsr tmp, tmp, #16
    vmov.w tmp3, s7 // qinv_table
    add tmp3, tmp3, tmp, lsl #2 //qinv_table+4*det to get the det^-1 2^(32*3) *qinv
    ldr.w tmp, [tmp3]

    //r0-r3
    vmov poly0, s2
    vmov poly1, s3
    vmov poly2, s4
    vmov poly3, s5

    smulwt poly0, tmp, poly0 //r0*det^-1
    smlabt poly0, poly0, q, qa
    smulwt poly1, tmp, poly1 //r1*det^-1
    smlabt poly1, poly1, q, qa
    smulwt poly2, tmp, poly2 //r2*det^-1
    smlabt poly2, poly2, q, qa
    smulwt poly3, tmp, poly3 //r3*det^-1
    smlabt poly3, poly3, q, qa

    pkhtb poly0, poly1, poly0, asr #16
    pkhtb poly1, poly3, poly2, asr #16
    neg.w zeta, zeta

    //mov poly2, tmp2 //check
    str.w poly0, [rptr], #4
    str.w poly1, [rptr], #4
    .endr

    vmov.w zetaptr, s1
    vmov.w loop, s0
    subs.w loop, #1
    bne.w 1b

    pop.w {r4-r11, pc}

