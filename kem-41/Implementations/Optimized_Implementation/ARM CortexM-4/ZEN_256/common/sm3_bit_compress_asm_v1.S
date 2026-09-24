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
// Code size: 844 bytes.
//FF1(x, y, z) ((x) ^ (y) ^ (z))
.macro FF1 xi0, xi1, xi2, res
    eor.w \res, \xi0, \xi1
    eor.w \res, \res, \xi2
.endm

.macro FF1_rot xi0, xi1, xi2, rot2, res
    eor.w \res, \xi0, \xi1
    eor.w \res, \res, \xi2, ror \rot2
.endm

// (((x) & (y)) | ((x) & (z)) | ((y) & (z)))=(((x) & (y)) | ((x) | (y)) & (z))
.macro FF2 xi0, xi1, xi2, tmp, res
    and.w \tmp, \xi0, \xi1
    orr.w \res, \xi0, \xi1
    and.w \res, \res, \xi2
    orr.w \res, \res, \tmp 
.endm

.macro FF2_rot xi0, xi1, xi2, rot2, tmp, res
    and.w \tmp, \xi0, \xi1
    orr.w \res, \xi0, \xi1
    and.w \res, \res, \xi2, ror \rot2
    orr.w \res, \res, \tmp 
.endm

//((x) ^ (y) ^ (z))
.macro GG1 xi0, xi1, xi2, res
    eor.w \res, \xi0, \xi1
    eor.w \res, \res, \xi2
.endm

.macro GG1_rot xi0, xi1, xi2, rot2, res
    eor.w \res, \xi0, \xi1
    eor.w \res, \res, \xi2, ror \rot2
.endm

//((((y) ^ (z)) & (x)) ^ (z))
.macro GG2 xi0, xi1, xi2, tmp, res
    eor.w \res, \xi1, \xi2
    and.w \res, \res, \xi0
    eor.w \res, \res, \xi2
.endm

.macro GG2_rot xi0, xi1, xi2, rot2, tmp, res
    eor.w \res, \xi1, \xi2, \rot2
    and.w \res, \res, \xi0
    eor.w \res, \res, \xi2, \rot2
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


.macro load_msg_expand msg_ptr, wptr, wi, w1, w2, w3, w4, w5, tmp, loop
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

    str.w \w2, [\wptr, #4*7]
    str.w \w3, [\wptr, #4*13]

    eor.w \wi, \w1, \w2
    eor.w \w2, \wi, \w3, ror #(32-15)
    P1 \w2, \wi //res in wi
    eor.w \wi, \wi, \w4, ror #(32-7)
    eor.w \wi, \wi, \w5

    str.w \w4, [\wptr, #4*3]
    str.w \w5, [\wptr, #4*10]
    str.w \wi, [\wptr, #16*4] 
    str.w \w1, [\wptr], #4
    .endr

    // msg_ptr, #3*4+3*4=6*4
    
    ldr.w \w1, [\wptr, #4*0]
    ldr.w \w2, [\wptr, #4*7]
    ldr.w \w3, [\wptr, #4*13]
    ldr.w \w5, [\wptr, #4*10]
    ldr.w \w4, [\msg_ptr, #4*3]

    rev.w \w4, \w4

    eor.w \wi, \w1, \w2
    eor.w \w2, \wi, \w3, ror #(32-15)
    P1 \w2, \wi //res in wi
    eor.w \wi, \wi, \w4, ror #(32-7)
    eor.w \wi, \wi, \w5

    str.w \w4, [\wptr, #4*3]
    str.w \wi, [\wptr, #16*4] 

    add.w \wptr, #17*4 //17+3=20

    // w20-w68
    mov.w \loop, #(68-20)
    2:
    ldr.w \w1, [\wptr, #-4*16]
    ldr.w \w2, [\wptr, #-4*9]
    ldr.w \w3, [\wptr, #-4*3]
    ldr.w \w4, [\wptr, #-4*13]
    ldr.w \w5, [\wptr, #-4*6]

    eor.w \wi, \w1, \w2
    eor.w \w2, \wi, \w3, ror #(32-15)
    P1 \w2, \wi //res in wi
    eor.w \wi, \wi, \w4, ror #(32-7)
    eor.w \wi, \wi, \w5

    str.w \wi, [\wptr], #4
    subs \loop, #1
    bne.w 2b

    // update msg and w index
    add.w \msg_ptr, #13*4 //go to the next block of msg
    sub.w \wptr, #68*4 //return to index 0
.endm

.macro comput_ssi ss1, ss2, sst, ssA, ssE 
    add \ss1, \ssE, \ssA, ror #(32-12)
    add \ss1, \ss1, \sst
    ror \ss1, \ss1, #(32-7)
    eor \ss2, \ss1, \ssA, ror #(32-12)
    ror \sst, \sst, #31
.endm

// void sm3_bit_compress(unsigned int dgst[8+68], const unsigned char *msg, unsigned int blocks): only supporting 32-bit for PQC support.
// perform message expansion and 64-round of permutation 
.global sm3_bit_compress_asm
.type sm3_bit_compress_asm, %function
.align 2
sm3_bit_compress_asm:

    push {r4-r11, lr}
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

    add sptr, sptr, #4*8
    vmov s2, sptr //wptr
    vmov s3, msg_ptr //msg_ptr
    vmov loop, s0 //blocks
.Lsm3_block_check:
    //save state and loop
    vmov s0, loop

    vmov sptr, s2 // get wptr
    vmov msg_ptr, s3 //get msg_ptr
    load_msg_expand msg_ptr, sptr, ssA, ssB, ssC, ssD, ssE, ssF, ssG, ssH 
    vmov s3, msg_ptr // update msg_ptr

    //extract state
    // load state
    vmov sptr, s1
    ldr.w ssA, [sptr, #4*0]
    ldr.w ssB, [sptr, #4*1]
    ldr.w ssC, [sptr, #4*2]
    ldr.w ssD, [sptr, #4*3]
    ldr.w ssE, [sptr, #4*4]
    ldr.w ssF, [sptr, #4*5]
    ldr.w ssG, [sptr, #4*6]
    ldr.w ssH, [sptr, #4*7]

    movw msg_ptr, #0x4519
    movt msg_ptr, #0x79cc // constant T0

    mov.w loop, #16
.Lsm3_round_0_15:
    vmov s4, loop
    add ss1, ssE, ssA, ror #(32-12)
    add ss1, ss1, msg_ptr
    ror ss1, ss1, #(32-7)
    eor ss2, ss1, ssA, ror #(32-12)
    ror msg_ptr, msg_ptr, #31

    // tt1 and tt2
    FF1 ssA, ssB, ssC, tt1
    add tt1, tt1, ssD
    add tt1, tt1, ss2
    GG1 ssE, ssF, ssG, tt2
    add tt2, tt2, ssH
    add tt2, tt2, ss1
    //ss1 and ss2 free
    vmov sptr, s2
    ldr.w ss2, [sptr, #4*4]
    ldr.w ss1, [sptr], #4
    vmov s2, sptr // save w[i]
    eor ss2, ss2, ss1
    add tt1, tt1, ss2
    add tt2, tt2, ss1

    //A~H
    mov ssD, ssC
    ror ssC, ssB, #(32-9)
    mov ssB, ssA
    mov ssA, tt1
    mov ssH, ssG
    ror ssG, ssF, #(32-19)
    mov ssF, ssE
    P0 tt2, ssE

    vmov loop, s4
    subs loop, #1
    bne.w .Lsm3_round_0_15

    movw msg_ptr, #0x7a87
    movt msg_ptr, #0x9d8a // constant T1 right shift by 16
    
    mov.w loop, #(64-16) 
.Lsm3_round_16_63:
    vmov s4, loop
    //ss1 ss2
    add ss1, ssE, ssA, ror #(32-12)
    add ss1, ss1, msg_ptr
    ror ss1, ss1, #(32-7)
    eor ss2, ss1, ssA, ror #(32-12)
    ror msg_ptr, msg_ptr, #31

    // tt1 and tt2
    FF2 ssA, ssB, ssC, sptr, tt1
    add tt1, tt1, ssD
    add tt1, tt1, ss2
    GG2 ssE, ssF, ssG, sptr, tt2
    add tt2, tt2, ssH
    add tt2, tt2, ss1
    //ss1 and ss2 free
    vmov sptr, s2
    ldr.w ss2, [sptr, #4*4]
    ldr.w ss1, [sptr], #4
    vmov s2, sptr // save w[i]
    eor ss2, ss2, ss1
    add tt1, tt1, ss2
    add tt2, tt2, ss1

    //A~H
    mov ssD, ssC
    ror ssC, ssB, #(32-9)
    mov ssB, ssA
    mov ssA, tt1
    mov ssH, ssG
    ror ssG, ssF, #(32-19)
    mov ssF, ssE
    P0 tt2, ssE

    vmov loop, s4
    subs loop, #1
    bne.w .Lsm3_round_16_63

    // xor with original dgst; update w index s2
    // use ss0,ss1,tt0,tt1 
    vmov sptr, s1
    ldr.w ss1, [sptr, #4*0]
    ldr.w ss2, [sptr, #4*1]
    ldr.w tt1, [sptr, #4*2]
    ldr.w tt2, [sptr, #4*3]
    eor ssA, ssA, ss1
    eor ssB, ssB, ss2
    eor ssC, ssC, tt1
    eor ssD, ssD, tt2

    ldr.w ss1, [sptr, #4*4]
    ldr.w ss2, [sptr, #4*5]
    ldr.w tt1, [sptr, #4*6]
    ldr.w tt2, [sptr, #4*7]
    eor ssE, ssE, ss1
    eor ssF, ssF, ss2
    eor ssG, ssG, tt1
    eor ssH, ssH, tt2

    str.w ssA, [sptr], #4
    str.w ssB, [sptr], #4
    str.w ssC, [sptr], #4
    str.w ssD, [sptr], #4
    str.w ssE, [sptr], #4
    str.w ssF, [sptr], #4
    str.w ssG, [sptr], #4
    str.w ssH, [sptr], #4
    

    vmov s2, sptr

    vmov loop, s0
    subs loop, #1
    bne.w .Lsm3_block_check

.Lsm3_done:
    
    pop {r4-r11, pc}
