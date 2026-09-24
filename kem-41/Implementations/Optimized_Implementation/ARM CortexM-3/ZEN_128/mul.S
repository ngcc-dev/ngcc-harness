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

.macro base_mul zeta
    ldrsh.w c_0, [addr_a]
    ldrsh.w c_1, [addr_a, #1*2]
    ldrsh.w c_2, [addr_a, #2*2]
    ldrsh.w tmp0, [addr_b]
    ldrsh.w tmp1, [addr_b, #1*2]
    ldrsh.w tmp2, [addr_b, #2*2]
    fqmul_asm c_0, c_0, tmp0, qinv, q, tmp0
    fqmul_asm c_1, c_1, tmp1, qinv, q, tmp1
    fqmul_asm c_2, c_2, tmp2, qinv, q, tmp2

    ldrsh.w c_3, [addr_a, #3*2]
    ldrsh.w tmp0, [addr_b, #3*2]
    fqmul_asm c_3, c_3, tmp0, qinv, q, tmp0

// compute r[0]
    ldrsh.w tmp0, [addr_a, #1*2]
    ldrsh.w tmp1, [addr_a, #3*2]
    add.w tmp0, tmp0, tmp1
    ldrsh.w tmp1, [addr_b, #1*2]
    ldrsh.w tmp2, [addr_b, #3*2]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[0]
    add.w tmp2, c_1, c_3
    add.w tmp0, tmp0, c_2
    sub.w tmp0, tmp0, tmp2
    fqmul_asm tmp0, tmp0, \zeta, qinv, q, tmp1   // fqmul(r[0], zeta);
    add.w tmp0, tmp0, c_0

    strh.w tmp0, [addr_r]

// compute r[2]
    ldrsh.w tmp0, [addr_a]
    ldrsh.w tmp1, [addr_a, #2*2]
    add.w tmp0, tmp0, tmp1
    ldrsh.w tmp1, [addr_b]
    ldrsh.w tmp2, [addr_b, #2*2]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[2]
    add.w tmp0, tmp0, c_1
    fqmul_asm tmp1, c_3, \zeta, qinv, q, tmp2     // fqmul(c3, zeta);
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_0, c_2
    sub.w tmp0, tmp0, tmp2

    strh.w tmp0, [addr_r, #2*2]

// compute r[1]
    ldrsh.w tmp0, [addr_a, #2*2]
    ldrsh.w tmp1, [addr_a, #3*2]
    add.w tmp0, tmp0, tmp1
    ldrsh.w tmp1, [addr_b, #2*2]
    ldrsh.w tmp2, [addr_b, #3*2]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[1]
    add.w tmp2, c_2, c_3
    sub.w tmp0, tmp0, tmp2
    fqmul_asm tmp0, tmp0, \zeta, qinv, q, tmp1   // fqmul(r[1], zeta);
    ldrsh.w tmp1, [addr_a]
    ldrsh.w tmp2, [addr_a, #1*2]
    add.w tmp1, tmp1, tmp2                       // tmp0 == a0 + a1
    ldrsh.w tmp2, [addr_b]
    ldrsh.w \zeta, [addr_b, #1*2]
    add.w tmp2, tmp2, \zeta                      // tmp2 == b0 + b1
    fqmul_asm tmp1, tmp1, tmp2, qinv, q, tmp2
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_0, c_1
    sub.w tmp0, tmp0, tmp2

    strh.w tmp0, [addr_r, #1*2]

// compute r[3]
    ldrsh.w tmp0, [addr_a]
    ldrsh.w tmp1, [addr_a, #3*2]
    add.w tmp0, tmp0, tmp1
    ldrsh.w tmp1, [addr_b]
    ldrsh.w tmp2, [addr_b, #3*2]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[3]
    add.w tmp2, c_0, c_3
    sub.w tmp0, tmp0, tmp2
    ldrsh.w tmp1, [addr_a, #1*2]
    ldrsh.w tmp2, [addr_a, #2*2]
    add.w tmp1, tmp1, tmp2                      // tmp1 == a1 + a2
    ldrsh.w tmp2, [addr_b, #1*2]
    ldrsh.w \zeta, [addr_b, #2*2]
    add.w tmp2, tmp2, \zeta                      // tmp2 == b1 + b2
    fqmul_asm tmp1, tmp1, tmp2, qinv, q, tmp2
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_1, c_2
    sub.w tmp0, tmp0, tmp2

    strh.w tmp0, [addr_r, #3*2]

// update addr of a, b, r
    add.w addr_a, addr_a, #8
    add.w addr_b, addr_b, #8
    add.w addr_r, addr_r, #8
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
    sub.w sp, sp, #4

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    add.w addr_zeta, addr_zeta, #128

    mov.w loop, #16
    str.w loop, [sp]
    basemul_loop:
        ldrsh.w zeta, [addr_zeta]
        base_mul zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul zeta

        ldrsh.w zeta, [addr_zeta]
        base_mul zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul zeta

        ldrsh.w zeta, [addr_zeta]
        base_mul zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul zeta

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

    add.w sp, sp, #4
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
    ldrsh.w c_0, [addr_a]
    ldrsh.w c_1, [addr_a, #1*2]
    ldrsh.w c_2, [addr_a, #2*2]
    ldrsh.w tmp0, [addr_b]
    ldrsh.w tmp1, [addr_b, #1*2]
    ldrsh.w tmp2, [addr_b, #2*2]
    fqmul_asm c_0, c_0, tmp0, qinv, q, tmp0
    fqmul_asm c_1, c_1, tmp1, qinv, q, tmp1
    fqmul_asm c_2, c_2, tmp2, qinv, q, tmp2

    ldrsh.w c_3, [addr_a, #3*2]
    ldrsh.w tmp0, [addr_b, #3*2]
    fqmul_asm c_3, c_3, tmp0, qinv, q, tmp0

// compute r[0]
    ldrsh.w tmp0, [addr_a, #1*2]
    ldrsh.w tmp1, [addr_a, #3*2]
    add.w tmp0, tmp0, tmp1
    ldrsh.w tmp1, [addr_b, #1*2]
    ldrsh.w tmp2, [addr_b, #3*2]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[0]
    add.w tmp2, c_1, c_3
    add.w tmp0, tmp0, c_2
    sub.w tmp0, tmp0, tmp2
    fqmul_asm tmp0, tmp0, \zeta, qinv, q, tmp1   // fqmul(r[0], zeta);
    add.w tmp0, tmp0, c_0

    mq_finish tmp0, tmp1
    strh.w tmp0, [addr_r]

// compute r[2]
    ldrsh.w tmp0, [addr_a]
    ldrsh.w tmp1, [addr_a, #2*2]
    add.w tmp0, tmp0, tmp1
    ldrsh.w tmp1, [addr_b]
    ldrsh.w tmp2, [addr_b, #2*2]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[2]
    add.w tmp0, tmp0, c_1
    fqmul_asm tmp1, c_3, \zeta, qinv, q, tmp2     // fqmul(c3, zeta);
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_0, c_2
    sub.w tmp0, tmp0, tmp2

    mq_finish tmp0, tmp1
    strh.w tmp0, [addr_r, #2*2]

// compute r[1]
    ldrsh.w tmp0, [addr_a, #2*2]
    ldrsh.w tmp1, [addr_a, #3*2]
    add.w tmp0, tmp0, tmp1
    ldrsh.w tmp1, [addr_b, #2*2]
    ldrsh.w tmp2, [addr_b, #3*2]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[1]
    add.w tmp2, c_2, c_3
    sub.w tmp0, tmp0, tmp2
    fqmul_asm tmp0, tmp0, \zeta, qinv, q, tmp1   // fqmul(r[1], zeta);
    ldrsh.w tmp1, [addr_a]
    ldrsh.w tmp2, [addr_a, #1*2]
    add.w tmp1, tmp1, tmp2                       // tmp0 == a0 + a1
    ldrsh.w tmp2, [addr_b]
    ldrsh.w \zeta, [addr_b, #1*2]
    add.w tmp2, tmp2, \zeta                      // tmp2 == b0 + b1
    fqmul_asm tmp1, tmp1, tmp2, qinv, q, tmp2
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_0, c_1
    sub.w tmp0, tmp0, tmp2

    mq_finish tmp0, tmp1
    strh.w tmp0, [addr_r, #1*2]

// compute r[3]
    ldrsh.w tmp0, [addr_a]
    ldrsh.w tmp1, [addr_a, #3*2]
    add.w tmp0, tmp0, tmp1
    ldrsh.w tmp1, [addr_b]
    ldrsh.w tmp2, [addr_b, #3*2]
    add.w tmp1, tmp1, tmp2

    fqmul_asm tmp0, tmp0, tmp1, qinv, q, tmp1   // tmp0 == r[3]
    add.w tmp2, c_0, c_3
    sub.w tmp0, tmp0, tmp2
    ldrsh.w tmp1, [addr_a, #1*2]
    ldrsh.w tmp2, [addr_a, #2*2]
    add.w tmp1, tmp1, tmp2                      // tmp1 == a1 + a2
    ldrsh.w tmp2, [addr_b, #1*2]
    ldrsh.w \zeta, [addr_b, #2*2]
    add.w tmp2, tmp2, \zeta                      // tmp2 == b1 + b2
    fqmul_asm tmp1, tmp1, tmp2, qinv, q, tmp2
    add.w tmp0, tmp0, tmp1
    add.w tmp2, c_1, c_2
    sub.w tmp0, tmp0, tmp2

    mq_finish tmp0, tmp1
    strh.w tmp0, [addr_r, #3*2]

// update addr of a, b, r
    add.w addr_a, addr_a, #8
    add.w addr_b, addr_b, #8
    add.w addr_r, addr_r, #8
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
zeta .req r10
tmp0 .req r11
tmp1 .req r12
tmp2 .req r14
loop .req r11


    push {r4-r11, lr}
    sub.w sp, sp, #4

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    add.w addr_zeta, addr_zeta, #128

    mov.w loop, #16
    str.w loop, [sp]
    basemul_mq_loop:
        ldrsh.w zeta, [addr_zeta]
        base_mul_mq zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul_mq zeta

        ldrsh.w zeta, [addr_zeta]
        base_mul_mq zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul_mq zeta

        ldrsh.w zeta, [addr_zeta]
        base_mul_mq zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul_mq zeta

        ldrsh.w zeta, [addr_zeta]
        base_mul_mq zeta
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        base_mul_mq zeta

        ldr.w loop, [sp]
        subs.w loop, loop, #1
        str.w loop, [sp]
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

    add.w sp, sp, #4
    pop {r4-r11, pc}
.size poly_basemul_mq_asm, .-poly_basemul_mq_asm

.macro base_inv zeta, zeta2
    ldrsh.w tmp0, [addr_a]
    ldrsh.w tmp1, [addr_a, #1*2]
    ldrsh.w r, [addr_a, #2*2]
    ldrsh.w tmp3, [addr_a, #3*2]
    fqmul_asm tmp2, r, r, qinv, q, r            // tmp2 == fqmul(a[2], a[2])
    fqmul_asm t, tmp1, tmp3, qinv, q, tmp
    str.w t, [sp, #2*4]                      // sp + 2*4 == fqmul(a[1], a[3])  first
    mov.w tmp, #342
    fqmul_asm t, t, tmp, qinv, q, tmp   // t == t
    add.w r, tmp2, t                         // r == r0
    fqmul_asm r, r, tmp0, qinv, q, tmp
    fqmul_asm tmp0, tmp0, tmp0, qinv, q, t   // tmp0 == fqmul(r[0], a[0])
    fqmul_asm tmp1, tmp1, tmp1, qinv, q, t      // tmp0 == fqmul(r[1], a[1])
    ldrsh.w t, [addr_a, #2*2]
    fqmul_asm tmp3, tmp1, t, qinv, q, t
    sub.w r, r, tmp3
    fqmul_asm r, r, \zeta, qinv, q, tmp
    ldrsh.w tmp3, [addr_a, #3*2]
    fqmul_asm tmp3, tmp3, tmp3, qinv, q, t   // tmp3 == fqmul(r[3], a[3])
    ldrsh.w t, [addr_a, #2*2]
    fqmul_asm t, tmp3, t, qinv, q, tmp
    fqmul_asm t, t, \zeta2, qinv, q, tmp
    sub.w r, r, t
    ldrsh.w t, [addr_a]
    fqmul_asm t, t, tmp0, qinv, q, tmp
    sub.w r, r, t
    mov.w t, #173
    fqmul_asm r, r, t, qinv, q, tmp
    strh.w r, [addr_r]

    ldrsh.w r, [addr_a, #1*2]
    ldrsh.w t, [addr_a, #2*2]
    fqmul_asm r, r, t, qinv, q, t
    ldrsh.w tmp, [addr_a]
    ldrsh.w t, [addr_a, #3*2]
    fqmul_asm t, tmp, t, qinv, q, tmp
    mov.w tmp, #342
    fqmul_asm t, t, tmp, qinv, q, tmp
    sub.w r, r, t
    ldrsh.w t, [addr_a, #2*2]
    fqmul_asm r, r, t, qinv, q, t
    ldrsh.w t, [addr_a, #3*2]
    fqmul_asm t, tmp1, t, qinv, q, tmp
    sub.w r, r, t
    fqmul_asm r, r, \zeta, qinv, q, tmp
    ldrsh.w t, [addr_a, #3*2]
    fqmul_asm t, tmp3, t, qinv, q, tmp
    fqmul_asm t, t, \zeta2, qinv, q, tmp
    add.w r, r, t
    ldrsh.w t, [addr_a, #1*2]
    fqmul_asm t, tmp0, t, qinv, q, tmp
    add.w r, r, t
    mov.w t, #173
    fqmul_asm r, r, t, qinv, q, tmp
    strh.w r, [addr_r, #1*2]

    ldr.w r, [sp, #2*4]                 // sp + 2*4 == fqmul(a[1], a[3])
    mov.w t, #342
    fqmul_asm r, r, t, qinv, q, t
    sub.w r, r, tmp2
    ldrsh.w t, [addr_a, #2*2]
    fqmul_asm r, r, t, qinv, q, t
    ldrsh.w t, [addr_a]
    fqmul_asm t, tmp3, t, qinv, q, tmp
    sub.w r, r, t
    fqmul_asm r, r, \zeta, qinv, q, tmp
    ldrsh.w t, [addr_a, #2*2]
    fqmul_asm t, tmp0, t, qinv, q, tmp
    add.w r, r, t
    ldrsh.w t, [addr_a]
    fqmul_asm t, tmp1, t, qinv, q, tmp
    sub.w r, r, t
    mov.w t, #173
    fqmul_asm r, r, t, qinv, q, tmp
    strh.w r, [addr_r, #2*2]

    ldrsh.w t, [addr_a, #3*2]
    ldr.w r, [sp, #2*4]                 // sp + 2*4 == fqmul(a[1], a[3])
    sub.w r, tmp2, r
    fqmul_asm r, r, t, qinv, q, tmp
    fqmul_asm r, r, \zeta, qinv, q, tmp
    ldrsh.w t, [addr_a, #1*2]
    fqmul_asm t, tmp1, t, qinv, q, tmp
    add.w r, r, t
    ldrsh.w tmp, [addr_a]
    ldrsh.w t, [addr_a, #2*2]
    fqmul_asm t, tmp, t, qinv, q, tmp
    str.w t, [sp, #3*4]                 // sp + 3*4 == fqmul(a[0], a[2]) first
    ldrsh.w tmp, [addr_a, #1*2]
    fqmul_asm t, t, tmp, qinv, q, tmp
    mov.w tmp, #342
    fqmul_asm t, t, tmp, qinv, q, tmp
    sub.w r, r, t
    ldrsh.w t, [addr_a, #3*2]
    fqmul_asm t, tmp0, t, qinv, q, tmp
    add.w r, r, t
    mov.w t, #173
    fqmul_asm r, r, t, qinv, q, tmp
    strh.w r, [addr_r, #3*2]

    ldr.w r, [sp, #2*4]                 // sp + 2*4 == fqmul(a[1], a[3])
    mov.w t, #684
    fqmul_asm r, r, t, qinv, q, tmp
    sub.w det, tmp2, r
    fqmul_asm det, det, tmp2, qinv, q, tmp
    ldr.w t, [sp, #3*4]                 // sp + 3*4 == fqmul(a[0], a[2])
    mov.w tmp, #342
    fqmul_asm t, t, tmp, qinv, q, tmp
    add.w t, t, tmp1
    fqmul_asm t, t, tmp3, qinv, q, tmp
    mov.w tmp, #342
    fqmul_asm t, t, tmp, qinv, q, tmp
    add.w det, det, t
    fqmul_asm det, det, \zeta2, qinv, q, tmp
    fqmul_asm t, tmp3, tmp3, qinv, q, tmp
    fqmul_asm t, t, \zeta2, qinv, q, tmp
    fqmul_asm t, t, \zeta, qinv, q, tmp
    sub.w det, t, det
    fqmul_asm t, tmp0, tmp0, qinv, q, tmp
    sub.w det, det, t
    ldr.w t, [sp, #2*4]                 // sp + 2*4 == fqmul(a[1], a[3])
    mov.w tmp, #342
    fqmul_asm t, t, tmp, qinv, q, tmp
    add.w t, t, tmp2
    mov.w tmp, #342
    fqmul_asm t, t, tmp, qinv, q, tmp
    fqmul_asm t, t, tmp0, qinv, q, tmp
    ldr.w k, [sp, #3*4]                 // sp + 3*4 == fqmul(a[0], a[2])
    mov.w tmp, #684
    rsb.w tmp, tmp, #0
    fqmul_asm k, k, tmp, qinv, q, tmp
    add.w k, k, tmp1
    fqmul_asm k, k, tmp1, qinv, q, tmp
    add.w t, t, k
    fqmul_asm t, t, \zeta, qinv, q, tmp
    add.w det, det, t
    mov.w t, #361
    fqmul_asm det, det, t, qinv, q, tmp
    and.w t, q, det, lsr #15
    add.w det, det, t
    lsl.w det, det, #1
    ldr.w addr_qinv, [sp]
    ldrsh.w det, [addr_qinv, det]

    ldrsh.w r_0, [addr_r]
    ldrsh.w r_1, [addr_r, #1*2]
    ldrsh.w r_2, [addr_r, #2*2]
    ldrsh.w r_3, [addr_r, #3*2]
    mov.w t, #19
    fqmul_asm r_0, r_0, det, qinv, q, tmp
    fqmul_asm r_0, r_0, t, qinv, q, tmp
    fqmul_asm r_1, r_1, det, qinv, q, tmp
    fqmul_asm r_1, r_1, t, qinv, q, tmp
    fqmul_asm r_2, r_2, det, qinv, q, tmp
    fqmul_asm r_2, r_2, t, qinv, q, tmp
    fqmul_asm r_3, r_3, det, qinv, q, tmp
    fqmul_asm r_3, r_3, t, qinv, q, tmp

    strh.w r_1, [addr_r, #1*2]
    strh.w r_2, [addr_r, #2*2]
    strh.w r_3, [addr_r, #3*2]
    strh.w r_0, [addr_r], #4*2
    add.w addr_a, addr_a, #8

.endm

.global poly_baseinv_asm
.type poly_baseinv_asm, %function
.align 2
poly_baseinv_asm:

addr_r .req r0
addr_a .req r1
addr_zeta .req r2
zeta .req r3
zeta2 .req r4
qinv .req r5
q .req r6
r .req r7
det .req r7
tmp .req r8
tmp0 .req r9
tmp1 .req r10
tmp2 .req r11
tmp3 .req r14
t .req r12
addr_qinv .req r12
k .req r11
r_0 .req r3
r_1 .req r4
r_2 .req r9
r_3 .req r10
loop .req r7


    push {r4-r11, lr}
    sub.w sp, sp, #4*4

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    add.w addr_zeta, addr_zeta, #128

    str.w r3, [sp] // push addr_qinv

    mov.w loop, #64
    str.w loop, [sp, #1*4]
    base_inv_lp:
        ldrsh.w zeta, [addr_zeta]
        fqmul_asm zeta2, zeta, zeta, qinv, q, tmp
        base_inv zeta, zeta2
        ldrsh.w zeta, [addr_zeta], #2
        rsb.w zeta, zeta, #0
        fqmul_asm zeta2, zeta, zeta, qinv, q, tmp
        base_inv zeta, zeta2

        ldr.w loop, [sp, #1*4]
        subs.w loop, loop, #1
        str.w loop, [sp, #1*4]
        bne.w base_inv_lp

.unreq addr_r
.unreq addr_a
.unreq addr_zeta
.unreq zeta
.unreq zeta2
.unreq qinv
.unreq q
.unreq r
.unreq det
.unreq tmp
.unreq tmp0
.unreq tmp1
.unreq tmp2
.unreq tmp3
.unreq t
.unreq addr_qinv
.unreq k
.unreq r_0
.unreq r_1
.unreq r_2
.unreq r_3
.unreq loop

    add.w sp, sp, #4*4
    pop {r4-r11, pc}
.size poly_baseinv_asm, .-poly_baseinv_asm

