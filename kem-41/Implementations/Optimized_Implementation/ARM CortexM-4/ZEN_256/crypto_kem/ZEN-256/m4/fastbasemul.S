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
// for DIM_N=1024, 8 registers for 16 coefficients
###################################
#### small point-multiplication####
#### r0: out; r1: in1; r2: in2; r3: zetas####
## (a0,a1,a2,a3,a4,a5,a6,a7)*(b0,b1,b2,b3,b4,b5,b6,b7)-> (c0,c1,c2,c3,c4,c5,c6,c7)##
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
    poly4   .req r10  
    poly5   .req r11
    poly6   .req r3  // reuse r3; save before use it
    poly7   .req r0  // reuse r0; save before use it
	q       .req r8
	qa      .req r14 // reuse r14; save before use it
	qinv    .req r9
	tmp     .req r1  //reuse r1; save aptr before use it
	tmp2    .req r2  //reuse r2; save bptr before use it
	zeta    .req r12
	loop    .req r14

    movt q, #769  // q
    movw qinv, #64769
	movt qinv, #58632 // qinv

    movw loop, #64
    1:

    ldr.w zeta, [zetaptr], #4
    // save reused registers
    vmov.w s0, loop
    vmov.w s2, zetaptr

    movw qa, #24608 // qa
    .rept 2
    
    vmov.w s1, rptr

    ldr.w poly0, [aptr], #4 // a0||a1
    ldr.w poly1, [aptr], #4 // a2||a3
    ldr.w poly2, [aptr], #4 // a4||a5
    ldr.w poly3, [aptr], #4 // a6||a7

    ldr.w poly4, [bptr], #4 // b0||b1
    ldr.w poly5, [bptr], #4 // b2||b3
    ldr.w poly6, [bptr], #4 // b4||b5
    ldr.w poly7, [bptr], #4 // b6||b7

    // save reused registers
    
    vmov.w s3, aptr
    vmov.w s4, bptr

    // c1
    smuadx tmp, poly0, poly4 // a0b1+a1b0
    plant_red q, qa, qinv, tmp // res in high-half
    smuadx tmp2, poly1, poly7 //a2b7+a3b6
    smladx tmp2, poly2, poly6, tmp2 //a4b5+a5b4
    smladx tmp2, poly3, poly5, tmp2 //a6b3+a7b2 // TODO: overflow check
    plant_red q, qa, qinv, tmp2 //high-half of tmp2 
    smulwt tmp2, zeta, tmp2 
    smlabt tmp2, tmp2, q, qa // (a2b3+a3b2)*zeta mod q; in high-half
    sadd16 tmp, tmp, tmp2 // c1 in high-half
    vmov s5, tmp //store c1 in s5
    
    //c3
    smuadx tmp, poly0, poly5 // a0b3+a1b2
    smladx tmp, poly1, poly4, tmp // a0b3+a1b2+a2b1+a3b0
    plant_red q, qa, qinv, tmp 
    smuadx tmp2, poly2, poly7 // a4b7+a5b6
    smladx tmp2, poly3, poly6, tmp2 //a6b5+a7b4
    plant_red q, qa, qinv, tmp2 
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa
    sadd16 tmp, tmp, tmp2  // c3 in high-half
    vmov s6, tmp // store c3 in s6

    //c5
    smuadx tmp, poly0, poly6 //a0b5+a1b4
    smladx tmp, poly1, poly5, tmp //a2b3+a3b2
    smladx tmp, poly2, poly4, tmp //a4b1+a5b0
    plant_red q, qa, qinv, tmp
    smuadx tmp2, poly3, poly7 // a6b7+a7b6
    plant_red q, qa, qinv, tmp2
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa
    sadd16 tmp, tmp, tmp2 // c5 in high-half
    vmov s7, tmp //store c5 in s7

    //c7
    smuadx tmp, poly0, poly7 //a0b7+a1b6
    smladx tmp, poly1, poly6, tmp //a2b5+a3b4
    smladx tmp, poly2, poly5, tmp //a4b3+a5b2
    smladx tmp, poly3, poly4, tmp //a6b1+a7b0
    plant_red q, qa, qinv, tmp
    vmov s8, tmp // store c7 in s8

    // c0
    smultt tmp, poly0, poly7 //a1*b7
    smlabb tmp, poly1, poly7, tmp //a2*b6
    smlatt tmp, poly1, poly6, tmp //a3*b5 
    smlabb tmp, poly2, poly6, tmp //a4b4
    smlatt tmp, poly2, poly5, tmp //a5b3
    smlabb tmp, poly3, poly5, tmp //a6b2
    smlatt tmp, poly3, poly4, tmp //a7b1
    plant_red q, qa, qinv, tmp // in high-half
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa // in high-half
    smulbb tmp2, poly0, poly4 //a0b0
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2
    vmov tmp2, s5
    pkhtb tmp, tmp2, tmp, asr #16 //c0||c1
    vmov s5, tmp //c0||c1

    //c2
    smulbb tmp, poly0, poly5 //a0b2
    smlatt tmp, poly0, poly4, tmp //a1b1
    smlabb tmp, poly1, poly4, tmp //a2b0
    plant_red q, qa, qinv, tmp //in high-half

    smultt tmp2, poly1, poly7 //a3b7
    smlabb tmp2, poly2, poly7, tmp2 //a4b6
    smlatt tmp2, poly2, poly6, tmp2 //a5b5
    smlabb tmp2, poly3, poly6, tmp2 //a6b4
    smlatt tmp2, poly3, poly5, tmp2 //a7b3
    plant_red q, qa, qinv, tmp2
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa //c2 in high-half
    sadd16 tmp, tmp, tmp2
    vmov tmp2, s6
    pkhtb tmp, tmp2, tmp, asr #16 //c2||c3
    vmov s6, tmp
    
    //c4
    smulbb tmp, poly0, poly6 //a0b4
    smlatt tmp, poly0, poly5, tmp //a1b3
    smlabb tmp, poly1, poly5, tmp //a2b2
    smlatt tmp, poly1, poly4, tmp //a3b1
    smlabb tmp, poly2, poly4, tmp //a4b0
    plant_red q, qa, qinv, tmp //in high-half

    smultt tmp2, poly2, poly7 //a5b7
    smlabb tmp2, poly3, poly7, tmp2 //a6b6
    smlatt tmp2, poly3, poly6, tmp2 //a7b5
    plant_red q, qa, qinv, tmp2 
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa //c4 in high-half
    sadd16 tmp, tmp, tmp2
    vmov tmp2, s7
    pkhtb tmp, tmp2, tmp, asr #16 //c4||c5
    vmov s7, tmp

    //c6
    smulbb tmp2, poly0, poly7 //a0b6
    smlatt tmp2, poly0, poly6, tmp2 //a1b5
    smlabb tmp2, poly1, poly6, tmp2 //a2b4
    smlatt tmp2, poly1, poly5, tmp2 //a3b3
    smlabb tmp2, poly2, poly5, tmp2 //a4b2
    smlatt tmp2, poly2, poly4, tmp2 //a5b1
    smlabb tmp2, poly3, poly4, tmp2 //a6b0
    // plant_red q, qa, qinv, tmp2 //in high-half

    //smultt tmp, poly3, poly7 //a7b7
    smulwt tmp, zeta, poly7 //b7zeta
    smlabt tmp, tmp, q, qa //b7zeta mod qa
    smlatt tmp, poly3, tmp, tmp2 
    plant_red q, qa, qinv, tmp
    vmov tmp2, s8
    pkhtb poly3, tmp2, tmp, asr #16 //c6||c7
    
    vmov poly0, s5
    vmov poly1, s6
    vmov poly2, s7

    // recover pre-save registers
    vmov.w rptr, s1 
    vmov.w aptr, s3 
    vmov.w bptr, s4 

    str.w poly0, [rptr], #4
    str.w poly1, [rptr], #4
    str.w poly2, [rptr], #4
    str.w poly3, [rptr], #4
    neg zeta, zeta 
    .endr

    vmov.w zetaptr, s2
    vmov.w loop, s0
    subs.w loop, #1
    bne.w 1b

    pop.w {r4-r11, pc}

// for DIM_N=1024, 8 registers for 16 coefficients
###################################
#### small point-multiplication####
#### r0: out; r1: in1; r2: in2; r3: zetas####
## (a0,a1,a2,a3,a4,a5,a6,a7)*(b0,b1,b2,b3,b4,b5,b6,b7)-> (c0,c1,c2,c3,c4,c5,c6,c7)##
### Output in [0,q) & normal domain: results multiplied by beta^2 *qinv mod q so that the results in normal domain instead of plantard domain.
###################################
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
    poly4   .req r10  
    poly5   .req r11
    poly6   .req r3  // reuse r3; save before use it
    poly7   .req r0  // reuse r0; save before use it
	q       .req r8
	qa      .req r14 // reuse r14; save before use it
	qinv    .req r9
	tmp     .req r1  //reuse r1; save aptr before use it
	tmp2    .req r2  //reuse r2; save bptr before use it
	zeta    .req r12
	loop    .req r14

    movt q, #769  // q
    movw qinv, #64769
	movt qinv, #58632 // qinv

    movw loop, #64
    1:

    ldr.w zeta, [zetaptr], #4
    // save reused registers
    vmov.w s0, loop
    vmov.w s2, zetaptr

    movw qa, #24608 // qa
    .rept 2
    
    vmov.w s1, rptr

    ldr.w poly0, [aptr], #4 // a0||a1
    ldr.w poly1, [aptr], #4 // a2||a3
    ldr.w poly2, [aptr], #4 // a4||a5
    ldr.w poly3, [aptr], #4 // a6||a7

    ldr.w poly4, [bptr], #4 // b0||b1
    ldr.w poly5, [bptr], #4 // b2||b3
    ldr.w poly6, [bptr], #4 // b4||b5
    ldr.w poly7, [bptr], #4 // b6||b7

    // save reused registers
    
    vmov.w s3, aptr
    vmov.w s4, bptr
    vmov s9, poly6
    movw tmp, #50793
    movt tmp, #63916
    vmov s10, tmp // multiplied by (-2^32)^2*qinv to recover in normal domain

    // c1
    smuadx tmp, poly0, poly4 // a0b1+a1b0
    plant_red q, qa, qinv, tmp // res in high-half
    smuadx tmp2, poly1, poly7 //a2b7+a3b6
    smladx tmp2, poly2, poly6, tmp2 //a4b5+a5b4
    smladx tmp2, poly3, poly5, tmp2 //a6b3+a7b2 // TODO: overflow check
    plant_red q, qa, qinv, tmp2 //high-half of tmp2 
    smulwt tmp2, zeta, tmp2 
    smlabt tmp2, tmp2, q, qa // (a2b3+a3b2)*zeta mod q; in high-half
    sadd16 tmp, tmp, tmp2 // c1 in high-half
    
    vmov poly6, s10
    smulwt tmp, poly6, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2
    vmov poly6, s9
    vmov s5, tmp //store c1 in s5
    
    //c3
    smuadx tmp, poly0, poly5 // a0b3+a1b2
    smladx tmp, poly1, poly4, tmp // a0b3+a1b2+a2b1+a3b0
    plant_red q, qa, qinv, tmp 
    smuadx tmp2, poly2, poly7 // a4b7+a5b6
    smladx tmp2, poly3, poly6, tmp2 //a6b5+a7b4
    plant_red q, qa, qinv, tmp2 
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa
    sadd16 tmp, tmp, tmp2  // c3 in high-half
    vmov poly6, s10
    smulwt tmp, poly6, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2
    vmov poly6, s9
    vmov s6, tmp // store c3 in s6

    //c5
    smuadx tmp, poly0, poly6 //a0b5+a1b4
    smladx tmp, poly1, poly5, tmp //a2b3+a3b2
    smladx tmp, poly2, poly4, tmp //a4b1+a5b0
    plant_red q, qa, qinv, tmp
    smuadx tmp2, poly3, poly7 // a6b7+a7b6
    plant_red q, qa, qinv, tmp2
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa
    sadd16 tmp, tmp, tmp2 // c5 in high-half
    vmov poly6, s10
    smulwt tmp, poly6, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2
    vmov poly6, s9
    vmov s7, tmp //store c5 in s7

    //c7
    smuadx tmp, poly0, poly7 //a0b7+a1b6
    smladx tmp, poly1, poly6, tmp //a2b5+a3b4
    smladx tmp, poly2, poly5, tmp //a4b3+a5b2
    smladx tmp, poly3, poly4, tmp //a6b1+a7b0
    plant_red q, qa, qinv, tmp
    vmov poly6, s10
    smulwt tmp, poly6, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2
    vmov poly6, s9
    vmov s8, tmp // store c7 in s8

    // c0
    smultt tmp, poly0, poly7 //a1*b7
    smlabb tmp, poly1, poly7, tmp //a2*b6
    smlatt tmp, poly1, poly6, tmp //a3*b5 
    smlabb tmp, poly2, poly6, tmp //a4b4
    smlatt tmp, poly2, poly5, tmp //a5b3
    smlabb tmp, poly3, poly5, tmp //a6b2
    smlatt tmp, poly3, poly4, tmp //a7b1
    plant_red q, qa, qinv, tmp // in high-half
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa // in high-half
    smulbb tmp2, poly0, poly4 //a0b0
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2
    vmov poly6, s10
    smulwt tmp, poly6, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2
    vmov poly6, s9
    vmov tmp2, s5
    pkhtb tmp, tmp2, tmp, asr #16 //c0||c1
    vmov s5, tmp //c0||c1

    //c2
    smulbb tmp, poly0, poly5 //a0b2
    smlatt tmp, poly0, poly4, tmp //a1b1
    smlabb tmp, poly1, poly4, tmp //a2b0
    plant_red q, qa, qinv, tmp //in high-half

    smultt tmp2, poly1, poly7 //a3b7
    smlabb tmp2, poly2, poly7, tmp2 //a4b6
    smlatt tmp2, poly2, poly6, tmp2 //a5b5
    smlabb tmp2, poly3, poly6, tmp2 //a6b4
    smlatt tmp2, poly3, poly5, tmp2 //a7b3
    plant_red q, qa, qinv, tmp2
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa //c2 in high-half
    sadd16 tmp, tmp, tmp2
    vmov poly6, s10
    smulwt tmp, poly6, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2
    vmov poly6, s9
    vmov tmp2, s6
    pkhtb tmp, tmp2, tmp, asr #16 //c2||c3
    vmov s6, tmp
    
    //c4
    smulbb tmp, poly0, poly6 //a0b4
    smlatt tmp, poly0, poly5, tmp //a1b3
    smlabb tmp, poly1, poly5, tmp //a2b2
    smlatt tmp, poly1, poly4, tmp //a3b1
    smlabb tmp, poly2, poly4, tmp //a4b0
    plant_red q, qa, qinv, tmp //in high-half

    smultt tmp2, poly2, poly7 //a5b7
    smlabb tmp2, poly3, poly7, tmp2 //a6b6
    smlatt tmp2, poly3, poly6, tmp2 //a7b5
    plant_red q, qa, qinv, tmp2 
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa //c4 in high-half
    sadd16 tmp, tmp, tmp2
    vmov poly6, s10
    smulwt tmp, poly6, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2
    vmov poly6, s9
    vmov tmp2, s7
    pkhtb tmp, tmp2, tmp, asr #16 //c4||c5
    vmov s7, tmp

    //c6
    smulbb tmp2, poly0, poly7 //a0b6
    smlatt tmp2, poly0, poly6, tmp2 //a1b5
    smlabb tmp2, poly1, poly6, tmp2 //a2b4
    smlatt tmp2, poly1, poly5, tmp2 //a3b3
    smlabb tmp2, poly2, poly5, tmp2 //a4b2
    smlatt tmp2, poly2, poly4, tmp2 //a5b1
    smlabb tmp2, poly3, poly4, tmp2 //a6b0
    // plant_red q, qa, qinv, tmp2 //in high-half

    //smultt tmp, poly3, poly7 //a7b7
    smulwt tmp, zeta, poly7 //b7zeta
    smlabt tmp, tmp, q, qa //b7zeta mod qa
    smlatt tmp, poly3, tmp, tmp2 
    plant_red q, qa, qinv, tmp
    vmov poly6, s10
    smulwt tmp, poly6, tmp
    smlabt tmp, tmp, q, qa
    and tmp2, q, tmp, asr #31
    sadd16 tmp, tmp, tmp2
    vmov poly6, s9
    vmov tmp2, s8
    pkhtb poly3, tmp2, tmp, asr #16 //c6||c7
    
    vmov poly0, s5
    vmov poly1, s6
    vmov poly2, s7

    // recover pre-save registers
    vmov.w rptr, s1 
    vmov.w aptr, s3 
    vmov.w bptr, s4 

    str.w poly0, [rptr], #4
    str.w poly1, [rptr], #4
    str.w poly2, [rptr], #4
    str.w poly3, [rptr], #4
    neg zeta, zeta 
    .endr

    vmov.w zetaptr, s2
    vmov.w loop, s0
    subs.w loop, #1
    bne.w 1b

    pop.w {r4-r11, pc}

.unreq rptr   
.unreq aptr   
.unreq bptr   
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
## (a0,...,a7)^-1 -> (c0,...,c7) ##
###################################
.align 2
.global baseinv_asm
.type baseinv_asm, %function
baseinv_asm:
    push {r4-r11, lr}

    rptr    .req r0
    aptr    .req r1
    zetaptr .req r2
    qtab    .req r3
    poly0   .req r4
    poly1   .req r5
    poly2   .req r6
    poly3   .req r7
    q       .req r8
    qinv    .req r9
    tmp     .req r10
    tmp2    .req r11
    zeta    .req r12
    qa      .req r14

    movt q, #769
    movw qinv, #64769
    movt qinv, #58632
    vmov s13, qtab

    movw tmp, #64
    vmov s15, tmp

1:
    movw qa, #24608
    ldr.w zeta, [zetaptr], #4
    neg.w zeta, zeta
    vmov s14, zetaptr

    .rept 2
    ldr.w poly0, [aptr], #4  // a0|a1
    ldr.w poly1, [aptr], #4  // a2|a3
    ldr.w poly2, [aptr], #4  // a4|a5
    ldr.w poly3, [aptr], #4  // a6|a7

    // b0 = ((a1a7 + a3a5 - a2a6)*2 - a4^2) * zeta + a0^2
    smultt tmp, poly0, poly3
    smlatt tmp, poly1, poly2, tmp
    smulbb tmp2, poly1, poly3
    sub.w tmp, tmp, tmp2
    lsl.w tmp, tmp, #1
    smulbb tmp2, poly2, poly2 
    sub.w tmp, tmp, tmp2 
    plant_red q, qa, qinv, tmp
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa
    smulbb tmp2, poly0, poly0
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2
    vmov s4, tmp // b0->s4

    // b1 = ((a3a7 - a4a6)*2 + a5^2) * zeta + 2a0a2 - a1^2
    smultt tmp, poly1, poly3
    smulbb tmp2, poly2, poly3
    sub.w tmp, tmp, tmp2
    lsl.w tmp, tmp, #1
    smlatt tmp, poly2, poly2, tmp 
    plant_red q, qa, qinv, tmp
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa

    smulbb tmp2, poly0, poly1
    lsl.w tmp2, tmp2, #1
    smultt qtab, poly0, poly0
    sub.w tmp2, tmp2, qtab
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2
    vmov s5, tmp //b1->s5

    // b2 = (2a5a7 - a6^2) * zeta + 2(a0a4 - a1a3) + a2^2
    smultt tmp, poly2, poly3
    lsl.w tmp, tmp, #1
    smulbb tmp2, poly3, poly3
    sub.w tmp, tmp, tmp2
    plant_red q, qa, qinv, tmp 
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa

    smulbb tmp2, poly0, poly2
    smultt qtab, poly0, poly1
    sub.w tmp2, tmp2, qtab
    lsl.w tmp2, tmp2, #1
    smlabb tmp2, poly1, poly1, tmp2
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2
    vmov s6, tmp

    // b3 = a7^2 * zeta + 2(a0a6 + a2a4 - a1a5) - a3^2
    smulwt tmp, zeta, poly3
    smlabt tmp, tmp, q, qa
    smultt tmp, tmp, poly3

    smulbb tmp2, poly0, poly3
    smlabb tmp2, poly1, poly2, tmp2
    smultt qtab, poly0, poly2
    sub.w tmp2, tmp2, qtab
    lsl.w tmp2, tmp2, #1
    smultt qtab, poly1, poly1
    sub.w tmp2, tmp2, qtab
    add.w tmp2, tmp, tmp2
    plant_red q, qa, qinv, tmp2

    // pack b2||b3 and b0||b1 
    vmov tmp, s6
    pkhtb tmp, tmp2, tmp, asr #16
    vmov s9, tmp
    vmov tmp, s4
    vmov tmp2, s5
    pkhtb tmp, tmp2, tmp, asr #16
    vmov s8, tmp
    
    // c0 = (2b1b3 - b2^2) * zeta + b0^2
    vmov poly2, s8
    vmov poly3, s9
    smultt tmp, poly2, poly3
    lsl.w tmp, tmp, #1
    smulbb tmp2, poly3, poly3
    sub.w tmp, tmp, tmp2
    plant_red q, qa, qinv, tmp
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa
    smulbb tmp2, poly2, poly2
    plant_red q, qa, qinv, tmp2
    sadd16 tmp, tmp, tmp2
    vmov s10, tmp

    // c1 = b3^2 * zeta + 2b0b2 - b1^2
    smulwt tmp, zeta, poly3
    smlabt tmp, tmp, q, qa
    smultt tmp, tmp, poly3
    smulbb tmp2, poly2, poly3
    add.w tmp, tmp, tmp2, lsl #1
    smultt tmp2, poly2, poly2
    sub.w tmp, tmp, tmp2
    plant_red q, qa, qinv, tmp
    vmov s11, tmp

    // det = c1^2 * zeta + c0^2
    smulwt tmp2, zeta, tmp
    smlabt tmp2, tmp2, q, qa
    smultt tmp, tmp2, tmp
    vmov tmp2, s10
    smlatt tmp, tmp2, tmp2, tmp
    plant_red q, qa, qinv, tmp

    // det*R^-7 -> load (det*R^-7)^{-1}=det^-1*R^7
    sxth tmp, tmp, ror #16
    movw qtab, #769
    and.w tmp2, qtab, tmp, asr #31
    add.w tmp, tmp, tmp2
    vmov qtab, s13
    add.w qtab, qtab, tmp, lsl #2
    ldr.w qtab, [qtab]

    // c0 <- det*c0, c1 <- -det*c1
    vmov tmp, s10
    smulwt tmp, qtab, tmp
    smlabt tmp, tmp, q, qa
    vmov tmp2, s11
    neg.w qtab, qtab
    smulwt tmp2, qtab, tmp2
    smlabt tmp2, tmp2, q, qa
    // pack c0||c1
    pkhtb tmp, tmp2, tmp, asr #16
    vmov s12, tmp

    // f0 = c0*b0 - c1*b2*zeta
    vmov poly2, s12 //c0|c1
    vmov poly3, s9  //b2|b3
    smulwb tmp, zeta, poly3 // b2*zeta
    smlabt tmp, tmp, q, qa 
    smultt tmp, poly2, tmp //c1*b2*zeta
    vmov poly3, s8  //b0|b1
    smulbb tmp2, poly2, poly3
    sub.w tmp, tmp2, tmp
    plant_red q, qa, qinv, tmp
    vmov s4, tmp

    // f1 = c1*b3*zeta - c0*b1
    vmov poly3, s9
    smulwt tmp, zeta, poly3
    smlabt tmp, tmp, q, qa
    smultt tmp, poly2, tmp
    vmov poly3, s8
    smulbt tmp2, poly2, poly3
    sub.w tmp, tmp, tmp2
    plant_red q, qa, qinv, tmp
    vmov s5, tmp

    // f2 = c0*b2 + c1*b0
    vmov poly3, s9
    smulbb tmp, poly2, poly3
    vmov poly3, s8
    smlatb tmp, poly2, poly3, tmp
    plant_red q, qa, qinv, tmp
    vmov s6, tmp

    // f3 = -(c0*b3 + c1*b1)
    vmov poly3, s9
    smulbt tmp, poly2, poly3
    vmov poly3, s8
    smlatt tmp, poly2, poly3, tmp
    neg.w tmp2, tmp
    plant_red q, qa, qinv, tmp2
    vmov s7, tmp2

    // pack f2||f3 and f0||f1 
    vmov tmp, s6
    pkhtb tmp, tmp2, tmp, asr #16
    vmov s11, tmp
    vmov tmp, s4
    vmov tmp2, s5
    pkhtb tmp, tmp2, tmp, asr #16
    vmov s10, tmp

    ldr.w qtab, [aptr, #-8]   // a4||a5
    ldr.w tmp2, [aptr, #-4]   // a6||a7
    vmov s12, tmp2
    vmov poly2, s10           // f0||f1
    vmov poly3, s11           // f2||f3

    // r0 = f0*a0 - zeta*(f1*a6 + f2*a4 + f3*a2)
    vmov tmp2, s12
    smultb tmp, poly2, tmp2
    smlabb tmp, poly3, qtab, tmp
    smlatb tmp, poly3, poly1, tmp
    plant_red q, qa, qinv, tmp
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa
    smulbb tmp2, poly2, poly0
    plant_red q, qa, qinv, tmp2
    ssub16 tmp2, tmp2, tmp
    vmov s4, tmp2

    // r1 = zeta*(f1*a7 + f2*a5 + f3*a3) - f0*a1
    vmov tmp2, s12
    smultt tmp, poly2, tmp2
    smlabt tmp, poly3, qtab, tmp
    smlatt tmp, poly3, poly1, tmp
    plant_red q, qa, qinv, tmp
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa
    smulbt tmp2, poly2, poly0
    plant_red q, qa, qinv, tmp2
    ssub16 tmp, tmp, tmp2
    vmov s5, tmp

    // r2 = f0*a2 + f1*a0 - zeta*(f2*a6 + f3*a4)
    smulbb tmp, poly2, poly1
    smlatb tmp, poly2, poly0, tmp
    plant_red q, qa, qinv, tmp
    vmov tmp2, s12
    smulbb tmp2, poly3, tmp2
    smlatb tmp2, poly3, qtab, tmp2
    plant_red q, qa, qinv, tmp2
    smulwt tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa
    ssub16 tmp, tmp, tmp2
    vmov s6, tmp

    // r3 = zeta*(f2*a7 + f3*a5) - (f0*a3 + f1*a1)
    vmov tmp2, s12
    smulbt tmp, poly3, tmp2
    smlatt tmp, poly3, qtab, tmp
    plant_red q, qa, qinv, tmp
    smulwt tmp, zeta, tmp
    smlabt tmp, tmp, q, qa
    smulbt tmp2, poly2, poly1
    smlatt tmp2, poly2, poly0, tmp2
    plant_red q, qa, qinv, tmp2
    ssub16 tmp, tmp, tmp2
    vmov s7, tmp

    // r4 = f0*a4 + f1*a2 + f2*a0 - zeta*(f3*a6)
    smulbb tmp, poly2, qtab
    smlatb tmp, poly2, poly1, tmp
    smlabb tmp, poly3, poly0, tmp
    vmov tmp2, s12
    smulwb tmp2, zeta, tmp2
    smlabt tmp2, tmp2, q, qa
    smultt tmp2, poly3, tmp2
    sub.w tmp, tmp, tmp2
    plant_red q, qa, qinv, tmp
    vmov s0, tmp

    // r5 = zeta*(f3*a7) - (f0*a5 + f1*a3 + f2*a1)
    vmov tmp2, s12
    smulwt tmp, zeta, tmp2
    smlabt tmp, tmp, q, qa
    smultt tmp, poly3, tmp
    
    smulbt tmp2, poly2, qtab
    smlatt tmp2, poly2, poly1, tmp2
    smlabt tmp2, poly3, poly0, tmp2
    sub.w tmp, tmp, tmp2
    plant_red q, qa, qinv, tmp
    vmov s1, tmp

    // r6 = f0*a6 + f1*a4 + f2*a2 + f3*a0
    vmov tmp2, s12
    smultb tmp, poly2, qtab
    smlabb tmp, poly3, poly1, tmp
    smlatb tmp, poly3, poly0, tmp
    smlabb tmp, poly2, tmp2, tmp
    plant_red q, qa, qinv, tmp
    vmov s2, tmp

    // r7 = -(f0*a7 + f1*a5 + f2*a3 + f3*a1)
    vmov tmp2, s12
    smulbt tmp, poly2, tmp2
    smlatt tmp, poly2, qtab, tmp
    smlabt tmp, poly3, poly1, tmp
    smlatt tmp, poly3, poly0, tmp
    neg.w tmp, tmp
    plant_red q, qa, qinv, tmp
    vmov s3, tmp

    // store r0..r7
    vmov tmp, s4
    vmov tmp2, s5
    pkhtb tmp, tmp2, tmp, asr #16
    vmov poly0, s6
    vmov poly1, s7
    pkhtb poly0, poly1, poly0, asr #16
    vmov poly2, s0
    vmov poly3, s1
    pkhtb poly2, poly3, poly2, asr #16
    vmov poly3, s2
    vmov tmp2, s3
    pkhtb poly3, tmp2, poly3, asr #16

    str.w tmp, [rptr], #4
    str.w poly0, [rptr], #4
    str.w poly2, [rptr], #4
    str.w poly3, [rptr], #4

    neg.w zeta, zeta
    .endr

    vmov zetaptr, s14
    vmov tmp, s15
    subs.w tmp, tmp, #1
    vmov s15, tmp
    bne.w 1b

    pop.w {r4-r11, pc}
.unreq rptr
.unreq aptr
.unreq zetaptr
.unreq qtab
.unreq poly0
.unreq poly1
.unreq poly2
.unreq poly3
.unreq q
.unreq qinv
.unreq tmp
.unreq tmp2
.unreq zeta
.unreq qa
