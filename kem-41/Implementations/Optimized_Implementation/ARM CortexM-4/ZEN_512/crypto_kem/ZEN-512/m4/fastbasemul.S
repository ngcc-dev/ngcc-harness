/**
 * ZEN_512 16-coefficient NTT base multiplication.
 *
 * Each block is multiplied modulo x^16 = zeta.  The block products are
 * computed with macro-expanded Karatsuba kernels.  Product coefficients stay
 * in high-half Plantard form until the final store, matching the existing
 * public ABI:
 *   basemul16_karatsuba_asm(r, a, b, zetas_769)
 *   basemul16_karatsuba_mq_asm(r, a, b, zetas_769)
 */

#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

.equ BM_P0, 0
.equ BM_P1, 60
.equ BM_PM, 120
.equ BM_AS, 180
.equ BM_BS, 196
.equ BM_TMP, 212
.equ BM_SAVE_R, 240
.equ BM_SAVE_A, 244
.equ BM_SAVE_B, 248
.equ BM_SAVE_Z, 252
.equ BM_FRAME_SIZE, 256

.equ BI_PE15, 0
.equ BI_PO15, 32
.equ BI_AE, 64
.equ BI_AO, 80
.equ BI_H, 96
.equ BI_HE, 112
.equ BI_HO, 120
.equ BI_PE7, 128
.equ BI_PO7, 144
.equ BI_B, 160
.equ BI_C, 168
.equ BI_F, 172
.equ BI_G, 180
.equ BI_AS, 196
.equ BI_BS, 212
.equ BI_TMP, 228
.equ BI_SAVE_R, 244
.equ BI_SAVE_A, 248
.equ BI_SAVE_Z, 252
.equ BI_SAVE_QTAB, 256
.equ BI_FRAME_SIZE, 264

.macro make_sum8 as, bs, a, b, t0, t1
    ldr.w \t0, [\a, #0]
    ldr.w \t1, [\a, #16]
    sadd16.w \t0, \t0, \t1
    str.w \t0, [\as, #0]
    ldr.w \t0, [\b, #0]
    ldr.w \t1, [\b, #16]
    sadd16.w \t0, \t0, \t1
    str.w \t0, [\bs, #0]

    ldr.w \t0, [\a, #0+4*1]
    ldr.w \t1, [\a, #16+4*1]
    sadd16.w \t0, \t0, \t1
    str.w \t0, [\as, #0+4*1]
    ldr.w \t0, [\b, #0+4*1]
    ldr.w \t1, [\b, #16+4*1]
    sadd16.w \t0, \t0, \t1
    str.w \t0, [\bs, #0+4*1]

    ldr.w \t0, [\a, #0+4*2]
    ldr.w \t1, [\a, #16+4*2]
    sadd16.w \t0, \t0, \t1
    str.w \t0, [\as, #0+4*2]
    ldr.w \t0, [\b, #0+4*2]
    ldr.w \t1, [\b, #16+4*2]
    sadd16.w \t0, \t0, \t1
    str.w \t0, [\bs, #0+4*2]

    ldr.w \t0, [\a, #0+4*3]
    ldr.w \t1, [\a, #16+4*3]
    sadd16.w \t0, \t0, \t1
    str.w \t0, [\as, #0+4*3]
    ldr.w \t0, [\b, #0+4*3]
    ldr.w \t1, [\b, #16+4*3]
    sadd16.w \t0, \t0, \t1
    str.w \t0, [\bs, #0+4*3]
.endm

.macro compute_block_products
    str.w r0, [sp, #BM_SAVE_R]
    str.w r1, [sp, #BM_SAVE_A]
    str.w r2, [sp, #BM_SAVE_B]
    str.w r3, [sp, #BM_SAVE_Z]

    movw r8, #0
    movt r8, #769       @ q in the top halfword
    movw r9, #64769
    movt r9, #58632     @ Plantard qinv
    movw r10, #24608    @ qa

    add.w r0, sp, #BM_P0
    ldr.w r1, [sp, #BM_SAVE_A]
    ldr.w r2, [sp, #BM_SAVE_B]
    add.w r3, sp, #BM_TMP
    add.w r11, sp, #BM_AS
    add.w r12, sp, #BM_BS
    mul8_plant_hh r0, r1, r2, r3, r11, r12, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BM_P1
    ldr.w r1, [sp, #BM_SAVE_A]
    add.w r1, r1, #16
    ldr.w r2, [sp, #BM_SAVE_B]
    add.w r2, r2, #16
    add.w r3, sp, #BM_TMP
    add.w r11, sp, #BM_AS
    add.w r12, sp, #BM_BS
    mul8_plant_hh r0, r1, r2, r3, r11, r12, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r11, sp, #BM_AS
    add.w r12, sp, #BM_BS
    ldr.w r1, [sp, #BM_SAVE_A]
    ldr.w r2, [sp, #BM_SAVE_B]
    make_sum8 r11, r12, r1, r2, r4, r5

    add.w r0, sp, #BM_PM
    mov r1, r11
    mov r2, r12
    add.w r3, sp, #BM_TMP
    mul8_plant_hh r0, r1, r2, r3, r1, r2, r8, r10, r9, r4, r5, r6, r7, lr
.endm

.macro setup_combine_ptrs
    add.w r0, sp, #BM_P0
    add.w r1, sp, #BM_P1
    add.w r2, sp, #BM_PM
    ldr.w r3, [sp, #BM_SAVE_R]
    ldr.w r7, [sp, #BM_SAVE_Z]
.endm

.macro cross_pair dst, idx, tmp
    ldr.w \dst, [r2, #(\idx * 2)]
    ldr.w \tmp, [r0, #(\idx * 2)]
    ssub16 \dst, \dst, \tmp
    ldr.w \tmp, [r1, #(\idx * 2)]
    ssub16 \dst, \dst, \tmp
.endm

.macro cross_lane dst, idx, tmp
    ldrh.w \dst, [r2, #(\idx * 2)]
    ldrh.w \tmp, [r0, #(\idx * 2)]
    ssub16 \dst, \dst, \tmp
    ldrh.w \tmp, [r1, #(\idx * 2)]
    ssub16 \dst, \dst, \tmp
.endm

.macro store_mq_pair val, idx, tmp, qtmp
    mul_twiddle_plant_mq \val, r11, \tmp, r8, r10, \qtmp
    str.w \val, [r3, #(\idx * 2)]
.endm

.macro store_mq_lane val, idx, tmp, qtmp
    smulwb \val, r11, \val
    smlabt \val, \val, r8, r10
    and \qtmp, r8, \val, asr #31
    sadd16 \val, \val, \qtmp
    asr \val, \val, #16
    strh.w \val, [r3, #(\idx * 2)]
.endm

.macro combine_low_pair_raw idx
    ldr.w r4, [r0, #(\idx * 2)]
    ldr.w r5, [r1, #(\idx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    cross_pair r6, (\idx + 8), r12
    mul_twiddle_plant r6, r7, r12, r8, r10
    sadd16 r4, r4, r5
    sadd16 r4, r4, r6
    str.w r4, [r3, #(\idx * 2)]
.endm

.macro combine_low_pair_mq idx
    ldr.w r4, [r0, #(\idx * 2)]
    ldr.w r5, [r1, #(\idx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    cross_pair r6, (\idx + 8), r12
    mul_twiddle_plant r6, r7, r12, r8, r10
    sadd16 r4, r4, r5
    sadd16 r4, r4, r6
    store_mq_pair r4, \idx, r5, r12
.endm

.macro combine_low_lane_raw idx
    ldrh.w r4, [r0, #(\idx * 2)]
    ldrh.w r5, [r1, #(\idx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    cross_lane r6, (\idx + 8), r12
    mul_twiddle_plant r6, r7, r12, r8, r10
    sadd16 r4, r4, r5
    sadd16 r4, r4, r6
    strh.w r4, [r3, #(\idx * 2)]
.endm

.macro combine_low_lane_mq idx
    ldrh.w r4, [r0, #(\idx * 2)]
    ldrh.w r5, [r1, #(\idx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    cross_lane r6, (\idx + 8), r12
    mul_twiddle_plant r6, r7, r12, r8, r10
    sadd16 r4, r4, r5
    sadd16 r4, r4, r6
    store_mq_lane r4, \idx, r5, r12
.endm

.macro combine_7_raw
    ldrh.w r4, [r0, #(7 * 2)]
    ldrh.w r5, [r1, #(7 * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    sadd16 r4, r4, r5
    strh.w r4, [r3, #(7 * 2)]
.endm

.macro combine_7_mq
    ldrh.w r4, [r0, #(7 * 2)]
    ldrh.w r5, [r1, #(7 * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    sadd16 r4, r4, r5
    store_mq_lane r4, 7, r5, r12
.endm

.macro combine_high_pair_raw idx
    ldr.w r4, [r0, #(\idx * 2)]
    ldr.w r5, [r1, #(\idx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    cross_pair r6, (\idx - 8), r12
    sadd16 r4, r4, r5
    sadd16 r4, r4, r6
    str.w r4, [r3, #(\idx * 2)]
.endm

.macro combine_high_pair_mq idx
    ldr.w r4, [r0, #(\idx * 2)]
    ldr.w r5, [r1, #(\idx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    cross_pair r6, (\idx - 8), r12
    sadd16 r4, r4, r5
    sadd16 r4, r4, r6
    store_mq_pair r4, \idx, r5, r12
.endm

.macro combine_high_lane_raw idx
    ldrh.w r4, [r0, #(\idx * 2)]
    ldrh.w r5, [r1, #(\idx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    cross_lane r6, (\idx - 8), r12
    sadd16 r4, r4, r5
    sadd16 r4, r4, r6
    strh.w r4, [r3, #(\idx * 2)]
.endm

.macro combine_high_lane_mq idx
    ldrh.w r4, [r0, #(\idx * 2)]
    ldrh.w r5, [r1, #(\idx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    cross_lane r6, (\idx - 8), r12
    sadd16 r4, r4, r5
    sadd16 r4, r4, r6
    store_mq_lane r4, \idx, r5, r12
.endm

.macro combine_15_raw
    cross_lane r4, 7, r12
    strh.w r4, [r3, #(15 * 2)]
.endm

.macro combine_15_mq
    cross_lane r4, 7, r12
    store_mq_lane r4, 15, r5, r12
.endm

.macro split_evenodd16 ae, ao, a, t0, t1, t2, t3
    ldr.w \t0, [\a, #0]
    ldr.w \t1, [\a, #4]
    pkhbt \t2, \t0, \t1, lsl #16
    pkhtb \t3, \t1, \t0, asr #16
    str.w \t2, [\ae, #0]
    str.w \t3, [\ao, #0]

    ldr.w \t0, [\a, #8]
    ldr.w \t1, [\a, #12]
    pkhbt \t2, \t0, \t1, lsl #16
    pkhtb \t3, \t1, \t0, asr #16
    str.w \t2, [\ae, #4]
    str.w \t3, [\ao, #4]

    ldr.w \t0, [\a, #16]
    ldr.w \t1, [\a, #20]
    pkhbt \t2, \t0, \t1, lsl #16
    pkhtb \t3, \t1, \t0, asr #16
    str.w \t2, [\ae, #8]
    str.w \t3, [\ao, #8]

    ldr.w \t0, [\a, #24]
    ldr.w \t1, [\a, #28]
    pkhbt \t2, \t0, \t1, lsl #16
    pkhtb \t3, \t1, \t0, asr #16
    str.w \t2, [\ae, #12]
    str.w \t3, [\ao, #12]
.endm

.macro split_evenodd8 he, ho, h, t0, t1, t2, t3
    ldr.w \t0, [\h, #0]
    ldr.w \t1, [\h, #4]
    pkhbt \t2, \t0, \t1, lsl #16
    pkhtb \t3, \t1, \t0, asr #16
    str.w \t2, [\he, #0]
    str.w \t3, [\ho, #0]

    ldr.w \t0, [\h, #8]
    ldr.w \t1, [\h, #12]
    pkhbt \t2, \t0, \t1, lsl #16
    pkhtb \t3, \t1, \t0, asr #16
    str.w \t2, [\he, #4]
    str.w \t3, [\ho, #4]
.endm

.macro h_recompose_0 pe, po, h
    ldrh.w r4, [\pe, #(0 * 2)]
    ldrh.w r5, [\pe, #(8 * 2)]
    ldrh.w r6, [\po, #(7 * 2)]
    ssub16 r5, r5, r6
    mul_twiddle_plant r5, r7, r12, r8, r10
    ssub16 r4, r4, r5
    strh.w r4, [\h, #(0 * 2)]
.endm

.macro h_recompose_mid pe, po, h, idx
    ldrh.w r4, [\pe, #(\idx * 2)]
    ldrh.w r6, [\po, #((\idx - 1) * 2)]
    ssub16 r4, r4, r6
    ldrh.w r5, [\pe, #((\idx + 8) * 2)]
    ldrh.w r6, [\po, #((\idx + 7) * 2)]
    ssub16 r5, r5, r6
    mul_twiddle_plant r5, r7, r12, r8, r10
    ssub16 r4, r4, r5
    strh.w r4, [\h, #(\idx * 2)]
.endm

.macro h_recompose_7 pe, po, h
    ldrh.w r4, [\pe, #(7 * 2)]
    ldrh.w r6, [\po, #(6 * 2)]
    ssub16 r4, r4, r6
    ldrh.w r5, [\po, #(14 * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    sadd16 r4, r4, r5
    strh.w r4, [\h, #(7 * 2)]
.endm

.macro b_recompose_0 pe, po, b
    ldrh.w r4, [\pe, #(0 * 2)]
    ldrh.w r5, [\pe, #(4 * 2)]
    ldrh.w r6, [\po, #(3 * 2)]
    ssub16 r5, r5, r6
    mul_twiddle_plant r5, r7, r12, r8, r10
    ssub16 r4, r4, r5
    strh.w r4, [\b, #(0 * 2)]
.endm

.macro b_recompose_mid pe, po, b, idx
    ldrh.w r4, [\pe, #(\idx * 2)]
    ldrh.w r6, [\po, #((\idx - 1) * 2)]
    ssub16 r4, r4, r6
    ldrh.w r5, [\pe, #((\idx + 4) * 2)]
    ldrh.w r6, [\po, #((\idx + 3) * 2)]
    ssub16 r5, r5, r6
    mul_twiddle_plant r5, r7, r12, r8, r10
    ssub16 r4, r4, r5
    strh.w r4, [\b, #(\idx * 2)]
.endm

.macro b_recompose_3 pe, po, b
    ldrh.w r4, [\pe, #(3 * 2)]
    ldrh.w r6, [\po, #(2 * 2)]
    ssub16 r4, r4, r6
    ldrh.w r5, [\po, #(6 * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    sadd16 r4, r4, r5
    strh.w r4, [\b, #(3 * 2)]
.endm

.macro g_even pe, g, ridx, loidx, hiidx
    ldrh.w r4, [\pe, #(\loidx * 2)]
    ldrh.w r5, [\pe, #(\hiidx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    ssub16 r4, r4, r5
    strh.w r4, [\g, #(\ridx * 2)]
.endm

.macro g_odd po, g, ridx, loidx, hiidx
    ldrh.w r4, [\po, #(\hiidx * 2)]
    mul_twiddle_plant r4, r7, r12, r8, r10
    ldrh.w r5, [\po, #(\loidx * 2)]
    ssub16 r4, r4, r5
    strh.w r4, [\g, #(\ridx * 2)]
.endm

.macro final_even pe, out, ridx, loidx, hiidx
    ldrh.w r4, [\pe, #(\loidx * 2)]
    ldrh.w r5, [\pe, #(\hiidx * 2)]
    mul_twiddle_plant r5, r7, r12, r8, r10
    ssub16 r4, r4, r5
    strh.w r4, [\out, #(\ridx * 2)]
.endm

.macro final_odd po, out, ridx, loidx, hiidx
    ldrh.w r4, [\po, #(\hiidx * 2)]
    mul_twiddle_plant r4, r7, r12, r8, r10
    ldrh.w r5, [\po, #(\loidx * 2)]
    ssub16 r4, r4, r5
    strh.w r4, [\out, #(\ridx * 2)]
.endm

.align 2
.thumb_func
.type basemul16_block_raw, %function
basemul16_block_raw:
    push.w {r4-r11, lr}
    sub.w sp, sp, #BM_FRAME_SIZE

    compute_block_products
    setup_combine_ptrs

    combine_low_pair_raw 0
    combine_low_pair_raw 2
    combine_low_pair_raw 4
    combine_low_lane_raw 6
    combine_7_raw
    combine_high_pair_raw 8
    combine_high_pair_raw 10
    combine_high_pair_raw 12
    combine_high_lane_raw 14
    combine_15_raw

    add.w sp, sp, #BM_FRAME_SIZE
    pop.w {r4-r11, pc}

.align 2
.thumb_func
.type basemul16_block_raw_mq, %function
basemul16_block_raw_mq:
    push.w {r4-r11, lr}
    sub.w sp, sp, #BM_FRAME_SIZE

    compute_block_products
    setup_combine_ptrs
    movw r11, #50793
    movt r11, #63916    @ beta^2*qinv conversion to normal domain

    combine_low_pair_mq 0
    combine_low_pair_mq 2
    combine_low_pair_mq 4
    combine_low_lane_mq 6
    combine_7_mq
    combine_high_pair_mq 8
    combine_high_pair_mq 10
    combine_high_pair_mq 12
    combine_high_lane_mq 14
    combine_15_mq

    add.w sp, sp, #BM_FRAME_SIZE
    pop.w {r4-r11, pc}

.align 2
.thumb_func
.type baseinv16_block_plant, %function
baseinv16_block_plant:
    push.w {r4-r11, lr}
    sub.w sp, sp, #BI_FRAME_SIZE

    str.w r0, [sp, #BI_SAVE_R]
    str.w r1, [sp, #BI_SAVE_A]
    str.w r2, [sp, #BI_SAVE_Z]
    str.w r3, [sp, #BI_SAVE_QTAB]

    movw r8, #0
    movt r8, #769
    movw r9, #64769
    movt r9, #58632
    movw r10, #24608

    add.w r11, sp, #BI_AE
    add.w r12, sp, #BI_AO
    ldr.w r0, [sp, #BI_SAVE_A]
    split_evenodd16 r11, r12, r0, r4, r5, r6, r7

    add.w r0, sp, #BI_PE15
    add.w r1, sp, #BI_AE
    mov r2, r1
    add.w r3, sp, #BI_TMP
    add.w r11, sp, #BI_AS
    add.w r12, sp, #BI_BS
    mul8_plant_hh r0, r1, r2, r3, r11, r12, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BI_PO15
    add.w r1, sp, #BI_AO
    mov r2, r1
    add.w r3, sp, #BI_TMP
    add.w r11, sp, #BI_AS
    add.w r12, sp, #BI_BS
    mul8_plant_hh r0, r1, r2, r3, r11, r12, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BI_PE15
    add.w r1, sp, #BI_PO15
    add.w r2, sp, #BI_H
    ldr.w r7, [sp, #BI_SAVE_Z]
    h_recompose_0 r0, r1, r2
    h_recompose_mid r0, r1, r2, 1
    h_recompose_mid r0, r1, r2, 2
    h_recompose_mid r0, r1, r2, 3
    h_recompose_mid r0, r1, r2, 4
    h_recompose_mid r0, r1, r2, 5
    h_recompose_mid r0, r1, r2, 6
    h_recompose_7 r0, r1, r2

    add.w r11, sp, #BI_HE
    add.w r12, sp, #BI_HO
    add.w r0, sp, #BI_H
    split_evenodd8 r11, r12, r0, r4, r5, r6, r7

    add.w r0, sp, #BI_PE7
    add.w r1, sp, #BI_HE
    mov r2, r1
    add.w r3, sp, #BI_TMP
    karatsuba_mul4_plant_hh r0, 0, r1, 0, r2, 0, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BI_PO7
    add.w r1, sp, #BI_HO
    mov r2, r1
    add.w r3, sp, #BI_TMP
    karatsuba_mul4_plant_hh r0, 0, r1, 0, r2, 0, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BI_PE7
    add.w r1, sp, #BI_PO7
    add.w r2, sp, #BI_B
    ldr.w r7, [sp, #BI_SAVE_Z]
    b_recompose_0 r0, r1, r2
    b_recompose_mid r0, r1, r2, 1
    b_recompose_mid r0, r1, r2, 2
    b_recompose_3 r0, r1, r2

    add.w r0, sp, #BI_B

    ldrh.w r11, [r0, #(0 * 2)]
    smulbb r4, r11, r11
    plant_red r8, r10, r9, r4

    ldrh.w r11, [r0, #(2 * 2)]
    smulbb r5, r11, r11
    plant_red r8, r10, r9, r5

    ldrh.w r11, [r0, #(0 * 2)]
    ldrh.w r12, [r0, #(2 * 2)]
    smulbb r6, r11, r12
    plant_red r8, r10, r9, r6
    sadd16 r6, r6, r6

    ldrh.w r11, [r0, #(1 * 2)]
    smulbb r11, r11, r11
    plant_red r8, r10, r9, r11

    ldrh.w r12, [r0, #(3 * 2)]
    smulbb r12, r12, r12
    plant_red r8, r10, r9, r12

    ldrh.w r2, [r0, #(1 * 2)]
    ldrh.w r3, [r0, #(3 * 2)]
    smulbb lr, r2, r3
    plant_red r8, r10, r9, lr
    sadd16 lr, lr, lr

    ssub16 r5, r5, lr
    ldr.w r7, [sp, #BI_SAVE_Z]
    mul_zeta_hh r5, r7, r8, r10
    ssub16 r4, r4, r5

    ssub16 r6, r6, r11
    mul_zeta_hh r12, r7, r8, r10
    sadd16 r6, r6, r12

    add.w r0, sp, #BI_C
    asr.w r2, r4, #16
    strh.w r2, [r0, #(0 * 2)]
    asr.w r2, r6, #16
    strh.w r2, [r0, #(1 * 2)]

    ldrh.w r2, [r0, #(1 * 2)]
    smulbb r5, r2, r2
    plant_red r8, r10, r9, r5
    mul_zeta_hh r5, r7, r8, r10

    ldrh.w r2, [r0, #(0 * 2)]
    smulbb r4, r2, r2
    plant_red r8, r10, r9, r4
    sadd16 r5, r5, r4

    sxth.w r5, r5, ror #16
    movw r6, #769
    and.w r4, r6, r5, asr #31
    add.w r5, r5, r4
    ldr.w r11, [sp, #BI_SAVE_QTAB]
    add.w r11, r11, r5, lsl #2
    ldr.w r11, [r11]

    ldrh.w r4, [r0, #(0 * 2)]
    smulwb r4, r11, r4
    smlabt r4, r4, r8, r10

    ldrh.w r6, [r0, #(1 * 2)]
    neg.w r11, r11
    smulwb r6, r11, r6
    smlabt r6, r6, r8, r10

    asr.w r4, r4, #16
    strh.w r4, [r0, #(0 * 2)]
    asr.w r6, r6, #16
    strh.w r6, [r0, #(1 * 2)]

    add.w r0, sp, #BI_C
    add.w r1, sp, #BI_B
    add.w r2, sp, #BI_F
    ldr.w r7, [sp, #BI_SAVE_Z]

    ldrh.w r4, [r0, #(1 * 2)]
    ldrh.w r5, [r1, #(2 * 2)]
    smulbb r6, r4, r5
    plant_red r8, r10, r9, r6
    mul_zeta_hh r6, r7, r8, r10
    ldrh.w r4, [r0, #(0 * 2)]
    ldrh.w r5, [r1, #(0 * 2)]
    smulbb r4, r4, r5
    plant_red r8, r10, r9, r4
    ssub16 r4, r4, r6
    asr.w r4, r4, #16
    strh.w r4, [r2, #(0 * 2)]

    ldrh.w r4, [r0, #(1 * 2)]
    ldrh.w r5, [r1, #(3 * 2)]
    smulbb r6, r4, r5
    plant_red r8, r10, r9, r6
    mul_zeta_hh r6, r7, r8, r10
    ldrh.w r4, [r0, #(0 * 2)]
    ldrh.w r5, [r1, #(1 * 2)]
    smulbb r4, r4, r5
    plant_red r8, r10, r9, r4
    ssub16 r6, r6, r4
    asr.w r6, r6, #16
    strh.w r6, [r2, #(1 * 2)]

    ldrh.w r4, [r0, #(0 * 2)]
    ldrh.w r5, [r1, #(2 * 2)]
    smulbb r6, r4, r5
    plant_red r8, r10, r9, r6
    ldrh.w r4, [r0, #(1 * 2)]
    ldrh.w r5, [r1, #(0 * 2)]
    smulbb r4, r4, r5
    plant_red r8, r10, r9, r4
    sadd16 r6, r6, r4
    asr.w r6, r6, #16
    strh.w r6, [r2, #(2 * 2)]

    ldrh.w r4, [r0, #(0 * 2)]
    ldrh.w r5, [r1, #(3 * 2)]
    smulbb r6, r4, r5
    ldrh.w r4, [r0, #(1 * 2)]
    ldrh.w r5, [r1, #(1 * 2)]
    smlabb r6, r4, r5, r6
    neg.w r6, r6
    plant_red r8, r10, r9, r6
    asr.w r6, r6, #16
    strh.w r6, [r2, #(3 * 2)]

    add.w r0, sp, #BI_PE7
    add.w r1, sp, #BI_F
    add.w r2, sp, #BI_HE
    add.w r3, sp, #BI_TMP
    karatsuba_mul4_plant_hh r0, 0, r1, 0, r2, 0, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BI_PO7
    add.w r1, sp, #BI_F
    add.w r2, sp, #BI_HO
    add.w r3, sp, #BI_TMP
    karatsuba_mul4_plant_hh r0, 0, r1, 0, r2, 0, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BI_PE7
    add.w r1, sp, #BI_PO7
    add.w r2, sp, #BI_G
    ldr.w r7, [sp, #BI_SAVE_Z]
    g_even r0, r2, 0, 0, 4
    g_odd r1, r2, 1, 0, 4
    g_even r0, r2, 2, 1, 5
    g_odd r1, r2, 3, 1, 5
    g_even r0, r2, 4, 2, 6
    g_odd r1, r2, 5, 2, 6
    ldrh.w r4, [r0, #(3 * 2)]
    strh.w r4, [r2, #(6 * 2)]
    ldrh.w r4, [r1, #(3 * 2)]
    sxth r4, r4
    neg.w r4, r4
    strh.w r4, [r2, #(7 * 2)]

    add.w r0, sp, #BI_PE15
    add.w r1, sp, #BI_G
    add.w r2, sp, #BI_AE
    add.w r3, sp, #BI_TMP
    add.w r11, sp, #BI_AS
    add.w r12, sp, #BI_BS
    mul8_plant_hh r0, r1, r2, r3, r11, r12, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BI_PO15
    add.w r1, sp, #BI_G
    add.w r2, sp, #BI_AO
    add.w r3, sp, #BI_TMP
    add.w r11, sp, #BI_AS
    add.w r12, sp, #BI_BS
    mul8_plant_hh r0, r1, r2, r3, r11, r12, r8, r10, r9, r4, r5, r6, r7, lr

    add.w r0, sp, #BI_PE15
    add.w r1, sp, #BI_PO15
    ldr.w r2, [sp, #BI_SAVE_R]
    ldr.w r7, [sp, #BI_SAVE_Z]
    final_even r0, r2, 0, 0, 8
    final_odd r1, r2, 1, 0, 8
    final_even r0, r2, 2, 1, 9
    final_odd r1, r2, 3, 1, 9
    final_even r0, r2, 4, 2, 10
    final_odd r1, r2, 5, 2, 10
    final_even r0, r2, 6, 3, 11
    final_odd r1, r2, 7, 3, 11
    final_even r0, r2, 8, 4, 12
    final_odd r1, r2, 9, 4, 12
    final_even r0, r2, 10, 5, 13
    final_odd r1, r2, 11, 5, 13
    final_even r0, r2, 12, 6, 14
    final_odd r1, r2, 13, 6, 14
    ldrh.w r4, [r0, #(7 * 2)]
    strh.w r4, [r2, #(14 * 2)]
    ldrh.w r4, [r1, #(7 * 2)]
    sxth r4, r4
    neg.w r4, r4
    strh.w r4, [r2, #(15 * 2)]

    add.w sp, sp, #BI_FRAME_SIZE
    pop.w {r4-r11, pc}

.align 2
.thumb_func
.global baseinv16_karatsuba_asm
.type baseinv16_karatsuba_asm, %function
baseinv16_karatsuba_asm:
    push.w {r4-r8, lr}

    mov r4, r0
    mov r5, r1
    mov r6, r2
    mov r7, r3
    movs r8, #64

.Lbaseinv16_poly_loop:
    ldr.w r2, [r6], #4
    neg.w r2, r2
    mov r0, r4
    mov r1, r5
    mov r3, r7
    bl baseinv16_block_plant

    adds r4, r4, #32
    adds r5, r5, #32

    ldr.w r2, [r6, #-4]
    mov r0, r4
    mov r1, r5
    mov r3, r7
    bl baseinv16_block_plant

    adds r4, r4, #32
    adds r5, r5, #32
    subs r8, r8, #1
    bne.w .Lbaseinv16_poly_loop

    pop.w {r4-r8, pc}

.align 2
.thumb_func
.global basemul16_karatsuba_asm
.type basemul16_karatsuba_asm, %function
basemul16_karatsuba_asm:
    push.w {r4-r8, lr}

    mov r4, r0          @ r pointer
    mov r5, r1          @ a pointer
    mov r6, r2          @ b pointer
    mov r7, r3          @ Plantard zeta table, zetas_769
    movs r8, #64        @ ZEN_N / 32

.Lraw_poly_loop:
    ldr.w r3, [r7], #4
    mov r0, r4
    mov r1, r5
    mov r2, r6
    bl basemul16_block_raw

    adds r4, r4, #32
    adds r5, r5, #32
    adds r6, r6, #32

    ldr.w r3, [r7, #-4]
    neg.w r3, r3
    mov r0, r4
    mov r1, r5
    mov r2, r6
    bl basemul16_block_raw

    adds r4, r4, #32
    adds r5, r5, #32
    adds r6, r6, #32
    subs r8, r8, #1
    bne.w .Lraw_poly_loop

    pop.w {r4-r8, pc}

.align 2
.thumb_func
.global basemul16_karatsuba_mq_asm
.type basemul16_karatsuba_mq_asm, %function
basemul16_karatsuba_mq_asm:
    push.w {r4-r8, lr}

    mov r4, r0          @ r pointer
    mov r5, r1          @ a pointer
    mov r6, r2          @ b pointer
    mov r7, r3          @ Plantard zeta table, zetas_769
    movs r8, #64        @ ZEN_N / 32

.Lmq_poly_loop:
    ldr.w r3, [r7], #4
    mov r0, r4
    mov r1, r5
    mov r2, r6
    bl basemul16_block_raw_mq

    adds r4, r4, #32
    adds r5, r5, #32
    adds r6, r6, #32

    ldr.w r3, [r7, #-4]
    neg.w r3, r3
    mov r0, r4
    mov r1, r5
    mov r2, r6
    bl basemul16_block_raw_mq

    adds r4, r4, #32
    adds r5, r5, #32
    adds r6, r6, #32
    subs r8, r8, #1
    bne.w .Lmq_poly_loop

    pop.w {r4-r8, pc}
