.syntax unified
.thumb

.global montgomery_reduce
.type montgomery_reduce, %function
.align 2
montgomery_reduce:

a_i .req r0
a_o .req r0
qinv .req r1
q .req r2
tmp .req r3

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01 //qinv
    movw q, #769                    //q

    mul.w tmp, a_i, qinv
    sxth tmp, tmp
    mul.w tmp, tmp, q
    sub.w a_o, a_i, tmp
    asr.w a_o, a_o, #16

.unreq a_i
.unreq a_o
.unreq qinv
.unreq q
.unreq tmp

    bx lr
.size montgomery_reduce, .-montgomery_reduce

.macro fqmul_asm re, a, b, qinv, q, tmp
    mul.w \re, \a, \b
    mul.w \tmp, \re, \qinv
    sxth \tmp, \tmp
    mul.w \tmp, \tmp, \q
    sub.w \re, \re, \tmp
    asr.w \re, \re, #16
.endm

.macro butterfly_wrap offset
    ldrsh.w a_h, [addr_a]
    ldrsh.w a_l, [addr_a, #\offset]
    fqmul_asm a_r, a_l, zeta, qinv, q, tmp // now a_r == bx
    sub.w a_l, a_h, a_r                // a_l == a - bx
    add.w a_h, a_h, a_r                // a_h == a + bx
    strh.w a_h, [addr_a], #2
    strh.w a_l, [addr_a, #\offset-2]
.endm

//void poly_ntt_asm(int16_t *a, const int16_t f[128]);
.global poly_ntt_asm
.type poly_ntt_asm, %function
.align 2
poly_ntt_asm:

addr_a .req r0
addr_zeta .req r1
qinv .req r2
q .req r3
zeta .req r4
loop .req r5
a_h .req r6
a_l .req r7
a_r .req r8
tmp .req r12

    push {r4-r8}

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

//level 9
    movw loop, #512
    add.w addr_zeta, addr_zeta, #2
    ldrsh.w zeta, [addr_zeta], #2
    level_9:
        butterfly_wrap 1024
        subs.w loop, loop, #1
        bne.w level_9
    sub.w addr_a, addr_a, #1024     //back to &a[0]

//level 8
    movw loop, #256
    ldrsh.w zeta, [addr_zeta], #2
    level_8_1:
        butterfly_wrap 512
        subs.w loop, loop, #1
        bne.w level_8_1
    add.w addr_a, addr_a, #512     //go to &a[512]

    movw loop, #256
    ldrsh.w zeta, [addr_zeta], #2
    level_8_2:
        butterfly_wrap 512
        subs.w loop, loop, #1
        bne.w level_8_2
    sub.w addr_a, addr_a, #1536     //back to &a[0]

//level 7
    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_1:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_1
    add.w addr_a, addr_a, #256     //go to &a[256]

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_2:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_2
    add.w addr_a, addr_a, #256     //go to &a[512]

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_3:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_3
    add.w addr_a, addr_a, #256     //go to &a[768]

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_4:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_4
    sub.w addr_a, addr_a, #1792     //back to &a[0]

//level 6
    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_1:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_1
    add.w addr_a, addr_a, #128

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_2:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_2
    add.w addr_a, addr_a, #128

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_3:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_3
    add.w addr_a, addr_a, #128

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_4:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_4
    add.w addr_a, addr_a, #128

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_5:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_5
    add.w addr_a, addr_a, #128

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_6:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_6
    add.w addr_a, addr_a, #128

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_7:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_7
    add.w addr_a, addr_a, #128

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_8:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_8
    sub.w addr_a, addr_a, #1920     //back to &a[0]

//level 5
    mov.w loop, #16
    level_4:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 32
            butterfly_wrap 64
        .endr
        add.w addr_a, addr_a, #64
        subs.w loop, loop, #1
        bne.w level_4
    sub.w addr_a, addr_a, #2048

//level 4
    mov.w loop, #32
    level_3:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 16
            butterfly_wrap 32
        .endr
        add.w addr_a, addr_a, #32
        subs.w loop, loop, #1
        bne.w level_3
    sub.w addr_a, addr_a, #2048

//level 3
    mov.w loop, #64
    level_2:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 8
            butterfly_wrap 16
        .endr
        add.w addr_a, addr_a, #16
        subs.w loop, loop, #1
        bne.w level_2
    //sub.w addr_a, addr_a, #2048

    pop {r4-r8}

.unreq addr_a
.unreq addr_zeta
.unreq qinv
.unreq q
.unreq zeta
.unreq loop
.unreq a_h
.unreq a_l
.unreq a_r
.unreq tmp

    bx lr
.size poly_ntt_asm, .-poly_ntt_asm

//void poly_ntt_mq_asm(int16_t *a, const int16_t f[128]);
.global poly_ntt_mq_asm
.type poly_ntt_mq_asm, %function
.align 2
poly_ntt_mq_asm:

addr_a .req r0
tmp .req r1
qinv .req r2
q .req r3
mont .req r4
loop .req r5
a_r .req r12

    push {r4-r5, lr}
    bl poly_ntt_asm
    sub.w addr_a, addr_a, #2048

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769
    mov.w mont, #171

// mq_reduce
    mov.w loop, #1024
    mq_reduce_lp:
        ldrsh.w a_r, [addr_a]
        fqmul_asm a_r, a_r, mont, qinv, q, tmp
        sxth a_r, a_r
        asr.w tmp, a_r, #15
        and.w tmp, tmp, q
        add.w a_r, a_r, tmp
        strh.w a_r, [addr_a], #2
        subs.w loop, loop, #1
        bne.w mq_reduce_lp

    pop {r4-r5, pc}

.unreq addr_a
.unreq tmp
.unreq qinv
.unreq q
.unreq mont
.unreq loop
.unreq a_r

.size poly_ntt_mq_asm, .-poly_ntt_mq_asm
