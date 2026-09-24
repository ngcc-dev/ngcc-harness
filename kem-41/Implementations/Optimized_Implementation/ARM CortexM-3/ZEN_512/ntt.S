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

//level 10
    movw loop, #1024
    add.w addr_zeta, addr_zeta, #2
    ldrsh.w zeta, [addr_zeta], #2
    level_10:
        butterfly_wrap 2048
        subs.w loop, loop, #1
        bne.w level_10
    sub.w addr_a, addr_a, #2048     //back to &a[0]

//level 9
    movw loop, #512
    ldrsh.w zeta, [addr_zeta], #2
    level_9_1:
        butterfly_wrap 1024
        subs.w loop, loop, #1
        bne.w level_9_1
    add.w addr_a, addr_a, #1024

    movw loop, #512
    ldrsh.w zeta, [addr_zeta], #2
    level_9_2:
        butterfly_wrap 1024
        subs.w loop, loop, #1
        bne.w level_9_2
    sub.w addr_a, addr_a, #3072     //back to &a[0]

//level 8
    mov.w loop, #256
    ldrsh.w zeta, [addr_zeta], #2
    level_8_1:
        butterfly_wrap 512
        subs.w loop, loop, #1
        bne.w level_8_1
    add.w addr_a, addr_a, #512

    mov.w loop, #256
    ldrsh.w zeta, [addr_zeta], #2
    level_8_2:
        butterfly_wrap 512
        subs.w loop, loop, #1
        bne.w level_8_2
    add.w addr_a, addr_a, #512

    mov.w loop, #256
    ldrsh.w zeta, [addr_zeta], #2
    level_8_3:
        butterfly_wrap 512
        subs.w loop, loop, #1
        bne.w level_8_3
    add.w addr_a, addr_a, #512

    mov.w loop, #256
    ldrsh.w zeta, [addr_zeta], #2
    level_8_4:
        butterfly_wrap 512
        subs.w loop, loop, #1
        bne.w level_8_4
    sub.w addr_a, addr_a, #3584     //back to &a[0]

//level 7
    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_1:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_1
    add.w addr_a, addr_a, #256

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_2:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_2
    add.w addr_a, addr_a, #256

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_3:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_3
    add.w addr_a, addr_a, #256

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_4:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_4
    add.w addr_a, addr_a, #256

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_5:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_5
    add.w addr_a, addr_a, #256

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_6:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_6
    add.w addr_a, addr_a, #256

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_7:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_7
    add.w addr_a, addr_a, #256

    mov.w loop, #128
    ldrsh.w zeta, [addr_zeta], #2
    level_7_8:
        butterfly_wrap 256
        subs.w loop, loop, #1
        bne.w level_7_8
    sub.w addr_a, addr_a, #3840     //back to &a[0]

//level 6
    mov.w loop, #16
    level_6:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 64
            butterfly_wrap 128
        .endr
        add.w addr_a, addr_a, #128
        subs.w loop, loop, #1
        bne.w level_6
    sub.w addr_a, addr_a, #4096

//level 5
    mov.w loop, #32
    level_5:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 32
            butterfly_wrap 64
        .endr
        add.w addr_a, addr_a, #64
        subs.w loop, loop, #1
        bne.w level_5
    sub.w addr_a, addr_a, #4096

//level 4
    mov.w loop, #64
    level_4:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 16
            butterfly_wrap 32
        .endr
        add.w addr_a, addr_a, #32
        subs.w loop, loop, #1
        bne.w level_4
    //sub.w addr_a, addr_a, #4096

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
r_mq .req r4
loop .req r5
a_r .req r12

    push {r4-r5, lr}
    bl poly_ntt_asm
    sub.w addr_a, addr_a, #4096

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769
    mov.w r_mq, #171

// mq_reduce
    mov.w loop, #2048
    mq_reduce_lp:
        ldrsh.w a_r, [addr_a]
        fqmul_asm a_r, a_r, r_mq, qinv, q, tmp
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
.unreq r_mq
.unreq loop
.unreq a_r

.size poly_ntt_mq_asm, .-poly_ntt_mq_asm
