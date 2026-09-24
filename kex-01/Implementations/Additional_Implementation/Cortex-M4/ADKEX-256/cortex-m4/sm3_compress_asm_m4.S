/******************************************************************************
* Optimized SM3 bit compress function for ARM Cortex-M4. Utilize floating-point
* registers. Do not use it on architectures without FP registers.
*
* @author   Junhao Huang, Singapore Management University, Singapre.
*           jhhuang_nuaa@126.com
*
* @date     May 2026
******************************************************************************/

.syntax unified
.cpu cortex-m4
.thumb
.altmacro
//  Code size: 8060 Bytes
.macro load_s reg, i
    vmov \reg, s\i
.endm

.macro save_s i, reg
    vmov s\i, reg
.endm

.macro mov_s dst, src
    vmov.f32 s\dst, s\src
.endm

// for unrolled version.
.macro FF1_rot xi0, xi1, xi2, rot2, res
    eor.w \res, \xi0, \xi1
    eor.w \res, \res, \xi2, ror \rot2
.endm

.macro FF2_rot xi0, xi1, xi2, rot2, tmp, res
    and.w \tmp, \xi0, \xi1
    orr.w \res, \xi0, \xi1
    and.w \res, \res, \xi2, ror \rot2
    orr.w \res, \res, \tmp 
.endm

.macro GG1_rot xi0, xi1, xi2, rot2, res
    eor.w \res, \xi0, \xi1
    eor.w \res, \res, \xi2, ror \rot2
.endm

.macro GG2_rot xi0, xi1, xi2, rot2, tmp, res
    eor.w \res, \xi1, \xi2, ror \rot2
    and.w \res, \res, \xi0
    eor.w \res, \res, \xi2, ror \rot2
.endm

//P0(x) ((x) ^ L_SHIFT((x), 9) ^ L_SHIFT((x), 17))
.macro P0 xi0, res
    eor \res, \xi0, \xi0, ror #(32-9)
    eor \res, \res, \xi0, ror #(32-17)
.endm 

// P1(x)=(x^L_SHIFT(x,15)^L_SHIFT(x,23))
.macro P1 xi0, res
    eor \res, \xi0, \xi0, ror #(32-15)
    eor \res, \res, \xi0, ror #(32-23) 
.endm

// load msg w0-w15; compute w16-w19, and store thme to s5-s24
.macro load_msg_expand_r1 msg_ptr, wi, w1, w2, w3, w4, w5
    .set idx, 3
    .rept 3
        ldr.w \w2, [\msg_ptr, #4*7]
        ldr.w \w3, [\msg_ptr, #4*13]
        ldr.w \w4, [\msg_ptr, #4*3]
        ldr.w \w5, [\msg_ptr, #4*10]
        ldr.w \w1, [\msg_ptr], #4
        
        rev.w \w1, \w1
        rev.w \w2, \w2
        rev.w \w3, \w3
        rev.w \w4, \w4
        rev.w \w5, \w5
        
        save_s %(idx+7),  \w2

        eor.w \wi, \w1, \w2
        eor.w \w2, \wi, \w3, ror #(32-15)
        P1 \w2, \wi //res in wi
        eor.w \wi, \wi, \w4, ror #(32-7)
        eor.w \wi, \wi, \w5

        save_s %(idx+0),  \w1
        save_s %(idx+13), \w3
        save_s %(idx+3),  \w4
        save_s %(idx+10), \w5
        save_s %(idx+16), \wi

        .set idx, idx+1
    .endr

    // idx=5+3
    // msg_ptr, #3*4+3*4=6*4
    load_s \w1, %(idx+0)
    load_s \w2, %(idx+7)
    load_s \w3, %(idx+13)
    load_s \w5, %(idx+10)
    ldr.w  \w4, [\msg_ptr, #4*3]

    rev.w \w4, \w4

    eor.w \wi, \w1, \w2
    eor.w \w2, \wi, \w3, ror #(32-15)
    P1 \w2, \wi //res in wi
    eor.w \wi, \wi, \w4, ror #(32-7)
    eor.w \wi, \wi, \w5

    save_s %(idx+3),  \w4
    save_s %(idx+16), \wi

    // update msg and w index
    add.w \msg_ptr, #13*4 //go to the next block of msg
.endm

// compute 16*i+4; move {16-19} to the head.
.macro load_msg_expand_ri w1, w2, w3, w4, w5
    // w20-w68
    // mov the last 4 to the head.
    .set i, 3
    mov_s %(i), %(i+16)
    mov_s %(i+1), %(i+17)
    mov_s %(i+2), %(i+18)
    mov_s %(i+3), %(i+19)

    .set i, i+4
    .rept 16
        load_s \w1, %(7 + ((i + 0  - 7) % 16))
        load_s \w2, %(7 + ((i + 7  - 7) % 16))
        load_s \w3, %(7 + ((i + 13 - 7) % 16))
        load_s \w4, %(7 + ((i + 3  - 7) % 16))
        load_s \w5, %(7 + ((i + 10 - 7) % 16))

        eor.w \w1, \w1, \w2
        eor.w \w2, \w1, \w3, ror #(32-15)
        P1 \w2, \w1 //res in w1
        eor.w \w1, \w1, \w4, ror #(32-7)
        eor.w \w1, \w1, \w5

        save_s %(i+0), \w1
        .set i, i+1
    .endr

.endm

// lazy rotation of ss1; merge it with ss2; ss2=(ss1'<<7)^(ssA<<12)=(ss1'^ssA<<5)<<7; omit explicit ss1'<<7 using inline rotation for ss1' and ss2 in compute_tti;
.macro compute_ssi_0_15 ss1, ss2, sst, ssA, ssE 
    add \ss1, \ssE, \ssA, ror #(32-12)
    add \ss1, \ss1, \sst
    //ror \ss1, \ss1, #(32-7)
    eor \ss2, \ss1, \ssA, ror #(32-5)
    ror \sst, \sst, #31
.endm

.macro compute_tti_0_15 tt1, tt2, ss1, ss2, ssA, ssB, ssC, rotC, ssD, rotD, ssE, ssF, ssG, rotG, ssH, rotH, sptr, idx
    FF1_rot \ssA, \ssB, \ssC, \rotC, \tt1
    add \tt1, \tt1, \ssD, ror \rotD
    add \tt1, \tt1, \ss2, ror #(32-7)
    GG1_rot \ssE, \ssF, \ssG, \rotG, \tt2
    add \tt2, \tt2, \ssH, ror \rotH
    add \tt2, \tt2, \ss1, ror #(32-7)
    //ss1 and ss2 free
    // ldr.w \ss2, [\sptr, #4*4]
    // ldr.w \ss1, [\sptr], #4
    load_s \ss1, %(\idx)
    load_s \ss2, %(\idx+4)
    eor \ss2, \ss2, \ss1
    add \tt1, \tt1, \ss2
    add \tt2, \tt2, \ss1
.endm

// lazy rotation of ss1; merge it with ss2; ss2=(ss1'<<7)^(ssA<<12)=(ss1'^ssA<<5)<<7; omit explicit ss1'<<7 using inline rotation for ss1' and ss2 in compute_tti; reduce 1 ror for each round.
.macro compute_ssi_16_63 ss1, ss2, sst, ssA, ssE
    add \ss1, \ssE, \ssA, ror #(32-12)
    add \ss1, \ss1, \sst
    //ror \ss1, \ss1, #(32-7)
    eor \ss2, \ss1, \ssA, ror #(32-5)
    ror \sst, \sst, #31
.endm

// require s2
.macro compute_tti_16_63 tt1, tt2, ss1, ss2, ssA, ssB, ssC, rotC, ssD, rotD, ssE, ssF, ssG, rotG, ssH, rotH, sptr, idx
    FF2_rot \ssA, \ssB, \ssC, \rotC, \sptr, \tt1
    add \tt1, \tt1, \ssD, ror \rotD
    add \tt1, \tt1, \ss2, ror #(32-7)
    GG2_rot \ssE, \ssF, \ssG, \rotG, \sptr, \tt2
    add \tt2, \tt2, \ssH, ror \rotH
    add \tt2, \tt2, \ss1, ror #(32-7)
    //ss1 and ss2 free
    // vmov \sptr, s2
    // ldr.w \ss2, [\sptr, #4*4]
    // ldr.w \ss1, [\sptr], #4
    // vmov s2, \sptr // save w[i]

    load_s \ss1, %(\idx)
    load_s \ss2, %(\idx+4)
    eor \ss2, \ss2, \ss1
    add \tt1, \tt1, \ss2
    add \tt2, \tt2, \ss1
.endm

// void sm3_bit_compress(unsigned int dgst[8], const unsigned char *msg, unsigned int blocks): only supporting 32-bit for PQC support.
// perform message expansion and 64-round of permutation 
.global sm3_bit_compress_asm
.type sm3_bit_compress_asm, %function
.align 2
sm3_bit_compress_asm:
    push {r4-r11, lr}
    vpush {s16-s22}
    sptr      .req r0 // state[8+68]
    msg_ptr   .req r1 // message.
    ssA       .req r2 // number of blocks
    ssB       .req r3
    ssC       .req r4
    ssD       .req r5
    ssE       .req r6
    ssF       .req r7 
    ssG       .req r8
    ssH       .req r9
    ss1       .req r10
    ss2       .req r11
    tt1       .req r12
    tt2       .req r14
    loop      .req r0

    cmp ssA, #0
    beq.w .Lsm3_done

    vmov s0, ssA //num blocks
    vmov s1, sptr //sptr

    vmov s2, msg_ptr //msg_ptr
    vmov loop, s0 //blocks

.Lsm3_block_check:
    //save state and loop
    vmov s0, loop

    vmov msg_ptr, s2 //get msg_ptr
    load_msg_expand_r1 msg_ptr, ssA, ssB, ssC, ssD, ssE, ssF
    vmov s2, msg_ptr // update msg_ptr

    // extract state
    vmov sptr, s1
    ldm.w sptr, {ssA, ssB, ssC, ssD, ssE, ssF, ssG, ssH}

    movw msg_ptr, #0x4519
    movt msg_ptr, #0x79cc // constant T0

    .set idx, 3
    .rept 2 // round 0-15
        // 0
        compute_ssi_0_15 ss1, ss2, msg_ptr, ssA, ssE
        compute_tti_0_15 tt1, tt2, ss1, ss2, ssA, ssB, ssC, 0, ssD, 0, ssE, ssF, ssG, 0, ssH, 0, sptr, idx
        // one register (ssD) is free; as tt1 is used to pass value

        // 1
        .set idx, idx+1
        P0 tt2, ssD // ssD is free
        compute_ssi_0_15 ss1, ss2, msg_ptr, tt1, ssD
        compute_tti_0_15 ssH, tt2, ss1, ss2, tt1, ssA, ssB, 23, ssC, 0, ssD, ssE, ssF, 13, ssG, 0, sptr, idx
        
        // 2
        .set idx, idx+1
        P0 tt2, ssC //ssC is free
        compute_ssi_0_15 ss1, ss2, msg_ptr, ssH, ssC
        compute_tti_0_15 ssG, tt2, ss1, ss2, ssH, tt1, ssA, 23, ssB, 23, ssC, ssD, ssE, 13, ssF, 13, sptr, idx

        // 3
        .set idx, idx+1
        P0 tt2, ssB //ssB is free
        compute_ssi_0_15 ss1, ss2, msg_ptr, ssG, ssB
        compute_tti_0_15 ssF, tt2, ss1, ss2, ssG, ssH, tt1, 23, ssA, 23, ssB, ssC, ssD, 13, ssE, 13, sptr, idx

        // 4
        .set idx, idx+1
        P0 tt2, ssE //ssE is free
        compute_ssi_0_15 ss1, ss2, msg_ptr, ssF, ssE
        compute_tti_0_15 ssA, tt2, ss1, ss2, ssF, ssG, ssH, 23, tt1, 23, ssE, ssB, ssC, 13, ssD, 13, sptr, idx

        // 5
        .set idx, idx+1
        P0 tt2, tt1 //tt1 is free
        compute_ssi_0_15 ss1, ss2, msg_ptr, ssA, tt1
        compute_tti_0_15 ssD, tt2, ss1, ss2, ssA, ssF, ssG, 23, ssH, 23, tt1, ssE, ssB, 13, ssC, 13, sptr, idx

        // 6
        .set idx, idx+1
        P0 tt2, ssH //ssH is free
        compute_ssi_0_15 ss1, ss2, msg_ptr, ssD, ssH
        compute_tti_0_15 ssC, tt2, ss1, ss2, ssD, ssA, ssF, 23, ssG, 23, ssH, tt1, ssE, 13, ssB, 13, sptr, idx

        // 7
        .set idx, idx+1
        P0 tt2, ssG //ssG is free
        compute_ssi_0_15 ss1, ss2, msg_ptr, ssC, ssG
        compute_tti_0_15 ssB, tt2, ss1, ss2, ssC, ssD, ssA, 23, ssF, 23, ssG, ssH, tt1, 13, ssE, 13, sptr, idx

        P0 tt2, ssE
        ror tt2, ssD, #23
        ror ssD, ssA, #23
        mov ssA, ssB
        mov ssB, ssC
        mov ssC, tt2
        mov ssF, ssG
        ror ssG, ssH, #13
        ror ssH, tt1, #13

        .set idx, idx+1
    .endr

    movw msg_ptr, #0x7a87
    movt msg_ptr, #0x9d8a // constant T1 right shift by 16
    
    .rept 3
        .set idx, 3
        load_msg_expand_ri ss1, ss2, tt1, tt2, sptr // udpate s5-s24
        .rept 2
        // 0
        compute_ssi_16_63 ss1, ss2, msg_ptr, ssA, ssE
        compute_tti_16_63 tt1, tt2, ss1, ss2, ssA, ssB, ssC, 0, ssD, 0, ssE, ssF, ssG, 0, ssH, 0, sptr, idx

        // 1
        .set idx, idx+1
        P0 tt2, ssD // ssD is free
        compute_ssi_16_63 ss1, ss2, msg_ptr, tt1, ssD
        compute_tti_16_63 ssH, tt2, ss1, ss2, tt1, ssA, ssB, 23, ssC, 0, ssD, ssE, ssF, 13, ssG, 0, sptr, idx
        
        // 2
        .set idx, idx+1
        P0 tt2, ssC //ssC is free
        compute_ssi_16_63 ss1, ss2, msg_ptr, ssH, ssC
        compute_tti_16_63 ssG, tt2, ss1, ss2, ssH, tt1, ssA, 23, ssB, 23, ssC, ssD, ssE, 13, ssF, 13, sptr, idx
        
        // 3
        .set idx, idx+1
        P0 tt2, ssB //ssB is free
        compute_ssi_16_63 ss1, ss2, msg_ptr, ssG, ssB
        compute_tti_16_63 ssF, tt2, ss1, ss2, ssG, ssH, tt1, 23, ssA, 23, ssB, ssC, ssD, 13, ssE, 13, sptr, idx

        // 4
        .set idx, idx+1
        P0 tt2, ssE //ssE is free
        compute_ssi_16_63 ss1, ss2, msg_ptr, ssF, ssE
        compute_tti_16_63 ssA, tt2, ss1, ss2, ssF, ssG, ssH, 23, tt1, 23, ssE, ssB, ssC, 13, ssD, 13, sptr, idx

        // 5
        .set idx, idx+1
        P0 tt2, tt1 //tt1 is free
        compute_ssi_16_63 ss1, ss2, msg_ptr, ssA, tt1
        compute_tti_16_63 ssD, tt2, ss1, ss2, ssA, ssF, ssG, 23, ssH, 23, tt1, ssE, ssB, 13, ssC, 13, sptr, idx

        // 6
        .set idx, idx+1
        P0 tt2, ssH //ssH is free
        compute_ssi_16_63 ss1, ss2, msg_ptr, ssD, ssH
        compute_tti_16_63 ssC, tt2, ss1, ss2, ssD, ssA, ssF, 23, ssG, 23, ssH, tt1, ssE, 13, ssB, 13, sptr, idx

        // 7
        .set idx, idx+1
        P0 tt2, ssG //ssG is free
        compute_ssi_16_63 ss1, ss2, msg_ptr, ssC, ssG
        compute_tti_16_63 ssB, tt2, ss1, ss2, ssC, ssD, ssA, 23, ssF, 23, ssG, ssH, tt1, 13, ssE, 13, sptr, idx

        P0 tt2, ssE
        ror tt2, ssD, #23
        ror ssD, ssA, #23
        mov ssA, ssB
        mov ssB, ssC
        mov ssC, tt2
        mov ssF, ssG
        ror ssG, ssH, #13
        ror ssH, tt1, #13
        .set idx, idx+1
        .endr //inner loop
    .endr //outer loop

    vmov sptr, s1

    ldm.w sptr!, {ss1, ss2, tt1, tt2}
    eor ssA, ssA, ss1
    eor ssB, ssB, ss2
    eor ssC, ssC, tt1
    eor ssD, ssD, tt2

    ldm.w sptr!, {ss1, ss2, tt1, tt2}
    eor ssE, ssE, ss1
    eor ssF, ssF, ss2
    eor ssG, ssG, tt1
    eor ssH, ssH, tt2

    sub.w sptr, sptr, #4*8
    stm.w sptr!, {ssA, ssB, ssC, ssD, ssE, ssF, ssG, ssH}

    vmov loop, s0
    subs loop, #1
    bne.w .Lsm3_block_check

.Lsm3_done:
    vpop {s16-s22}
    pop {r4-r11, pc}
    
