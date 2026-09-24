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

.syntax unified
.cpu cortex-m4
.thumb

.macro R2_LINEAR_MUL4_BODY out, ap, bp
    mov lr, \out
    movs r10, #8
    movs r2, #0
.Lr2_lm4_zero\@:
    str.w r2, [lr], #4
    subs r10, r10, #1
    bne .Lr2_lm4_zero\@

    movs r10, #0
.Lr2_lm4_outer\@:
    ldr.w r0, [\ap, r10, lsl #2]
    movs r12, #0
    mov r1, r10
.Lr2_lm4_inner\@:
    ldr.w r2, [\bp, r12, lsl #2]
    umull r2, r3, r0, r2
    and.w r2, r2, r11
    and.w r3, r3, r11

    ldr.w lr, [\out, r1, lsl #2]
    eor.w lr, lr, r2
    str.w lr, [\out, r1, lsl #2]

    adds r2, r1, #1
    ldr.w lr, [\out, r2, lsl #2]
    eor.w lr, lr, r3
    str.w lr, [\out, r2, lsl #2]

    adds r1, r1, #1
    adds r12, r12, #1
    cmp r12, #4
    bne .Lr2_lm4_inner\@

    adds r10, r10, #1
    cmp r10, #4
    bne .Lr2_lm4_outer\@
.endm


###########################################################################
#### n = 2 and n = 4 special cases                                      ####
#### r0: res, r1: a, r2: b                                              ####
###########################################################################
.align 2
.thumb_func
.global r2_radix16_mul_2_asm
.type r2_radix16_mul_2_asm, %function
r2_radix16_mul_2_asm:
    ldr.w r3, [r1]
    ldr.w r12, [r2]
    movs r1, #0x11
    smulbb r3, r3, r12
    eor.w r3, r3, r3, lsr #8
    and.w r1, r1, r3
    str.w r1, [r0]
    bx lr

.align 2
.thumb_func
.global r2_radix16_mul_4_asm
.type r2_radix16_mul_4_asm, %function
r2_radix16_mul_4_asm:
    ldr.w r3, [r1]
    ldr.w r12, [r2]
    movw r1, #0x1111
    smulbb r3, r3, r12
    eor.w r3, r3, r3, lsr #16
    and.w r1, r1, r3
    str.w r1, [r0]
    bx lr

###########################################################################
#### fixed word-count wrappers                                           ####
#### r0: res, r1: a, r2: b                                              ####
###########################################################################
.align 2
.thumb_func
.global r2_radix16_mul_8_asm
.type r2_radix16_mul_8_asm, %function
r2_radix16_mul_8_asm:
    movs r3, #1
    b r2_radix16_mul_words_asm

.align 2
.thumb_func
.global r2_radix16_mul_16_asm
.type r2_radix16_mul_16_asm, %function
r2_radix16_mul_16_asm:
    movs r3, #2
    b r2_radix16_mul_words_asm

.align 2
.thumb_func
.global r2_radix16_mul_32_asm
.type r2_radix16_mul_32_asm, %function
r2_radix16_mul_32_asm:
    movs r3, #4
    b r2_radix16_mul_words_asm

.align 2
.thumb_func
.global r2_radix16_mul_64_asm
.type r2_radix16_mul_64_asm, %function
r2_radix16_mul_64_asm:
    movs r3, #8
    b r2_radix16_mul_words_asm

.align 2
.thumb_func
.global r2_radix16_mul_128_asm
.type r2_radix16_mul_128_asm, %function
r2_radix16_mul_128_asm:
    push.w {r3-r11, lr}
    sub.w sp, sp, #128

    mov r4, r0                  @ res/base, 16 words
    mov r5, r1                  @ a, 16 words
    mov r6, r2                  @ b, 16 words
    mov r7, sp                  @ tmp: z2, then p1, 16 words
    add.w r8, sp, #64           @ sum a0+a1, 8 words
    add.w r9, sp, #96           @ sum b0+b1, 8 words

    bl r2_radix16_linear_mul8_asm

    mov r0, r7
    add.w r1, r5, #32
    add.w r2, r6, #32
    bl r2_radix16_linear_mul8_asm

    movs r10, #0
1:
    ldr.w r0, [r4, r10, lsl #2]
    ldr.w r1, [r7, r10, lsl #2]
    eor.w r0, r0, r1
    str.w r0, [r4, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #16
    bne 1b

    movw r11, #0x1111
    movt r11, #0x1111
    movs r10, #0
2:
    ldr.w r0, [r5, r10, lsl #2]
    add.w r12, r5, #32
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    and.w r0, r0, r11
    str.w r0, [r8, r10, lsl #2]

    ldr.w r0, [r6, r10, lsl #2]
    add.w r12, r6, #32
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    and.w r0, r0, r11
    str.w r0, [r9, r10, lsl #2]

    adds r10, r10, #1
    cmp r10, #8
    bne 2b

    mov r0, r7
    mov r1, r8
    mov r2, r9
    bl r2_radix16_linear_mul8_asm

    movs r10, #0
3:
    ldr.w r0, [r7, r10, lsl #2]
    ldr.w r1, [r4, r10, lsl #2]
    eor.w r0, r0, r1
    str.w r0, [r7, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #16
    bne 3b

    movs r10, #0
4:
    ldr.w r0, [r7, r10, lsl #2]
    add.w r12, r4, #32
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r1, r1, r0
    str.w r1, [r12, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #8
    bne 4b

    movs r10, #0
    add.w r12, r7, #32
5:
    ldr.w r0, [r12, r10, lsl #2]
    ldr.w r1, [r4, r10, lsl #2]
    eor.w r1, r1, r0
    str.w r1, [r4, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #8
    bne 5b

    add.w sp, sp, #128
    pop.w {r3-r11, pc}

.align 2
.thumb_func
.global r2_radix16_mul_256_asm
.type r2_radix16_mul_256_asm, %function
r2_radix16_mul_256_asm:
    push.w {r3-r11, lr}
    sub.w sp, sp, #256

    mov r4, r0                  @ res/base, 32 words
    mov r5, r1                  @ a, 32 words
    mov r6, r2                  @ b, 32 words
    mov r7, sp                  @ tmp: z2, then p1, 32 words
    add.w r8, sp, #128          @ sum a0+a1, 16 words
    add.w r9, sp, #192          @ sum b0+b1, 16 words

    bl r2_radix16_linear_mul16_asm

    mov r0, r7
    add.w r1, r5, #64
    add.w r2, r6, #64
    bl r2_radix16_linear_mul16_asm

    movs r10, #0
6:
    ldr.w r0, [r4, r10, lsl #2]
    ldr.w r1, [r7, r10, lsl #2]
    eor.w r0, r0, r1
    str.w r0, [r4, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #32
    bne 6b

    movw r11, #0x1111
    movt r11, #0x1111
    movs r10, #0
7:
    ldr.w r0, [r5, r10, lsl #2]
    add.w r12, r5, #64
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    and.w r0, r0, r11
    str.w r0, [r8, r10, lsl #2]

    ldr.w r0, [r6, r10, lsl #2]
    add.w r12, r6, #64
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    and.w r0, r0, r11
    str.w r0, [r9, r10, lsl #2]

    adds r10, r10, #1
    cmp r10, #16
    bne 7b

    mov r0, r7
    mov r1, r8
    mov r2, r9
    bl r2_radix16_linear_mul16_asm

    movs r10, #0
8:
    ldr.w r0, [r7, r10, lsl #2]
    ldr.w r1, [r4, r10, lsl #2]
    eor.w r0, r0, r1
    str.w r0, [r7, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #32
    bne 8b

    movs r10, #0
9:
    ldr.w r0, [r7, r10, lsl #2]
    add.w r12, r4, #64
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r1, r1, r0
    str.w r1, [r12, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #16
    bne 9b

    movs r10, #0
    add.w r12, r7, #64
10:
    ldr.w r0, [r12, r10, lsl #2]
    ldr.w r1, [r4, r10, lsl #2]
    eor.w r1, r1, r0
    str.w r1, [r4, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #16
    bne 10b

    add.w sp, sp, #256
    pop.w {r3-r11, pc}

###########################################################################
#### asymmetric 256x128 word-ring multiply                               ####
#### r0: res, 32 words                                                   ####
#### r1: a,   32 words                                                   ####
#### r2: b,   16 words; high 128 coefficients are implicit zeros         ####
###########################################################################
.align 2
.thumb_func
.global r2_radix16_mul_256x128_asm
.type r2_radix16_mul_256x128_asm, %function
r2_radix16_mul_256x128_asm:
    push.w {r3-r11, lr}
    sub.w sp, sp, #256

    mov r4, r0                  @ res, 32 words
    mov r5, r1                  @ a = a0 + x^128 a1, 32 words
    mov r6, r2                  @ b, 16 words
    mov r7, sp                  @ z0 = a0*b, 32 words
    add.w r8, sp, #128          @ z1 = a1*b, 32 words

    mov r0, r7
    mov r1, r5
    mov r2, r6
    bl r2_radix16_linear_mul16_asm

    mov r0, r8
    add.w r1, r5, #64
    mov r2, r6
    bl r2_radix16_linear_mul16_asm

    movs r10, #0
23:
    ldr.w r0, [r7, r10, lsl #2]
    add.w r12, r8, #64
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    str.w r0, [r4, r10, lsl #2]

    add.w r12, r7, #64
    ldr.w r0, [r12, r10, lsl #2]
    ldr.w r1, [r8, r10, lsl #2]
    eor.w r0, r0, r1
    add.w r12, r4, #64
    str.w r0, [r12, r10, lsl #2]

    adds r10, r10, #1
    cmp r10, #16
    bne 23b

    add.w sp, sp, #256
    pop.w {r3-r11, pc}

###########################################################################
#### linear full-product helpers                                         ####
#### r0: out, r1: a, r2: b                                               ####
###########################################################################
.align 2
.thumb_func
.type r2_radix16_linear_mul4_asm, %function
r2_radix16_linear_mul4_asm:
    push.w {r4-r11, lr}

    mov r4, r0                  @ out, 8 words
    mov r5, r1                  @ a, 4 words
    mov r6, r2                  @ b, 4 words
    movw r7, #0x1111
    movt r7, #0x1111

    mov r12, r4
    movs r11, #8
    movs r2, #0
11:
    str.w r2, [r12], #4
    subs r11, r11, #1
    bne 11b

    movs r8, #0
12:
    ldr.w r0, [r5, r8, lsl #2]
    movs r9, #0
    mov r10, r8
13:
    ldr.w r1, [r6, r9, lsl #2]
    umull r2, r3, r0, r1
    and.w r2, r2, r7
    and.w r3, r3, r7

    ldr.w r12, [r4, r10, lsl #2]
    eor.w r12, r12, r2
    str.w r12, [r4, r10, lsl #2]

    adds r1, r10, #1
    ldr.w r12, [r4, r1, lsl #2]
    eor.w r12, r12, r3
    str.w r12, [r4, r1, lsl #2]

    adds r10, r10, #1
    adds r9, r9, #1
    cmp r9, #4
    bne 13b

    adds r8, r8, #1
    cmp r8, #4
    bne 12b

    pop.w {r4-r11, pc}

.align 2
.thumb_func
.type r2_radix16_linear_mul8_asm, %function
r2_radix16_linear_mul8_asm:
    push.w {r3-r11, lr}
    sub.w sp, sp, #64

    mov r4, r0                  @ out, 16 words
    mov r5, r1                  @ a, 8 words
    mov r6, r2                  @ b, 8 words
    mov r7, sp                  @ p1, 8 words
    add.w r8, sp, #32           @ sum a0+a1, 4 words
    add.w r9, sp, #48           @ sum b0+b1, 4 words
    movw r11, #0x1111
    movt r11, #0x1111

    R2_LINEAR_MUL4_BODY r4, r5, r6

    add.w r7, r4, #32
    add.w r8, r5, #16
    add.w r9, r6, #16
    R2_LINEAR_MUL4_BODY r7, r8, r9

    mov r7, sp
    add.w r8, sp, #32
    add.w r9, sp, #48
    movs r10, #0
.Llin8_sum:
    ldr.w r0, [r5, r10, lsl #2]
    add.w r12, r5, #16
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    and.w r0, r0, r11
    str.w r0, [r8, r10, lsl #2]

    ldr.w r0, [r6, r10, lsl #2]
    add.w r12, r6, #16
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    and.w r0, r0, r11
    str.w r0, [r9, r10, lsl #2]

    adds r10, r10, #1
    cmp r10, #4
    bne .Llin8_sum

    R2_LINEAR_MUL4_BODY r7, r8, r9

    movs r10, #0
.Llin8_cross:
    ldr.w r0, [r7, r10, lsl #2]
    ldr.w r1, [r4, r10, lsl #2]
    add.w r12, r4, #32
    ldr.w r2, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    eor.w r0, r0, r2
    str.w r0, [r7, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #8
    bne .Llin8_cross

    movs r10, #0
.Llin8_fold:
    ldr.w r0, [r7, r10, lsl #2]
    add.w r12, r4, #16
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r1, r1, r0
    str.w r1, [r12, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #8
    bne .Llin8_fold

    add.w sp, sp, #64
    pop.w {r3-r11, pc}

.align 2
.thumb_func
.type r2_radix16_linear_mul16_asm, %function
r2_radix16_linear_mul16_asm:
    push.w {r3-r11, lr}
    sub.w sp, sp, #128

    mov r4, r0                  @ out, 32 words
    mov r5, r1                  @ a, 16 words
    mov r6, r2                  @ b, 16 words
    mov r7, sp                  @ p1, 16 words
    add.w r8, sp, #64           @ sum a0+a1, 8 words
    add.w r9, sp, #96           @ sum b0+b1, 8 words

    bl r2_radix16_linear_mul8_asm

    add.w r0, r4, #64
    add.w r1, r5, #32
    add.w r2, r6, #32
    bl r2_radix16_linear_mul8_asm

    movw r11, #0x1111
    movt r11, #0x1111
    movs r10, #0
17:
    ldr.w r0, [r5, r10, lsl #2]
    add.w r12, r5, #32
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    and.w r0, r0, r11
    str.w r0, [r8, r10, lsl #2]

    ldr.w r0, [r6, r10, lsl #2]
    add.w r12, r6, #32
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    and.w r0, r0, r11
    str.w r0, [r9, r10, lsl #2]

    adds r10, r10, #1
    cmp r10, #8
    bne 17b

    mov r0, r7
    mov r1, r8
    mov r2, r9
    bl r2_radix16_linear_mul8_asm

    movs r10, #0
18:
    ldr.w r0, [r7, r10, lsl #2]
    ldr.w r1, [r4, r10, lsl #2]
    add.w r12, r4, #64
    ldr.w r2, [r12, r10, lsl #2]
    eor.w r0, r0, r1
    eor.w r0, r0, r2
    str.w r0, [r7, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #16
    bne 18b

    movs r10, #0
19:
    ldr.w r0, [r7, r10, lsl #2]
    add.w r12, r4, #32
    ldr.w r1, [r12, r10, lsl #2]
    eor.w r1, r1, r0
    str.w r1, [r12, r10, lsl #2]
    adds r10, r10, #1
    cmp r10, #16
    bne 19b

    add.w sp, sp, #128
    pop.w {r3-r11, pc}

###########################################################################
#### shared word-ring multiply                                           ####
#### r0: res, r1: a, r2: b, r3: word count                              ####
###########################################################################
.align 2
.thumb_func
.type r2_radix16_mul_words_asm, %function
r2_radix16_mul_words_asm:
    push {r4-r11, lr}

    resbase     .req r4
    abase       .req r5
    bbase       .req r6
    words       .req r7
    iword       .req r8
    jword       .req r9
    ridx        .req r10
    lanemask    .req r11
    ai          .req r0
    bj          .req r1
    prodlo      .req r2
    prodhi      .req r3
    tmp         .req r12

    mov resbase, r0
    mov abase, r1
    mov bbase, r2
    mov words, r3

    movw lanemask, #0x1111
    movt lanemask, #0x1111

    mov tmp, resbase
    movs prodlo, #0
    mov prodhi, words
20:
    str.w prodlo, [tmp], #4
    subs prodhi, prodhi, #1
    bne 20b

    movs iword, #0
21:
    ldr.w ai, [abase, iword, lsl #2]
    movs jword, #0
    mov ridx, iword
22:
    ldr.w bj, [bbase, jword, lsl #2]
    ldr.w tmp, [resbase, ridx, lsl #2]
    umull prodlo, prodhi, ai, bj
    and.w prodlo, prodlo, lanemask
    and.w prodhi, prodhi, lanemask

    eor.w tmp, tmp, prodlo
    str.w tmp, [resbase, ridx, lsl #2]

    adds tmp, ridx, #1
    subs prodlo, tmp, words
    sbc.w prodlo, prodlo, prodlo
    and.w tmp, tmp, prodlo
    ldr.w prodlo, [resbase, tmp, lsl #2]
    eor.w prodlo, prodlo, prodhi
    str.w prodlo, [resbase, tmp, lsl #2]

    adds ridx, ridx, #1
    subs prodlo, ridx, words
    sbc.w prodlo, prodlo, prodlo
    and.w ridx, ridx, prodlo

    adds jword, jword, #1
    cmp jword, words
    bne 22b

    adds iword, iword, #1
    cmp iword, words
    bne 21b

    pop {r4-r11, pc}

    .unreq resbase
    .unreq abase
    .unreq bbase
    .unreq words
    .unreq iword
    .unreq jword
    .unreq ridx
    .unreq lanemask
    .unreq ai
    .unreq bj
    .unreq prodlo
    .unreq prodhi
    .unreq tmp





// int poly_xor4_radix16_asm(uint32_t r[16], const int16_t a[512])
// Folds modulo x^(N/4)+1, packs 8 binary coefficients per word into
// nibble lanes 0,4,...,28, and returns the Z2 non-invertibility flag.
.align 2
.global poly_xor4_radix16_asm
.type poly_xor4_radix16_asm, %function
poly_xor4_radix16_asm:
    push {r4-r11, r14}

    rptr    .req r0
    aptr    .req r1
    out     .req r2
    fold    .req r3
    acc     .req r4
    scale   .req r5
    poly0   .req r6
    poly1   .req r7
    poly2   .req r8
    poly3   .req r9
    tmp     .req r10
    loop    .req r11
    tmp2    .req r12

    movw scale, #1
    movt scale, #1
    movs acc, #0
    movw loop, #16
    .equ xor4_radix16_stride_128, 128*2

1:
    ldr.w poly0, [aptr, #0+xor4_radix16_stride_128*0]
    ldr.w poly1, [aptr, #0+xor4_radix16_stride_128*1]
    ldr.w poly2, [aptr, #0+xor4_radix16_stride_128*2]
    ldr.w poly3, [aptr, #0+xor4_radix16_stride_128*3]
    eor fold, poly0, poly1
    eor tmp, poly2, poly3
    eor fold, fold, tmp
    and fold, fold, scale
    and out, fold, #1
    orr out, out, fold, lsr #12
    and out, out, #0x11

    ldr.w poly0, [aptr, #4+xor4_radix16_stride_128*0]
    ldr.w poly1, [aptr, #4+xor4_radix16_stride_128*1]
    ldr.w poly2, [aptr, #4+xor4_radix16_stride_128*2]
    ldr.w poly3, [aptr, #4+xor4_radix16_stride_128*3]
    eor fold, poly0, poly1
    eor tmp, poly2, poly3
    eor fold, fold, tmp
    and fold, fold, scale
    and tmp2, fold, #1
    orr tmp2, tmp2, fold, lsr #12
    and tmp2, tmp2, #0x11
    orr out, out, tmp2, lsl #8

    ldr.w poly0, [aptr, #8+xor4_radix16_stride_128*0]
    ldr.w poly1, [aptr, #8+xor4_radix16_stride_128*1]
    ldr.w poly2, [aptr, #8+xor4_radix16_stride_128*2]
    ldr.w poly3, [aptr, #8+xor4_radix16_stride_128*3]
    eor fold, poly0, poly1
    eor tmp, poly2, poly3
    eor fold, fold, tmp
    and fold, fold, scale
    and tmp2, fold, #1
    orr tmp2, tmp2, fold, lsr #12
    and tmp2, tmp2, #0x11
    orr out, out, tmp2, lsl #16

    ldr.w poly0, [aptr, #12+xor4_radix16_stride_128*0]
    ldr.w poly1, [aptr, #12+xor4_radix16_stride_128*1]
    ldr.w poly2, [aptr, #12+xor4_radix16_stride_128*2]
    ldr.w poly3, [aptr, #12+xor4_radix16_stride_128*3]
    eor fold, poly0, poly1
    eor tmp, poly2, poly3
    eor fold, fold, tmp
    and fold, fold, scale
    and tmp2, fold, #1
    orr tmp2, tmp2, fold, lsr #12
    and tmp2, tmp2, #0x11
    orr out, out, tmp2, lsl #24

    str.w out, [rptr], #4
    eor acc, acc, out
    add aptr, aptr, #16
    subs loop, #1
    bne 1b

    eor acc, acc, acc, lsr #16
    eor acc, acc, acc, lsr #8
    eor acc, acc, acc, lsr #4
    and acc, acc, #1
    eor r0, acc, #1
    pop {r4-r11, pc}

.unreq rptr
.unreq aptr
.unreq out
.unreq fold
.unreq acc
.unreq scale
.unreq poly0
.unreq poly1
.unreq poly2
.unreq poly3
.unreq tmp
.unreq loop
.unreq tmp2
