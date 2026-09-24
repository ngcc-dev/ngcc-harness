.syntax unified
.thumb

.macro umull64 c0, c1, c2, c3, a0, a1, b0, b1, tmp1, tmp2
    umull \tmp1, \tmp2, \a0, \b1
    umlal \tmp1, \tmp2, \a1, \b0
    umull \c0, \c1, \a0, \b0
    umull \c2, \c3, \a1, \b1
    adds.w \c1, \c1, \tmp1
    adcs.w \c2, \c2, \tmp2
    adc.w \c3, \c3, #0
.endm

.global umull64_shr58
.type umull64_shr58, %function
.align 2
umull64_shr58:
    push {r4}

    /* r0 == x low */
    /* r1 == x high */
    /* r2 == y low */
    /* r3 == y high */

    /* compute x * y */
    /* res == r3 | r1 | r2 | r0 */
    umull64 r0, r2, r1, r3, r0, r1, r2, r3, r4, r12

    lsr.w r12, r1, #26
    lsr.w r4, r2, #26
    orr.w r0, r4, r1, lsl #6
    orr.w r1, r12, r3, lsl #6

    pop {r4}
    bx lr
.size umull64_shr58, .-umull64_shr58

.macro fqmul_asm re, a, b, qinv, q, tmp
    mul.w \re, \a, \b
    mul.w \tmp, \re, \qinv
    sxth \tmp, \tmp
    mul.w \tmp, \tmp, \q
    sub.w \re, \re, \tmp
    asr.w \re, \re, #16
.endm

.macro base_mul4 zeta
    ldr.w c_0, [addr_a]
    ldr.w c_1, [addr_a, #1*4]
    ldr.w c_2, [addr_a, #2*4]
    ldr.w tmp0, [addr_b]
    ldr.w tmp1, [addr_b, #1*4]
    ldr.w tmp2, [addr_b, #2*4]
    fqmul_asm c_0, c_0, tmp0, qinv, q, tmp0
    fqmul_asm c_1, c_1, tmp1, qinv, q, tmp1
    fqmul_asm c_2, c_2, tmp2, qinv, q, tmp2

    ldr.w c_3, [addr_a, #3*4]
    ldr.w tmp0, [addr_b, #3*4]
    fqmul_asm c_3, c_3, tmp0, qinv, q, tmp0

// compute r[0]
    ldr.w tmp0, [addr_a, #1*4]
    ldr.w tmp1, [addr_a, #3*4]
    add.w tmp0, tmp0, tmp1
    ldr.w tmp1, [addr_b, #1*4]
    ldr.w tmp2, [addr_b, #3*4]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[0]
    add.w tmp2, c_1, c_3
    add.w tmp0, tmp0, c_2
    sub.w tmp0, tmp0, tmp2
    fqmul_asm tmp0, tmp0, \zeta, qinv, q, tmp1   // fqmul(r[0], zeta);
    add.w tmp0, tmp0, c_0

    str.w tmp0, [addr_r]

// compute r[2]
    ldr.w tmp0, [addr_a]
    ldr.w tmp1, [addr_a, #2*4]
    add.w tmp0, tmp0, tmp1
    ldr.w tmp1, [addr_b]
    ldr.w tmp2, [addr_b, #2*4]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[2]
    add.w tmp0, tmp0, c_1
    fqmul_asm tmp1, c_3, \zeta, qinv, q, tmp2     // fqmul(c3, zeta);
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_0, c_2
    sub.w tmp0, tmp0, tmp2

    str.w tmp0, [addr_r, #2*4]

// compute r[1]
    ldr.w tmp0, [addr_a, #2*4]
    ldr.w tmp1, [addr_a, #3*4]
    add.w tmp0, tmp0, tmp1
    ldr.w tmp1, [addr_b, #2*4]
    ldr.w tmp2, [addr_b, #3*4]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[1]
    add.w tmp2, c_2, c_3
    sub.w tmp0, tmp0, tmp2
    fqmul_asm tmp0, tmp0, \zeta, qinv, q, tmp1   // fqmul(r[1], zeta);
    ldr.w tmp1, [addr_a]
    ldr.w tmp2, [addr_a, #1*4]
    add.w tmp1, tmp1, tmp2                       // tmp0 == a0 + a1
    ldr.w tmp2, [addr_b]
    ldr.w \zeta, [addr_b, #1*4]
    add.w tmp2, tmp2, \zeta                      // tmp2 == b0 + b1
    fqmul_asm tmp1, tmp1, tmp2, qinv, q, tmp2
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_0, c_1
    sub.w tmp0, tmp0, tmp2

    str.w tmp0, [addr_r, #1*4]

// compute r[3]
    ldr.w tmp0, [addr_a]
    ldr.w tmp1, [addr_a, #3*4]
    add.w tmp0, tmp0, tmp1
    ldr.w tmp1, [addr_b]
    ldr.w tmp2, [addr_b, #3*4]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[3]
    add.w tmp2, c_0, c_3
    sub.w tmp0, tmp0, tmp2
    ldr.w tmp1, [addr_a, #1*4]
    ldr.w tmp2, [addr_a, #2*4]
    add.w tmp1, tmp1, tmp2                      // tmp1 == a1 + a2
    ldr.w tmp2, [addr_b, #1*4]
    ldr.w \zeta, [addr_b, #2*4]
    add.w tmp2, tmp2, \zeta                      // tmp2 == b1 + b2
    fqmul_asm tmp1, tmp1, tmp2, qinv, q, tmp2
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_1, c_2
    sub.w tmp0, tmp0, tmp2

    str.w tmp0, [addr_r, #3*4]
.endm

.macro base_mul zeta
        ldr.w addr_a, [sp, #1*4]
        ldrsh.w r5, [addr_a, #1*2]
        ldrsh.w r6, [addr_a, #2*2]
        ldrsh.w r7, [addr_a, #3*2]
        ldrsh.w r4, [addr_a], #4*2
        str.w r4, [sp, #4*4]        // addr_ae == sp + 4*4
        str.w r5, [sp, #8*4]        // addr_ao == sp + 8*4
        str.w r6, [sp, #5*4]
        str.w r7, [sp, #9*4]
        ldrsh.w r5, [addr_a, #1*2]
        ldrsh.w r6, [addr_a, #2*2]
        ldrsh.w r7, [addr_a, #3*2]
        ldrsh.w r4, [addr_a], #4*2
        str.w r4, [sp, #6*4]
        str.w r5, [sp, #10*4]
        str.w r6, [sp, #7*4]
        str.w r7, [sp, #11*4]

        ldr.w addr_b, [sp, #2*4]
        ldrsh.w r5, [addr_b, #1*2]
        ldrsh.w r6, [addr_b, #2*2]
        ldrsh.w r7, [addr_b, #3*2]
        ldrsh.w r4, [addr_b], #4*2
        str.w r4, [sp, #12*4]        // addr_be == sp + 12*4
        str.w r5, [sp, #16*4]        // addr_bo == sp + 16*4
        str.w r6, [sp, #13*4]
        str.w r7, [sp, #17*4]
        ldrsh.w r5, [addr_b, #1*2]
        ldrsh.w r6, [addr_b, #2*2]
        ldrsh.w r7, [addr_b, #3*2]
        ldrsh.w r4, [addr_b], #4*2
        str.w r4, [sp, #14*4]
        str.w r5, [sp, #18*4]
        str.w r6, [sp, #15*4]
        str.w r7, [sp, #19*4]

        str.w addr_a, [sp, #1*4]
        str.w addr_b, [sp, #2*4]

        str.w \zeta, [sp, #28*4] // zeta == sp + 28*4
        /* addr_p0 == sp + 20*4 ; addr_p1 == sp + 24*4 ; addr_pc == sp + 8*4 */
        add.w addr_a, sp, #4*4
        add.w addr_b, sp, #12*4
        add.w addr_r, sp, #20*4
        ldrsh.w \zeta, [sp, #28*4]
        base_mul4 \zeta
        add.w addr_a, sp, #8*4
        add.w addr_b, sp, #16*4
        add.w addr_r, sp, #24*4
        ldrsh.w \zeta, [sp, #28*4]
        base_mul4 \zeta

        ldr.w c_0, [sp, #4*4]
        ldr.w c_1, [sp, #5*4]
        ldr.w c_2, [sp, #8*4]
        ldr.w c_3, [sp, #9*4]
        add.w c_0, c_0, c_2
        add.w c_1, c_1, c_3
        str.w c_0, [sp, #4*4]
        str.w c_1, [sp, #5*4]

        ldr.w c_0, [sp, #6*4]
        ldr.w c_1, [sp, #7*4]
        ldr.w c_2, [sp, #10*4]
        ldr.w c_3, [sp, #11*4]
        add.w c_0, c_0, c_2
        add.w c_1, c_1, c_3
        str.w c_0, [sp, #6*4]
        str.w c_1, [sp, #7*4]

        ldr.w c_0, [sp, #12*4]
        ldr.w c_1, [sp, #13*4]
        ldr.w c_2, [sp, #16*4]
        ldr.w c_3, [sp, #17*4]
        add.w c_0, c_0, c_2
        add.w c_1, c_1, c_3
        str.w c_0, [sp, #12*4]
        str.w c_1, [sp, #13*4]

        ldr.w c_0, [sp, #14*4]
        ldr.w c_1, [sp, #15*4]
        ldr.w c_2, [sp, #18*4]
        ldr.w c_3, [sp, #19*4]
        add.w c_0, c_0, c_2
        add.w c_1, c_1, c_3
        str.w c_0, [sp, #14*4]
        str.w c_1, [sp, #15*4]

        add.w addr_a, sp, #4*4
        add.w addr_b, sp, #12*4
        add.w addr_r, sp, #8*4
        ldrsh.w \zeta, [sp, #28*4]
        base_mul4 \zeta

        ldr.w addr_r, [sp, #3*4]

        ldr.w c_0, [sp, #8*4]
        ldr.w c_1, [sp, #9*4]
        ldr.w c_2, [sp, #20*4]
        ldr.w c_3, [sp, #21*4]
        sub.w c_0, c_0, c_2
        sub.w c_1, c_1, c_3
        ldr.w c_2, [sp, #24*4]
        ldr.w c_3, [sp, #25*4]
        sub.w c_0, c_0, c_2
        sub.w c_1, c_1, c_3
        strh.w c_0, [addr_r, #1*2]   // r[1] == pc[0]
        strh.w c_1, [addr_r, #3*2]   // r[3] == pc[1]

        ldr.w c_0, [sp, #10*4]
        ldr.w c_1, [sp, #11*4]
        ldr.w c_2, [sp, #22*4]
        ldr.w c_3, [sp, #23*4]
        sub.w c_0, c_0, c_2
        sub.w c_1, c_1, c_3
        ldr.w c_2, [sp, #26*4]
        ldr.w c_3, [sp, #27*4]
        sub.w c_0, c_0, c_2
        sub.w c_1, c_1, c_3
        strh.w c_0, [addr_r, #5*2]   // r[5] == pc[2]
        strh.w c_1, [addr_r, #7*2]   // r[7] == pc[3]

        ldr.w c_0, [sp, #22*4]      //c_0 == p0[2]
        ldr.w c_1, [sp, #25*4]      //c_1 == p1[1]
        ldr.w c_2, [sp, #23*4]      //c_2 == p0[3]
        ldr.w c_3, [sp, #26*4]      //c_3 == p1[2]
        add.w c_0, c_0, c_1         // c_0 == r[4]
        add.w c_2, c_2, c_3         // c_2 == r[6]
        strh.w c_0, [addr_r, #4*2]
        strh.w c_2, [addr_r, #6*2]

        ldr.w c_0, [sp, #20*4]      //c_0 == p0[0]
        ldr.w c_1, [sp, #27*4]      //c_1 == p1[3]
        ldr.w c_2, [sp, #21*4]      //c_2 == p0[1]
        ldr.w c_3, [sp, #24*4]      //c_3 == p1[0]
        ldrsh \zeta, [sp, #28*4]
        fqmul_asm c_1, c_1, \zeta, qinv, q, \zeta
        add.w c_0, c_0, c_1         // c_0 == r[0]
        add.w c_2, c_2, c_3         // c_2 == r[2]
        strh.w c_2, [addr_r, #2*2]
        strh.w c_0, [addr_r], #8*2

        str.w addr_r, [sp, #3*4]
.endm

//void poly_basemul_asm(int16_t *r,  int16_t *a,  int16_t *b, int16_t *f)
.global poly_basemul_asm
.type poly_basemul_asm, %function
.align 2
poly_basemul_asm:

addr_r .req r0
addr_a .req r1
addr_b .req r2
addr_zeta .req r3
c_0 .req r4
c_1 .req r5
c_2 .req r6
c_3 .req r7
qinv .req r8
q .req r9
tmp0 .req r10
tmp1 .req r11
tmp2 .req r12
zeta .req r14
loop .req r11


    push {r4-r11, lr}
    sub.w sp, sp, #29*4
    str.w addr_a, [sp, #1*4]
    str.w addr_b, [sp, #2*4]
    str.w addr_r, [sp, #3*4]

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    add.w addr_zeta, addr_zeta, #128

    mov.w loop, #64
    str.w loop, [sp]
    basemul_loop:

        ldrsh.w zeta, [addr_zeta]
        base_mul zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul zeta

        ldr.w  loop, [sp]
        subs.w loop, loop, #1
        str.w  loop, [sp]
        bne.w basemul_loop

.unreq addr_r
.unreq addr_a
.unreq addr_b
.unreq addr_zeta
.unreq c_0
.unreq c_1
.unreq c_2
.unreq c_3
.unreq qinv
.unreq q
.unreq zeta
.unreq tmp0
.unreq tmp1
.unreq tmp2
.unreq loop

    add.w sp, sp, #29*4
    pop {r4-r11, pc}
.size poly_basemul_asm, .-poly_basemul_asm

.macro mq_finish re, tmp
    mov.w \tmp, #19
    fqmul_asm \re, \re, \tmp, qinv, q, \tmp
    asr.w \tmp, \re, #15
    and.w \tmp, \tmp, q
    add.w \re, \re, \tmp
.endm

.macro base_mul_mq zeta
        ldr.w addr_a, [sp, #1*4]
        ldrsh.w r5, [addr_a, #1*2]
        ldrsh.w r6, [addr_a, #2*2]
        ldrsh.w r7, [addr_a, #3*2]
        ldrsh.w r4, [addr_a], #4*2
        str.w r4, [sp, #4*4]        // addr_ae == sp + 4*4
        str.w r5, [sp, #8*4]        // addr_ao == sp + 8*4
        str.w r6, [sp, #5*4]
        str.w r7, [sp, #9*4]
        ldrsh.w r5, [addr_a, #1*2]
        ldrsh.w r6, [addr_a, #2*2]
        ldrsh.w r7, [addr_a, #3*2]
        ldrsh.w r4, [addr_a], #4*2
        str.w r4, [sp, #6*4]
        str.w r5, [sp, #10*4]
        str.w r6, [sp, #7*4]
        str.w r7, [sp, #11*4]

        ldr.w addr_b, [sp, #2*4]
        ldrsh.w r5, [addr_b, #1*2]
        ldrsh.w r6, [addr_b, #2*2]
        ldrsh.w r7, [addr_b, #3*2]
        ldrsh.w r4, [addr_b], #4*2
        str.w r4, [sp, #12*4]        // addr_be == sp + 12*4
        str.w r5, [sp, #16*4]        // addr_bo == sp + 16*4
        str.w r6, [sp, #13*4]
        str.w r7, [sp, #17*4]
        ldrsh.w r5, [addr_b, #1*2]
        ldrsh.w r6, [addr_b, #2*2]
        ldrsh.w r7, [addr_b, #3*2]
        ldrsh.w r4, [addr_b], #4*2
        str.w r4, [sp, #14*4]
        str.w r5, [sp, #18*4]
        str.w r6, [sp, #15*4]
        str.w r7, [sp, #19*4]

        str.w addr_a, [sp, #1*4]
        str.w addr_b, [sp, #2*4]

        str.w \zeta, [sp, #28*4] // zeta == sp + 28*4
        /* addr_p0 == sp + 20*4 ; addr_p1 == sp + 24*4 ; addr_pc == sp + 8*4 */
        add.w addr_a, sp, #4*4
        add.w addr_b, sp, #12*4
        add.w addr_r, sp, #20*4
        ldrsh.w \zeta, [sp, #28*4]
        base_mul4 \zeta
        add.w addr_a, sp, #8*4
        add.w addr_b, sp, #16*4
        add.w addr_r, sp, #24*4
        ldrsh.w \zeta, [sp, #28*4]
        base_mul4 \zeta

        ldr.w c_0, [sp, #4*4]
        ldr.w c_1, [sp, #5*4]
        ldr.w c_2, [sp, #8*4]
        ldr.w c_3, [sp, #9*4]
        add.w c_0, c_0, c_2
        add.w c_1, c_1, c_3
        str.w c_0, [sp, #4*4]
        str.w c_1, [sp, #5*4]

        ldr.w c_0, [sp, #6*4]
        ldr.w c_1, [sp, #7*4]
        ldr.w c_2, [sp, #10*4]
        ldr.w c_3, [sp, #11*4]
        add.w c_0, c_0, c_2
        add.w c_1, c_1, c_3
        str.w c_0, [sp, #6*4]
        str.w c_1, [sp, #7*4]

        ldr.w c_0, [sp, #12*4]
        ldr.w c_1, [sp, #13*4]
        ldr.w c_2, [sp, #16*4]
        ldr.w c_3, [sp, #17*4]
        add.w c_0, c_0, c_2
        add.w c_1, c_1, c_3
        str.w c_0, [sp, #12*4]
        str.w c_1, [sp, #13*4]

        ldr.w c_0, [sp, #14*4]
        ldr.w c_1, [sp, #15*4]
        ldr.w c_2, [sp, #18*4]
        ldr.w c_3, [sp, #19*4]
        add.w c_0, c_0, c_2
        add.w c_1, c_1, c_3
        str.w c_0, [sp, #14*4]
        str.w c_1, [sp, #15*4]

        add.w addr_a, sp, #4*4
        add.w addr_b, sp, #12*4
        add.w addr_r, sp, #8*4
        ldrsh.w \zeta, [sp, #28*4]
        base_mul4 \zeta

        ldr.w addr_r, [sp, #3*4]

        ldr.w c_0, [sp, #8*4]
        ldr.w c_1, [sp, #9*4]
        ldr.w c_2, [sp, #20*4]
        ldr.w c_3, [sp, #21*4]
        sub.w c_0, c_0, c_2
        sub.w c_1, c_1, c_3
        ldr.w c_2, [sp, #24*4]
        ldr.w c_3, [sp, #25*4]
        sub.w c_0, c_0, c_2
        sub.w c_1, c_1, c_3
        mq_finish c_0, tmp0
        mq_finish c_1, tmp0
        strh.w c_0, [addr_r, #1*2]   // r[1] == pc[0]
        strh.w c_1, [addr_r, #3*2]   // r[3] == pc[1]

        ldr.w c_0, [sp, #10*4]
        ldr.w c_1, [sp, #11*4]
        ldr.w c_2, [sp, #22*4]
        ldr.w c_3, [sp, #23*4]
        sub.w c_0, c_0, c_2
        sub.w c_1, c_1, c_3
        ldr.w c_2, [sp, #26*4]
        ldr.w c_3, [sp, #27*4]
        sub.w c_0, c_0, c_2
        sub.w c_1, c_1, c_3
        mq_finish c_0, tmp0
        mq_finish c_1, tmp0
        strh.w c_0, [addr_r, #5*2]   // r[5] == pc[2]
        strh.w c_1, [addr_r, #7*2]   // r[7] == pc[3]

        ldr.w c_0, [sp, #22*4]      //c_0 == p0[2]
        ldr.w c_1, [sp, #25*4]      //c_1 == p1[1]
        ldr.w c_2, [sp, #23*4]      //c_2 == p0[3]
        ldr.w c_3, [sp, #26*4]      //c_3 == p1[2]
        add.w c_0, c_0, c_1         // c_0 == r[4]
        add.w c_2, c_2, c_3         // c_2 == r[6]
        mq_finish c_0, tmp0
        mq_finish c_2, tmp0
        strh.w c_0, [addr_r, #4*2]
        strh.w c_2, [addr_r, #6*2]

        ldr.w c_0, [sp, #20*4]      //c_0 == p0[0]
        ldr.w c_1, [sp, #27*4]      //c_1 == p1[3]
        ldr.w c_2, [sp, #21*4]      //c_2 == p0[1]
        ldr.w c_3, [sp, #24*4]      //c_3 == p1[0]
        ldrsh \zeta, [sp, #28*4]
        fqmul_asm c_1, c_1, \zeta, qinv, q, \zeta
        add.w c_0, c_0, c_1         // c_0 == r[0]
        add.w c_2, c_2, c_3         // c_2 == r[2]
        mq_finish c_0, tmp0
        mq_finish c_2, tmp0
        strh.w c_2, [addr_r, #2*2]
        strh.w c_0, [addr_r], #8*2

        str.w addr_r, [sp, #3*4]
.endm

//void poly_basemul_mq_asm(int16_t *r,  int16_t *a,  int16_t *b, int16_t *f)
.global poly_basemul_mq_asm
.type poly_basemul_mq_asm, %function
.align 2
poly_basemul_mq_asm:

addr_r .req r0
addr_a .req r1
addr_b .req r2
addr_zeta .req r3
c_0 .req r4
c_1 .req r5
c_2 .req r6
c_3 .req r7
qinv .req r8
q .req r9
tmp0 .req r10
tmp1 .req r11
tmp2 .req r12
zeta .req r14
loop .req r11


    push {r4-r11, lr}
    sub.w sp, sp, #29*4
    str.w addr_a, [sp, #1*4]
    str.w addr_b, [sp, #2*4]
    str.w addr_r, [sp, #3*4]

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    add.w addr_zeta, addr_zeta, #128

    mov.w loop, #64
    str.w loop, [sp]
    basemul_mq_loop:

        ldrsh.w zeta, [addr_zeta]
        base_mul_mq zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul_mq zeta

        ldr.w  loop, [sp]
        subs.w loop, loop, #1
        str.w  loop, [sp]
        bne.w basemul_mq_loop

.unreq addr_r
.unreq addr_a
.unreq addr_b
.unreq addr_zeta
.unreq c_0
.unreq c_1
.unreq c_2
.unreq c_3
.unreq qinv
.unreq q
.unreq zeta
.unreq tmp0
.unreq tmp1
.unreq tmp2
.unreq loop

    add.w sp, sp, #29*4
    pop {r4-r11, pc}
.size poly_basemul_mq_asm, .-poly_basemul_mq_asm

.macro base_inv zeta
    /* pe */
    ldrsh.w a_0_or_1, [addr_a]
    ldrsh.w a_2_or_3, [addr_a, #2*2]
    ldrsh.w a_4_or_5, [addr_a, #4*2]
    ldrsh.w a_6_or_7, [addr_a, #6*2]

    /* pe[0] == sp + 8*4 */
    /* po[0] == sp + 15*4 */
    /* p0 == sp ; p1 == sp + 1*4 ; p2 == sp + 2*4 */
    /* q0 == sp + 3*4 ; q1 == sp + 4*4 ; q2 == sp + 5*4 */
    /* m0 == sp + 6*4 ; m1 == sp + 7*4 */

    fqmul_asm tmp0, a_0_or_1, a_0_or_1, qinv, q, tmp1
    str.w tmp0, [sp]
    str.w tmp0, [sp, #8*4]  // pe[0]
    fqmul_asm tmp0, a_0_or_1, a_2_or_3, qinv, q, tmp1
    mov.w tmp1, #342
    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1
    str.w tmp0, [sp, #1*4]
    str.w tmp0, [sp, #9*4]  // pe[1]
    fqmul_asm tmp0, a_2_or_3, a_2_or_3, qinv, q, tmp1
    str.w tmp0, [sp, #2*4]

    fqmul_asm tmp0, a_4_or_5, a_4_or_5, qinv, q, tmp1
    str.w tmp0, [sp, #3*4]
    fqmul_asm tmp0, a_4_or_5, a_6_or_7, qinv, q, tmp1
    mov.w tmp1, #342
    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1
    str.w tmp0, [sp, #4*4]
    str.w tmp0, [sp, #13*4]  // pe[5]
    fqmul_asm tmp0, a_6_or_7, a_6_or_7, qinv, q, tmp1
    str.w tmp0, [sp, #5*4]
    str.w tmp0, [sp, #14*4]  // pe[6]

    add.w sx0, a_0_or_1, a_4_or_5
    add.w sx1, a_2_or_3, a_6_or_7
    fqmul_asm tmp2, sx0, sx0, qinv, q, tmp3
    str.w tmp2, [sp, #6*4]
    fqmul_asm tmp2, sx0, sx1, qinv, q, tmp3
    mov.w tmp3, #342
    fqmul_asm tmp2, tmp2, tmp3, qinv, q, tmp3
    str.w tmp2, [sp, #7*4]
    fqmul_asm tmp2, sx1, sx1, qinv, q, tmp3
    //str.w tmp2, [sp, #8*4]

    ldr.w tmp0, [sp, #3*4] //q0
    ldr.w tmp1, [sp, #2*4] //p2
    ldr.w tmp3, [sp, #5*4]
    add.w tmp2, tmp0, tmp2 // q0 + m2
    add.w tmp3, tmp1, tmp3 // p2 + q2
    sub.w tmp2, tmp2, tmp3
    str.w tmp2, [sp, #12*4] // pe[4]

    ldr.w tmp2, [sp]       // p0
    ldr.w tmp3, [sp, #6*4] // m0
    add.w tmp0, tmp0, tmp2  // p0 + q0
    add.w tmp1, tmp1, tmp3  // p2 + m0
    sub.w tmp1, tmp1, tmp0
    str.w tmp1, [sp, #10*4] // pe[2]

    ldr.w tmp0, [sp, #7*4] // m1
    ldr.w tmp1, [sp, #1*4] // p1
    ldr.w tmp2, [sp, #4*4] // q1
    sub.w tmp0, tmp0, tmp1
    sub.w tmp0, tmp0, tmp2
    str.w tmp0, [sp, #11*4] // pe[3]

    /* po */
    ldrsh.w a_0_or_1, [addr_a, #1*2]
    ldrsh.w a_2_or_3, [addr_a, #3*2]
    ldrsh.w a_4_or_5, [addr_a, #5*2]
    ldrsh.w a_6_or_7, [addr_a, #7*2]

    /* pe[0] == sp + 8*4 */
    /* po[0] == sp + 15*4 */
    /* p0 == sp ; p1 == sp + 1*4 ; p2 == sp + 2*4 */
    /* q0 == sp + 3*4 ; q1 == sp + 4*4 ; q2 == sp + 5*4 */
    /* m0 == sp + 6*4 ; m1 == sp + 7*4 */

    fqmul_asm tmp0, a_0_or_1, a_0_or_1, qinv, q, tmp1
    str.w tmp0, [sp]
    str.w tmp0, [sp, #15*4]  // po[0]
    fqmul_asm tmp0, a_0_or_1, a_2_or_3, qinv, q, tmp1
    mov.w tmp1, #342
    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1
    str.w tmp0, [sp, #1*4]
    str.w tmp0, [sp, #16*4]  // po[1]
    fqmul_asm tmp0, a_2_or_3, a_2_or_3, qinv, q, tmp1
    str.w tmp0, [sp, #2*4]

    fqmul_asm tmp0, a_4_or_5, a_4_or_5, qinv, q, tmp1
    str.w tmp0, [sp, #3*4]
    fqmul_asm tmp0, a_4_or_5, a_6_or_7, qinv, q, tmp1
    mov.w tmp1, #342
    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1
    str.w tmp0, [sp, #4*4]
    str.w tmp0, [sp, #20*4]  // po[5]
    fqmul_asm tmp0, a_6_or_7, a_6_or_7, qinv, q, tmp1
    str.w tmp0, [sp, #5*4]
    str.w tmp0, [sp, #21*4]  // po[6]

    add.w sx0, a_0_or_1, a_4_or_5
    add.w sx1, a_2_or_3, a_6_or_7
    fqmul_asm tmp2, sx0, sx0, qinv, q, tmp3
    str.w tmp2, [sp, #6*4]
    fqmul_asm tmp2, sx0, sx1, qinv, q, tmp3
    mov.w tmp3, #342
    fqmul_asm tmp2, tmp2, tmp3, qinv, q, tmp3
    str.w tmp2, [sp, #7*4]
    fqmul_asm tmp2, sx1, sx1, qinv, q, tmp3
    //str.w tmp2, [sp, #8*4]

    ldr.w tmp0, [sp, #3*4] //q0
    ldr.w tmp1, [sp, #2*4] //p2
    ldr.w tmp3, [sp, #5*4]
    add.w tmp2, tmp0, tmp2 // q0 + m2
    add.w tmp3, tmp1, tmp3 // p2 + q2
    sub.w tmp2, tmp2, tmp3
    str.w tmp2, [sp, #19*4] // po[4]

    ldr.w tmp2, [sp]       // p0
    ldr.w tmp3, [sp, #6*4] // m0
    add.w tmp0, tmp0, tmp2  // p0 + q0
    add.w tmp1, tmp1, tmp3  // p2 + m0
    sub.w tmp1, tmp1, tmp0
    str.w tmp1, [sp, #17*4] // po[2]

    ldr.w tmp0, [sp, #7*4] // m1
    ldr.w tmp1, [sp, #1*4] // p1
    ldr.w tmp2, [sp, #4*4] // q1
    sub.w tmp0, tmp0, tmp1
    sub.w tmp0, tmp0, tmp2
    str.w tmp0, [sp, #18*4] // po[3]

    /* b0 b1 b2 b3 ; sp + 22*4 */
    ldr.w tmp1, [sp, #12*4] // pe[4]
    ldr.w tmp2, [sp, #8*4]  // pe[0]
    sub.w tmp1, tmp1, tmp0  // pe[4] - p0[3]
    fqmul_asm tmp0, tmp1, zeta, qinv, q, tmp3
    sub.w b0, tmp2, tmp0     // b0

    ldr.w tmp0, [sp, #11*4] // pe[3]
    ldr.w tmp1, [sp, #17*4]  // po[2]
    ldr.w tmp2, [sp, #21*4] // po[6]
    fqmul_asm tmp2, tmp2, zeta, qinv, q, tmp3
    sub.w tmp0, tmp0, tmp1
    add.w b3, tmp0, tmp2  // b3

    ldr.w tmp0, [sp, #9*4] // pe[1]
    ldr.w tmp1, [sp, #15*4]  // po[0]
    ldr.w tmp2, [sp, #13*4] // pe[5]
    ldr.w tmp3, [sp, #19*4]  // po[4]
    sub.w tmp2, tmp2, tmp3  //  pe[5] - po[4]
    fqmul_asm tmp2, tmp2, zeta, qinv, q, tmp3
    sub.w tmp0, tmp0, tmp1
    sub.w b1, tmp0, tmp2 // b1

    ldr.w tmp0, [sp, #10*4] // pe[2]
    ldr.w tmp1, [sp, #16*4]  // po[1]
    ldr.w tmp, [sp, #14*4] // pe[6]
    ldr.w tmp3, [sp, #20*4]  // po[5]
    sub.w tmp, tmp, tmp3  //  pe[6] - po[5]
    fqmul_asm tmp, tmp, zeta, qinv, q, tmp3
    sub.w tmp0, tmp0, tmp1
    sub.w b2, tmp0, tmp // b2

    str.w b0, [sp, #22*4]
    str.w b1, [sp, #23*4]
    str.w b2, [sp, #24*4]
    str.w b3, [sp, #25*4]

    /* p0 == sp ; p1 == sp + 1*4 ; p2 == sp + 2*4 */
    /* q0 == sp + 3*4 ; q1 == sp + 4*4 ; q2 == sp + 5*4 */
    /* c0 == r8 ; c1 == r9 */
    fqmul_asm tmp0, b0, b0, qinv, q, tmp1 // p0
    fqmul_asm tmp1, b0, b2, qinv, q, tmp
    mov.w tmp, #342
    fqmul_asm tmp1, tmp1, tmp, qinv, q, tmp // p1
    str.w tmp0, [sp]
    str.w tmp1, [sp, #1*4]
    fqmul_asm tmp0, b2, b2, qinv, q, tmp1 // p2
    str.w tmp0, [sp, #2*4]

    fqmul_asm tmp0, b1, b1, qinv, q, tmp1 // q0
    fqmul_asm tmp1, b1, b3, qinv, q, tmp
    mov.w tmp, #342
    fqmul_asm tmp1, tmp1, tmp, qinv, q, tmp // q1
    str.w tmp0, [sp, #3*4]
    str.w tmp1, [sp, #4*4]
    fqmul_asm tmp0, b3, b3, qinv, q, tmp1 // q2

    /* c0 c1 */
    fqmul_asm tmp0, tmp0, zeta, qinv, q, tmp1
    ldr.w c_one, [sp, #1*4] // p1
    ldr.w tmp1, [sp, #3*4]  // q0
    ldr.w tmp2, [sp, #2*4] // p2
    ldr.w tmp3, [sp, #4*4]  // q1
    ldr.w c_zero, [sp]       // p0
    sub.w tmp2, tmp2, tmp3    // p2 - q1
    fqmul_asm tmp2, tmp2, zeta, qinv, q, tmp3
    sub.w tmp1, tmp1, tmp0
    sub.w c_zero, c_zero, tmp2
    sub.w c_one, c_one, tmp1

    /* e == tmp0 ; t == tmp1 */
    fqmul_asm tmp0, c_one, c_one, qinv, q, tmp2
    fqmul_asm tmp0, tmp0, zeta, qinv, q, tmp2
    fqmul_asm tmp1, c_zero, c_zero, qinv, q, tmp2
    add.w tmp0, tmp0, tmp1
    and.w tmp2, q, tmp0, asr #15
    add.w tmp0, tmp0, tmp2
    lsl.w tmp0, tmp0, #1
    ldr.w tmp2, [sp, #26*4] // sp + 26*4 == qinv array
    ldrsh.w tmp0, [tmp2, tmp0]  // tmp0 == e == qinv[e]
    fqmul_asm c_zero, tmp0, c_zero, qinv, q, tmp2
    fqmul_asm c_one, tmp0, c_one, qinv, q, tmp2
    mov.w tmp2, #171
    rsb.w tmp2, tmp2, #0
    fqmul_asm c_one, c_one, tmp2, qinv, q, tmp2

    /* p0 == sp ; p1 == sp + 1*4 ; p2 == sp + 2*4 */
    /* q0 == sp + 3*4 ; q1 == sp + 4*4 ; q2 == sp + 5*4 */
    /* f0 f1 f2 f3 reuse b0 b1 b2 b3 ; f0 == sp + 22*4 */

    ldr.w tmp0, [sp, #22*4] // b0
    ldr.w tmp2, [sp, #24*4] // b2
    fqmul_asm tmp1, c_zero, tmp0, qinv, q, tmp // p0
    fqmul_asm tmp3, c_one, tmp2, qinv, q, tmp  // p2
    add.w tmp0, tmp0, tmp2 // b0 + b2
    add.w tmp, c_one, c_zero
    fqmul_asm tmp0, tmp0, tmp, qinv, q, tmp
    add.w tmp, tmp1, tmp3
    sub.w tmp0, tmp0, tmp // p1
    /* tmp1 = p0 ; tmp0 == p1 ; tmp3 == p2 */
    fqmul_asm tmp3, tmp3, zeta, qinv, q, tmp
    sub.w tmp1, tmp1, tmp3 // f0
    str.w tmp0, [sp, #24*4]
    str.w tmp1, [sp, #22*4]

    ldr.w tmp0, [sp, #23*4] // b0
    ldr.w tmp2, [sp, #25*4] // b2
    fqmul_asm tmp1, c_zero, tmp0, qinv, q, tmp // q0
    fqmul_asm tmp3, c_one, tmp2, qinv, q, tmp  // q2
    add.w tmp0, tmp0, tmp2 // b0 + b2
    add.w tmp, c_one, c_zero
    fqmul_asm tmp0, tmp0, tmp, qinv, q, tmp
    add.w tmp, tmp1, tmp3
    sub.w tmp0, tmp0, tmp // q1
    /* tmp1 = q0 ; tmp0 == q1 ; tmp3 == q2 */
    fqmul_asm tmp3, tmp3, zeta, qinv, q, tmp
    sub.w tmp3, tmp3, tmp1 // f1
    mov.w tmp, #171
    rsb.w tmp, tmp, #0
    fqmul_asm tmp0, tmp0, tmp, qinv, q, tmp // f3
    str.w tmp3, [sp, #23*4]
    str.w tmp0, [sp, #25*4]

    /* pe[0] == sp + 8*4 */
    /* po[0] == sp + 15*4 */
    /* p0 == sp ; p1 == sp + 1*4 ; p2 == sp + 2*4 */
    /* q0 == sp + 3*4 ; q1 == sp + 4*4 ; q2 == sp + 5*4 */
    /* m0 == sp + 6*4 ; m1 == sp + 7*4 */

    /* pe */
    ldrsh.w a_zero, [addr_a]
    ldrsh.w a_one, [addr_a, #2*2]
    ldr.w tmp0, [sp, #22*4] // tmp0 == f0 ; tmp3 == f1
    fqmul_asm tmp1, tmp0, a_zero, qinv, q, tmp //p0
    fqmul_asm tmp2, tmp3, a_one, qinv, q, tmp //p2
    add.w tmp0, tmp0, tmp3
    add.w a_zero, a_zero, a_one
    add.w tmp, tmp1, tmp2 // p0 + p2
    fqmul_asm tmp0, tmp0, a_zero, qinv, q, a_zero
    sub.w tmp0, tmp0, tmp // p1
    str.w tmp1, [sp]        // p0
    str.w tmp0, [sp, #1*4]  // p1
    str.w tmp2, [sp, #2*4]  // p2
    str.w tmp1, [sp, #8*4]  // p0 -> pe[0]
    str.w tmp0, [sp, #9*4]  // p1 -> pe[1]

    ldrsh.w a_zero, [addr_a, #4*2]
    ldrsh.w a_one, [addr_a, #6*2]
    ldr.w tmp0, [sp, #24*4] // tmp0 == f2
    ldr.w tmp3, [sp, #25*4] // tmp3 == f3
    fqmul_asm tmp1, tmp0, a_zero, qinv, q, tmp //q0
    fqmul_asm tmp2, tmp3, a_one, qinv, q, tmp //q2
    add.w tmp0, tmp0, tmp3
    add.w a_zero, a_zero, a_one
    add.w tmp, tmp1, tmp2 // q0 + q2
    fqmul_asm tmp0, tmp0, a_zero, qinv, q, a_zero
    sub.w tmp0, tmp0, tmp // q1
    str.w tmp0, [sp, #4*4]  // q1
    str.w tmp2, [sp, #5*4]  // q2
    str.w tmp1, [sp, #3*4]  // q0
    str.w tmp0, [sp, #13*4]  // q1 -> pe[5]
    str.w tmp2, [sp, #14*4]  // q2 -> pe[6]

    ldrsh.w a_0_or_1, [sp, #22*4]
    ldrsh.w a_2_or_3, [sp, #23*4]
    ldrsh.w a_4_or_5, [sp, #24*4]
    ldrsh.w a_6_or_7, [sp, #25*4]
    add.w sx0, a_0_or_1, a_4_or_5
    add.w sx1, a_2_or_3, a_6_or_7

    ldrsh.w a_0_or_1, [addr_a]
    ldrsh.w a_2_or_3, [addr_a, #2*2]
    ldrsh.w a_4_or_5, [addr_a, #4*2]
    ldrsh.w a_6_or_7, [addr_a, #6*2]
    add.w sy0, a_0_or_1, a_4_or_5
    add.w sy1, a_2_or_3, a_6_or_7

    fqmul_asm tmp2, sx0, sy0, qinv, q, tmp // m0
    fqmul_asm tmp3, sx1, sy1, qinv, q, tmp // m2
    add.w sx0, sx0, sx1
    add.w sy0, sy0, sy1
    add.w tmp, tmp2, tmp3
    fqmul_asm sx0, sx0, sy0, qinv, q, sy0
    sub.w tmp, sx0, tmp                    // m1

    str.w tmp2, [sp, #6*4]
    str.w tmp, [sp, #7*4]

    ldr.w tmp0, [sp, #3*4] // q0
    ldr.w tmp1, [sp, #2*4] // p2
    ldr.w tmp2, [sp, #5*4] // q2 ; tmp3 == m2
    add.w tmp3, tmp0, tmp3
    add.w tmp2, tmp1, tmp2
    sub.w tmp3, tmp3, tmp2 // -> pe[4]

    ldr.w tmp, [sp, #6*4] // m0
    ldr.w tmp2, [sp] // p0
    add.w tmp1, tmp1, tmp  // p2 + m0
    add.w tmp0, tmp0, tmp2 // p0 + q0
    sub.w tmp1, tmp1, tmp0 // -> pe[2]

    ldr.w tmp, [sp, #7*4]   // m1
    ldr.w tmp0, [sp, #1*4]  // p1
    ldr.w tmp2, [sp, #4*4]  // q1
    add.w tmp0, tmp0, tmp2
    sub.w tmp, tmp, tmp0   // -> pe[3]

    strh.w tmp, [addr_r, #6*2]   // -> pe[3] == r[6]
    str.w tmp1, [sp, #10*4]  // -> pe[2]
    str.w tmp3, [sp, #12*4]  // -> pe[4]

    /* pe[0] == sp + 8*4 */
    /* po[0] == sp + 15*4 */
    /* p0 == sp ; p1 == sp + 1*4 ; p2 == sp + 2*4 */
    /* q0 == sp + 3*4 ; q1 == sp + 4*4 ; q2 == sp + 5*4 */
    /* m0 == sp + 6*4 ; m1 == sp + 7*4 */

    /* po */
    ldrsh.w a_zero, [addr_a, #1*2]
    ldrsh.w a_one, [addr_a, #3*2]
    ldr.w tmp0, [sp, #22*4] // tmp0 == f0
    ldr.w tmp3, [sp, #23*4] // tmp3 == f1
    fqmul_asm tmp1, tmp0, a_zero, qinv, q, tmp //p0
    fqmul_asm tmp2, tmp3, a_one, qinv, q, tmp //p2
    add.w tmp0, tmp0, tmp3
    add.w a_zero, a_zero, a_one
    add.w tmp, tmp1, tmp2 // p0 + p2
    fqmul_asm tmp0, tmp0, a_zero, qinv, q, a_zero
    sub.w tmp0, tmp0, tmp // p1
    str.w tmp1, [sp]        // p0
    str.w tmp0, [sp, #1*4]  // p1
    str.w tmp2, [sp, #2*4]  // p2
    str.w tmp1, [sp, #15*4]  // p0 -> po[0]
    str.w tmp0, [sp, #16*4]  // p1 -> po[1]

    ldrsh.w a_zero, [addr_a, #5*2]
    ldrsh.w a_one, [addr_a, #7*2]
    ldr.w tmp0, [sp, #24*4] // tmp0 == f2
    ldr.w tmp3, [sp, #25*4] // tmp3 == f3
    fqmul_asm tmp1, tmp0, a_zero, qinv, q, tmp //q0
    fqmul_asm tmp2, tmp3, a_one, qinv, q, tmp //q2
    add.w tmp0, tmp0, tmp3
    add.w a_zero, a_zero, a_one
    add.w tmp, tmp1, tmp2 // q0 + q2
    fqmul_asm tmp0, tmp0, a_zero, qinv, q, a_zero
    sub.w tmp0, tmp0, tmp // q1
    str.w tmp0, [sp, #4*4]  // q1
    str.w tmp2, [sp, #5*4]  // q2
    str.w tmp1, [sp, #3*4]  // q0
    str.w tmp0, [sp, #20*4]  // q1 -> po[5]
    str.w tmp2, [sp, #21*4]  // q2 -> po[6]

    ldrsh.w a_0_or_1, [sp, #22*4]
    ldrsh.w a_2_or_3, [sp, #23*4]
    ldrsh.w a_4_or_5, [sp, #24*4]
    ldrsh.w a_6_or_7, [sp, #25*4]
    add.w sx0, a_0_or_1, a_4_or_5
    add.w sx1, a_2_or_3, a_6_or_7

    ldrsh.w a_0_or_1, [addr_a, #1*2]
    ldrsh.w a_2_or_3, [addr_a, #3*2]
    ldrsh.w a_4_or_5, [addr_a, #5*2]
    ldrsh.w a_6_or_7, [addr_a, #7*2]
    add.w sy0, a_0_or_1, a_4_or_5
    add.w sy1, a_2_or_3, a_6_or_7

    fqmul_asm tmp2, sx0, sy0, qinv, q, tmp // m0
    fqmul_asm tmp3, sx1, sy1, qinv, q, tmp // m2
    add.w sx0, sx0, sx1
    add.w sy0, sy0, sy1
    add.w tmp, tmp2, tmp3
    fqmul_asm sx0, sx0, sy0, qinv, q, sy0
    sub.w tmp, sx0, tmp                    // m1

    str.w tmp2, [sp, #6*4]
    str.w tmp, [sp, #7*4]

    ldr.w tmp0, [sp, #3*4] // q0
    ldr.w tmp1, [sp, #2*4] // p2
    ldr.w tmp2, [sp, #5*4] // q2 ; tmp3 == m2
    add.w tmp3, tmp0, tmp3
    add.w tmp2, tmp1, tmp2
    sub.w tmp3, tmp3, tmp2 // -> po[4]

    ldr.w tmp, [sp, #6*4] // m0
    ldr.w tmp2, [sp] // p0
    add.w tmp1, tmp1, tmp  // p2 + m0
    add.w tmp0, tmp0, tmp2 // p0 + q0
    sub.w tmp1, tmp1, tmp0 // -> po[2]

    ldr.w tmp, [sp, #7*4]   // m1
    ldr.w tmp0, [sp, #1*4]  // p1
    ldr.w tmp2, [sp, #4*4]  // q1
    add.w tmp0, tmp0, tmp2
    sub.w tmp, tmp, tmp0   // -> po[3]

    str.w tmp, [sp, #18*4]   // -> po[3]
    str.w tmp1, [sp, #17*4]  // -> po[2]
    str.w tmp3, [sp, #19*4]  // -> po[4]

    // r[7]
    mov.w tmp0, #171
    rsb.w tmp0, tmp0, #0
    fqmul_asm tmp, tmp, tmp0, qinv, q, tmp0
    strh.w tmp, [addr_r, #7*2]

    // r[5]
    ldr.w tmp, [sp, #21*4]
    fqmul_asm tmp, tmp, zeta, qinv, q, tmp0
    sub.w tmp, tmp, tmp1
    strh.w tmp, [addr_r, #5*2]

    // r[1]
    fqmul_asm tmp3, tmp3, zeta, qinv, q, tmp0
    ldr.w tmp, [sp, #15*4]
    sub.w tmp3, tmp3, tmp
    strh.w tmp3, [addr_r, #1*2]

    // r[2] r[3] r[4]
    ldr.w tmp0, [sp, #9*4] // pe[1]
    ldr.w tmp1, [sp, #13*4] // pe[5]
    ldr.w tmp2, [sp, #20*4] // po[5]
    ldr.w tmp3, [sp, #16*4] // po[1]
    ldr.w r8, [sp, #10*4] // pe[2]
    ldr.w r9, [sp, #14*4] // pe[6]

    fqmul_asm tmp1, tmp1, zeta, qinv, q, tmp
    fqmul_asm tmp2, tmp2, zeta, qinv, q, tmp
    fqmul_asm r9, r9, zeta, qinv, q, tmp
    sub.w tmp0, tmp0, tmp1 // r[2]
    sub.w tmp2, tmp2, tmp3 // r[3]
    sub.w r8, r8, r9 // r[4]

    strh.w tmp0, [addr_r, #2*2]
    strh.w tmp2, [addr_r, #3*2]
    strh.w r8, [addr_r, #4*2]

    // r[0]
    ldr.w tmp0, [sp, #8*4]
    ldr.w tmp1, [sp, #12*4]
    fqmul_asm tmp1, tmp1, zeta, qinv, q, tmp
    sub.w tmp0, tmp0, tmp1 // r[0]
    strh.w tmp0, [addr_r], #8*2

    add.w addr_a, addr_a, #8*2
.endm

//void poly_baseinv_asm(int16_t *r,  int16_t *a, int16_t *f, int16_t *qinv)
.global poly_baseinv_asm
.type poly_baseinv_asm, %function
.align 2
poly_baseinv_asm:

addr_r .req r0
addr_a .req r1
addr_zeta .req r2
zeta .req r3
qinv .req r4
q .req r5
a_0_or_1 .req r6
a_2_or_3 .req r7
a_4_or_5 .req r8
a_6_or_7 .req r9
a_zero .req r8
a_one .req r9
sy0 .req r8
sy1 .req r9
sx0 .req r10
sx1 .req r11
tmp0 .req r10
tmp1 .req r11
tmp2 .req r6
tmp3 .req r7
b0 .req r8
b1 .req r6
b2 .req r7
b3 .req r9
c_zero .req r8
c_one .req r9
tmp .req r12
loop .req r14

    push {r4-r11, lr}
    sub.w sp, sp, #27*4

    str.w r3, [sp, #26*4] // qinv array

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    add.w addr_zeta, addr_zeta, #128

    mov.w loop, #64
    poly_baseinv_lp:

        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_inv zeta
        rsb.w zeta, zeta, #0
        base_inv zeta

        subs.w loop, loop, #1
        bne.w poly_baseinv_lp

.unreq addr_r
.unreq addr_a
.unreq addr_zeta
.unreq zeta
.unreq qinv
.unreq q
.unreq a_0_or_1
.unreq a_2_or_3
.unreq a_4_or_5
.unreq a_6_or_7
.unreq a_zero
.unreq a_one
.unreq sy0
.unreq sy1
.unreq sx0
.unreq sx1
.unreq tmp0
.unreq tmp1
.unreq tmp2
.unreq tmp3
.unreq b0
.unreq b1
.unreq b2
.unreq b3
.unreq c_zero
.unreq c_one
.unreq tmp
.unreq loop

    add.w sp, sp, #27*4
    pop {r4-r11, pc}
.size poly_baseinv_asm, .-poly_baseinv_asm
