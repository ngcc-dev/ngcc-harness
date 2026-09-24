.syntax unified
.thumb

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
    sub.w a_r, a_h, a_l                     // a_r == 2bx
    add.w a_h, a_h, a_l                     // a_h == 2a
    fqmul_asm a_l, a_r, zeta, qinv, q, tmp  // a_l == 2b
    strh.w a_h, [addr_a], #2
    strh.w a_l, [addr_a, #\offset-2]
.endm

.macro butterfly_wrap_mq offset
    ldrsh.w a_h, [addr_a]
    ldrsh.w a_l, [addr_a, #\offset]
    sub.w a_r, a_h, a_l                     // a_r == 2bx
    add.w a_h, a_h, a_l                     // a_h == 2a
    fqmul_asm a_l, a_r, zeta, qinv, q, tmp  // a_l == 2b
    fqmul_asm a_h, a_h, r_mq, qinv, q, tmp  // mongt_red ah
    fqmul_asm a_l, a_l, r_mq, qinv, q, tmp  // mongt_red al
    strh.w a_h, [addr_a], #2
    strh.w a_l, [addr_a, #\offset-2]
.endm

//void poly_intt_asm(int16_t *a, const int16_t fn[128]);
.global poly_intt_asm
.type poly_intt_asm, %function
.align 2
poly_intt_asm:

addr_a .req r0
addr_zeta .req r1
qinv .req r2
q .req r3
zeta .req r4
loop .req r5
a_h .req r6
a_l .req r7
a_r .req r8
tmp .req r9
r_mq .req r12

    push {r4-r9}

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769
    movw r_mq, #655

//level 3
    mov.w loop, #64
    level_3:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 8
            butterfly_wrap 16
        .endr
        add.w addr_a, addr_a, #16
        subs.w loop, loop, #1
        bne.w level_3
    sub.w addr_a, addr_a, #2048

//level 4
    mov.w loop, #32
    level_4:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 16
            butterfly_wrap 32
        .endr
        add.w addr_a, addr_a, #32
        subs.w loop, loop, #1
        bne.w level_4
    sub.w addr_a, addr_a, #2048

//level 5
    mov.w loop, #16
    level_5:
        ldrsh.w zeta, [addr_zeta], #2
        .rept 32
            butterfly_wrap 64
        .endr
        add.w addr_a, addr_a, #64
        subs.w loop, loop, #1
        bne.w level_5
    sub.w addr_a, addr_a, #2048

//level 6
    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_1:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_1
    add.w addr_a, addr_a, #128     //go to &a[64]

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_2:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_2
    add.w addr_a, addr_a, #128     //go to &a[128]

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_3:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_3
    add.w addr_a, addr_a, #128     //go to &a[192]

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_4:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_4
    add.w addr_a, addr_a, #128     //go to &a[256]

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_5:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_5
    add.w addr_a, addr_a, #128     //go to &a[320]

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_6:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_6
    add.w addr_a, addr_a, #128     //go to &a[384]

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_7:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_7
    add.w addr_a, addr_a, #128     //go to &a[448]

    mov.w loop, #64
    ldrsh.w zeta, [addr_zeta], #2
    level_6_8:
        butterfly_wrap 128
        subs.w loop, loop, #1
        bne.w level_6_8
    sub.w addr_a, addr_a, #1920     //back to &a[0]

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
    sub.w addr_a, addr_a, #1792     //back to &a[0]

//level 8
    movw loop, #256
    ldrsh.w zeta, [addr_zeta], #2
    level_8_1:
        butterfly_wrap 512
        subs.w loop, loop, #1
        bne.w level_8_1
    add.w addr_a, addr_a, #512

    movw loop, #256
    ldrsh.w zeta, [addr_zeta], #2
    level_8_2:
        butterfly_wrap 512
        subs.w loop, loop, #1
        bne.w level_8_2
    sub.w addr_a, addr_a, #1536     //back to &a[0]

//level 9
    movw loop, #512
    ldrsh.w zeta, [addr_zeta], #2
    level_9:
        butterfly_wrap_mq 1024
        subs.w loop, loop, #1
        bne.w level_9
    sub.w addr_a, addr_a, #1024     //back to &a[0]


    pop {r4-r9}

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
.unreq r_mq

    bx lr
.size poly_intt_asm, .-poly_intt_asm
