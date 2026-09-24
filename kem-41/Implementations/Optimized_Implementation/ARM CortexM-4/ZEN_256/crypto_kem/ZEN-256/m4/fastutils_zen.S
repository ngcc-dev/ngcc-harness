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

#### check poly inverse in Zq #############################################
#### r0: a                                                               ##
#### return: 1 if any 8-coefficient NTT block sums to zero, otherwise 0  ##
##########################################################################
.align 2
.global check_poly_inv_Zq_asm
.type check_poly_inv_Zq_asm, %function
check_poly_inv_Zq_asm:
    push {r4-r9, lr}

    aptr    .req r0
    acc     .req r1
    ones    .req r2
    loop    .req r3
    sum     .req r4
    word0   .req r5
    word1   .req r6
    word2   .req r7
    tmp     .req r8
    word3   .req r9

    movs acc, #0
    movw ones, #1
    movt ones, #1
    movw loop, #128

1:
    ldr.w word0, [aptr], #4
    ldr.w word1, [aptr], #4
    ldr.w word2, [aptr], #4
    ldr.w word3, [aptr], #4

    smuad sum, word0, ones
    smlad sum, word1, ones, sum
    smlad sum, word2, ones, sum
    smlad sum, word3, ones, sum

    rsb tmp, sum, #0
    orr tmp, tmp, sum
    mvn tmp, tmp
    orr acc, acc, tmp, lsr #31

    subs loop, #1
    bne 1b

    and r0, acc, #1
    pop {r4-r9, pc}

.unreq aptr
.unreq acc
.unreq ones
.unreq loop
.unreq sum
.unreq word0
.unreq word1
.unreq word2
.unreq tmp
.unreq word3

###################################
#### check poly inverse in Z2 #####
#### r0: a ########################
###################################
.align 2
.global check_poly_inv_Z2_asm
.type check_poly_inv_Z2_asm, %function
check_poly_inv_Z2_asm:
    push {r4-r6, lr}

    aptr    .req r0
    xacc0   .req r1
    xacc1   .req r2
    tmp0    .req r3
    tmp1    .req r4
    tmp2    .req r5
    tmp3    .req r6

    movs xacc0, #0
    movs xacc1, #0
    movw tmp3, #16*2

1:
    ldr.w tmp0, [aptr], #4
    ldr.w tmp1, [aptr], #4
    ldr.w tmp2, [aptr], #4
    ldr.w r12, [aptr], #4

    eor xacc0, xacc0, tmp0
    eor xacc1, xacc1, tmp1
    eor xacc0, xacc0, tmp2
    eor xacc1, xacc1, r12
    subs tmp3, #1
    bne 1b

    eor xacc0, xacc0, xacc1
    eor xacc0, xacc0, xacc0, ror #16
    uxth xacc0, xacc0
    cmp xacc0, #0
    ite eq
    moveq r0, #1
    movne r0, #0
    pop {r4-r6, pc}

.unreq aptr
.unreq xacc0
.unreq xacc1
.unreq tmp0
.unreq tmp1
.unreq tmp2
.unreq tmp3


###################################
#### polynomial rounding ##########
#### r0: a ########################
###################################
.align 2
.global poly_round_asm
.type poly_round_asm, %function
poly_round_asm:
    push {r4-r11, lr}

    aptr    .req r0
    poly0   .req r1
    poly1   .req r2
    poly2   .req r3
    poly3   .req r4
    poly4   .req r5
    poly5   .req r6
    poly6   .req r7
    loop    .req r8
    poly7   .req r12
    scale   .req r9

    movw scale, #0x3fff // mask the low 14-bit; eliminate the 2-bit from top-half
    movt scale, #0x3fff
    movw loop, #64
1:
    ldr.w poly0, [aptr, #0]
    ldr.w poly1, [aptr, #4]
    ldr.w poly2, [aptr, #8]
    ldr.w poly3, [aptr, #12]
    ldr.w poly4, [aptr, #4*4]
    ldr.w poly5, [aptr, #4*5]
    ldr.w poly6, [aptr, #4*6]
    ldr.w poly7, [aptr, #4*7]

    and poly0, scale, poly0, lsr #2
    and poly1, scale, poly1, lsr #2
    and poly2, scale, poly2, lsr #2
    and poly3, scale, poly3, lsr #2
    and poly4, scale, poly4, lsr #2
    and poly5, scale, poly5, lsr #2
    and poly6, scale, poly6, lsr #2
    and poly7, scale, poly7, lsr #2

    str.w poly0, [aptr], #4
    str.w poly1, [aptr], #4
    str.w poly2, [aptr], #4
    str.w poly3, [aptr], #4
    str.w poly4, [aptr], #4
    str.w poly5, [aptr], #4
    str.w poly6, [aptr], #4
    str.w poly7, [aptr], #4
    subs loop, #1
    bne 1b

    pop {r4-r11, pc}

.unreq aptr  
.unreq poly0 
.unreq poly1 
.unreq poly2 
.unreq poly3 
.unreq poly4 
.unreq poly5 
.unreq poly6 
.unreq loop  
.unreq poly7 
.unreq scale 

// r=r+r<<1+r<<2+1
.align 2
.global poly_unround_asm
.type poly_unround_asm, %function
poly_unround_asm:
    push {r4-r10, lr}

    aptr    .req r0
    poly0   .req r1
    poly1   .req r2
    poly2   .req r3
    poly3   .req r4
    poly4   .req r5
    poly5   .req r6
    poly6   .req r7
    poly7   .req r8
    tmp     .req r9
    scale   .req r10
    loop    .req r12

    movw scale, #1
    movt scale, #1
    movw loop, #32*2
1:
    ldr.w poly0, [aptr, #4*0]
    ldr.w poly1, [aptr, #4*1]
    ldr.w poly2, [aptr, #4*2]
    ldr.w poly3, [aptr, #4*3]
    ldr.w poly4, [aptr, #4*4]
    ldr.w poly5, [aptr, #4*5]
    ldr.w poly6, [aptr, #4*6]
    ldr.w poly7, [aptr, #4*7]

    lsl poly0, poly0, #2
    lsl poly1, poly1, #2
    lsl poly2, poly2, #2
    lsl poly3, poly3, #2
    lsl poly4, poly4, #2
    lsl poly5, poly5, #2
    lsl poly6, poly6, #2
    lsl poly7, poly7, #2

    str.w poly0, [aptr], #4
    str.w poly1, [aptr], #4
    str.w poly2, [aptr], #4
    str.w poly3, [aptr], #4
    str.w poly4, [aptr], #4
    str.w poly5, [aptr], #4
    str.w poly6, [aptr], #4
    str.w poly7, [aptr], #4
    subs loop, #1
    bne 1b

    pop {r4-r10, pc}

.unreq aptr 
.unreq poly0
.unreq poly1
.unreq poly2
.unreq poly3
.unreq poly4
.unreq poly5
.unreq poly6
.unreq poly7
.unreq tmp  
.unreq scale
.unreq loop


// r=r+r<<7+r<<8+1
// void poly_mul385(int16_t r[DIM_N]);
.align 2
.global poly_mul385_asm
.type poly_mul385_asm, %function
poly_mul385_asm:
    push {r4-r10, lr}

    aptr    .req r0
    poly0   .req r1
    poly1   .req r2
    poly2   .req r3
    poly3   .req r4
    poly4   .req r5
    poly5   .req r6
    poly6   .req r7
    poly7   .req r8
    tmp     .req r9
    scale   .req r10
    loop    .req r12

    movw scale, #1
    movt scale, #1
    movw loop, #8*2
    .equ stride, 128*2*2
1:
    ldr.w poly0, [aptr, #4*0]
    ldr.w poly1, [aptr, #4*1]
    ldr.w poly2, [aptr, #4*2]
    ldr.w poly3, [aptr, #4*3]
    ldr.w poly4, [aptr, #4*4]
    ldr.w poly5, [aptr, #4*5]
    ldr.w poly6, [aptr, #4*6]
    ldr.w poly7, [aptr, #4*7]

    add tmp, poly0, poly0, lsl #7 // a0+a0*128=129a0
    add poly0, tmp, poly0, lsl #8 // 129a0+256a0=385a0
    add tmp, poly1, poly1, lsl #7 
    add poly1, tmp, poly1, lsl #8 
    add tmp, poly2, poly2, lsl #7 
    add poly2, tmp, poly2, lsl #8 
    add tmp, poly3, poly3, lsl #7 
    add poly3, tmp, poly3, lsl #8 
    add tmp, poly4, poly4, lsl #7 
    add poly4, tmp, poly4, lsl #8 
    add tmp, poly5, poly5, lsl #7 
    add poly5, tmp, poly5, lsl #8 
    add tmp, poly6, poly6, lsl #7 
    add poly6, tmp, poly6, lsl #8 
    add tmp, poly7, poly7, lsl #7 
    add poly7, tmp, poly7, lsl #8 

    str.w poly1, [aptr, #4*1+stride*0]
    str.w poly1, [aptr, #4*1+stride*1]
    str.w poly1, [aptr, #4*1+stride*2]
    str.w poly1, [aptr, #4*1+stride*3]

    str.w poly2, [aptr, #4*2+stride*0]
    str.w poly2, [aptr, #4*2+stride*1]
    str.w poly2, [aptr, #4*2+stride*2]
    str.w poly2, [aptr, #4*2+stride*3]

    str.w poly3, [aptr, #4*3+stride*0]
    str.w poly3, [aptr, #4*3+stride*1]
    str.w poly3, [aptr, #4*3+stride*2]
    str.w poly3, [aptr, #4*3+stride*3]

    str.w poly4, [aptr, #4*4+stride*0]
    str.w poly4, [aptr, #4*4+stride*1]
    str.w poly4, [aptr, #4*4+stride*2]
    str.w poly4, [aptr, #4*4+stride*3]

    str.w poly5, [aptr, #4*5+stride*0]
    str.w poly5, [aptr, #4*5+stride*1]
    str.w poly5, [aptr, #4*5+stride*2]
    str.w poly5, [aptr, #4*5+stride*3]

    str.w poly6, [aptr, #4*6+stride*0]
    str.w poly6, [aptr, #4*6+stride*1]
    str.w poly6, [aptr, #4*6+stride*2]
    str.w poly6, [aptr, #4*6+stride*3]

    str.w poly7, [aptr, #4*7+stride*0]
    str.w poly7, [aptr, #4*7+stride*1]
    str.w poly7, [aptr, #4*7+stride*2]
    str.w poly7, [aptr, #4*7+stride*3]
    
    str.w poly0, [aptr, #4*0+stride*1]
    str.w poly0, [aptr, #4*0+stride*2]
    str.w poly0, [aptr, #4*0+stride*3]
    str.w poly0, [aptr], #4*8

    subs loop, #1
    bne 1b

    pop {r4-r10, pc}

.unreq aptr 
.unreq poly0
.unreq poly1
.unreq poly2
.unreq poly3
.unreq poly4
.unreq poly5
.unreq poly6
.unreq poly7
.unreq tmp  
.unreq scale
.unreq loop

//poly_xor4(int16_t r[256],int16_t a[1024])
.align 2
.global poly_xor4_asm
.type poly_xor4_asm, %function
poly_xor4_asm:
    push {r4-r11, r14}

    rptr    .req r0
    aptr    .req r1
    poly0   .req r11
    poly1   .req r2
    poly2   .req r3
    poly3   .req r4
    poly4   .req r5
    poly5   .req r6
    poly6   .req r7
    poly7   .req r8
    tmp     .req r9
    scale   .req r10
    tmp2    .req r14
    loop    .req r12

    movw scale, #1
    movt scale, #1
    movw loop, #32*2
    .equ stride, 128*2*2
1:
    ldr.w poly1, [aptr, #4+stride*0]
    ldr.w poly2, [aptr, #0+stride*1]
    ldr.w poly3, [aptr, #4+stride*1]
    ldr.w poly4, [aptr, #0+stride*2]
    ldr.w poly5, [aptr, #4+stride*2]
    ldr.w poly6, [aptr, #0+stride*3]
    ldr.w poly7, [aptr, #4+stride*3]
    ldr.w poly0, [aptr], #4*2

    eor tmp, poly0, poly2
    eor tmp2, poly4, poly6
    eor tmp, tmp, tmp2
    and poly0, tmp, scale

    eor tmp, poly1, poly3
    eor tmp2, poly5, poly7
    eor tmp, tmp, tmp2
    and poly1, tmp, scale

    str.w poly1, [rptr,#4]
    str.w poly0, [rptr], #4*2
    subs loop, #1
    bne 1b

    pop {r4-r11, pc}

.unreq aptr 
.unreq poly0
.unreq poly1
.unreq poly2
.unreq poly3
.unreq poly4
.unreq poly5
.unreq poly6
.unreq poly7
.unreq tmp  
.unreq scale
.unreq tmp2
.unreq loop 



//poly_unpack_f2(int16_t r[256],uint8_t a[32])
.align 2
.global poly_unpack_f2_asm
.type poly_unpack_f2_asm, %function
poly_unpack_f2_asm:
    push {r4-r7, r14}

    rptr    .req r0
    aptr    .req r1
    byte    .req r2
    out0    .req r3
    out1    .req r4
    out2    .req r5
    out3    .req r6
    tmp     .req r7
    loop    .req r12

    movw loop, #32

1:
    ldrb byte, [aptr], #1

    ubfx out0, byte, #0, #1
    ubfx tmp,  byte, #1, #1
    pkhbt out0, out0, tmp, lsl #16

    ubfx out1, byte, #2, #1
    ubfx tmp,  byte, #3, #1
    pkhbt out1, out1, tmp, lsl #16

    ubfx out2, byte, #4, #1
    ubfx tmp,  byte, #5, #1
    pkhbt out2, out2, tmp, lsl #16

    ubfx out3, byte, #6, #1
    ubfx tmp,  byte, #7, #1
    pkhbt out3, out3, tmp, lsl #16

    str.w out0, [rptr], #4
    str.w out1, [rptr], #4
    str.w out2, [rptr], #4
    str.w out3, [rptr], #4

    subs.w loop, #1
    bne 1b

    pop {r4-r7, pc}

.unreq aptr
.unreq byte
.unreq out0
.unreq out1
.unreq out2
.unreq out3
.unreq tmp
.unreq loop


//poly_pack_f2(uint8_t a[32],int16_t r[256])
.align 2
.global poly_pack_f2_asm
.type poly_pack_f2_asm, %function
poly_pack_f2_asm:
    push {r4-r7, r14}

    rptr    .req r0
    aptr    .req r1
    word0   .req r2
    word1   .req r3
    word2   .req r4
    word3   .req r5
    byte    .req r6
    tmp     .req r7
    loop    .req r12

    movw loop, #32

1:
    ldr.w word0, [aptr], #4
    ldr.w word1, [aptr], #4
    ldr.w word2, [aptr], #4
    ldr.w word3, [aptr], #4

    and.w byte, word0, #1
    ubfx tmp, word0, #16, #1
    orr.w byte, byte, tmp, lsl #1

    and.w tmp, word1, #1
    orr.w byte, byte, tmp, lsl #2
    ubfx tmp, word1, #16, #1
    orr.w byte, byte, tmp, lsl #3

    and.w tmp, word2, #1
    orr.w byte, byte, tmp, lsl #4
    ubfx tmp, word2, #16, #1
    orr.w byte, byte, tmp, lsl #5

    and.w tmp, word3, #1
    orr.w byte, byte, tmp, lsl #6
    ubfx tmp, word3, #16, #1
    orr.w byte, byte, tmp, lsl #7

    strb byte, [rptr], #1

    subs.w loop, #1
    bne 1b

    pop {r4-r7, pc}

.unreq aptr
.unreq word0
.unreq word1
.unreq word2
.unreq word3
.unreq byte
.unreq tmp
.unreq loop

.align 2
.global pointwise_add
.type pointwise_add, %function
pointwise_add:
  push {r4-r11, lr}

  movw r14, #51*2
  1:
    ldm r1!, {r3-r7}
    ldm r2!, {r8-r12}
    uadd16 r3, r3, r8
    uadd16 r4, r4, r9
    uadd16 r5, r5, r10
    uadd16 r6, r6, r11
    uadd16 r7, r7, r12
    stm r0!, {r3-r7}

    subs.w r14, #1
  bne.w 1b

  ldr.w r3, [r1], #4
  ldr.w r4, [r1], #4
  ldr.w r8, [r2], #4
  ldr.w r9, [r2], #4
  uadd16 r3, r3, r8
  uadd16 r4, r4, r9
  str.w r3, [r0], #4
  str.w r4, [r0], #4
  pop {r4-r11, pc}


###################################
#### update cp and parity #########
#### r0: cp; r1: tmp2 #############
###################################
.align 2
.global update_cp_asm
.type update_cp_asm, %function
update_cp_asm:
    push {r4-r11, lr}

    cplow      .req r0
    lowptr     .req r1
    hiptr      .req r2
    outptr     .req r3
    cphigh     .req r4
    low        .req r5
    high       .req r6
    q          .req r7
    qa         .req r8
    tmp        .req r9
    plantconst .req r10
    ones       .req r11
    loop       .req r12
    parity     .req lr

    mov outptr, lowptr
    add.w hiptr, lowptr, #512*2
    add.w cphigh, cplow, #512*2
    movt q, #769
    movw qa, #24608
    movw plantconst, #14574
    movt plantconst, #85
    movw ones, #1
    movt ones, #1
    movw loop, #128*2

1:
    ldr.w low, [lowptr], #4
    ldr.w high, [hiptr], #4

    usub16 tmp, high, low
    uadd16 high, high, low

    doubleplant tmp, low, q, qa, plantconst
    doubleplant high, low, q, qa, plantconst

    and low, tmp, ones
    and parity, high, ones
    eor low, low, parity

    str.w tmp, [cplow], #4
    str.w high, [cphigh], #4    
    str.w low, [outptr], #4

    subs.w loop, #1
    bne.w 1b

    pop {r4-r11, pc}

 .unreq cplow
 .unreq lowptr
 .unreq hiptr
 .unreq outptr
 .unreq cphigh
 .unreq low
 .unreq high
 .unreq q
 .unreq qa
 .unreq tmp
 .unreq plantconst
 .unreq ones
 .unreq loop
 .unreq parity
