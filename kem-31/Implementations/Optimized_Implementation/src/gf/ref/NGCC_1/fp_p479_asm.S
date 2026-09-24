/*
 * NGCC_1 p479 field arithmetic, x86-64 BMI2/ADX.
 *
 * Saturated 8x64-bit Montgomery arithmetic.  The modulus has low limb
 * p[0] = -1, so mu = -p^{-1} mod 2^64 = 1.
 */

    .text

    .section .rodata
    .p2align 3
p479_const:
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0x94cbe0b3ffffffff
    .quad 0xe0daea16974b919e
    .quad 0x588643dadd9997fb
    .quad 0x86b19c0bf3bb9a90
    .quad 0xc23ef552f7bea022
    .quad 0x00000000588ee91a

/* p[2] plus the carry injected by the p[0] = p[1] = -1 reduction step. */
    .p2align 3
p479_p2_plus_one:
    .quad 0x94cbe0b400000000

    .p2align 3
p479_zero:
    .quad 0, 0, 0, 0, 0, 0, 0, 0
    .quad 0, 0, 0, 0, 0, 0, 0, 0

    .text

    .macro FP479_COND_ADD_P l0, l1, l2, l3, l4, l5, l6, l7, bit, tmp, trash
    movq \bit, \tmp
    negq \tmp
    addq \tmp, \l0
    adcq \tmp, \l1
    mulxq p479_const+16(%rip), \tmp, \trash
    adcq \tmp, \l2
    mulxq p479_const+24(%rip), \tmp, \trash
    adcq \tmp, \l3
    mulxq p479_const+32(%rip), \tmp, \trash
    adcq \tmp, \l4
    mulxq p479_const+40(%rip), \tmp, \trash
    adcq \tmp, \l5
    mulxq p479_const+48(%rip), \tmp, \trash
    adcq \tmp, \l6
    mulxq p479_const+56(%rip), \tmp, \trash
    adcq \tmp, \l7
    .endm

/* c = a + b mod p */
    .globl fp479_add_asm
    .type fp479_add_asm, @function
    .p2align 4
fp479_add_asm:
    pushq %rbx
    movq %rdi, %rbx

    movq 0(%rsi), %r8
    addq 0(%rdx), %r8
    movq 8(%rsi), %r9
    adcq 8(%rdx), %r9
    movq 16(%rsi), %r10
    adcq 16(%rdx), %r10
    movq 24(%rsi), %r11
    adcq 24(%rdx), %r11
    movq 32(%rsi), %rax
    adcq 32(%rdx), %rax
    movq 40(%rsi), %rcx
    adcq 40(%rdx), %rcx
    movq 48(%rsi), %rdi
    adcq 48(%rdx), %rdi
    movq 56(%rsi), %rsi
    adcq 56(%rdx), %rsi

    movq %r8, 0(%rbx)
    movq %r9, 8(%rbx)
    movq %r10, 16(%rbx)
    movq %r11, 24(%rbx)
    movq %rax, 32(%rbx)
    movq %rcx, 40(%rbx)
    movq %rdi, 48(%rbx)
    movq %rsi, 56(%rbx)

    leaq p479_const(%rip), %rdx
    subq 0(%rdx), %r8
    sbbq 8(%rdx), %r9
    sbbq 16(%rdx), %r10
    sbbq 24(%rdx), %r11
    sbbq 32(%rdx), %rax
    sbbq 40(%rdx), %rcx
    sbbq 48(%rdx), %rdi
    sbbq 56(%rdx), %rsi

    cmovcq 0(%rbx), %r8
    cmovcq 8(%rbx), %r9
    cmovcq 16(%rbx), %r10
    cmovcq 24(%rbx), %r11
    cmovcq 32(%rbx), %rax
    cmovcq 40(%rbx), %rcx
    cmovcq 48(%rbx), %rdi
    cmovcq 56(%rbx), %rsi

    movq %r8, 0(%rbx)
    movq %r9, 8(%rbx)
    movq %r10, 16(%rbx)
    movq %r11, 24(%rbx)
    movq %rax, 32(%rbx)
    movq %rcx, 40(%rbx)
    movq %rdi, 48(%rbx)
    movq %rsi, 56(%rbx)

    popq %rbx
    ret

/* c = a - b mod p */
    .globl fp479_sub_asm
    .type fp479_sub_asm, @function
    .p2align 4
fp479_sub_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    movq %rdi, %rbx

    movq 0(%rsi), %r8
    subq 0(%rdx), %r8
    movq 8(%rsi), %r9
    sbbq 8(%rdx), %r9
    movq 16(%rsi), %r10
    sbbq 16(%rdx), %r10
    movq 24(%rsi), %r11
    sbbq 24(%rdx), %r11
    movq 32(%rsi), %rax
    sbbq 32(%rdx), %rax
    movq 40(%rsi), %rcx
    sbbq 40(%rdx), %rcx
    movq 48(%rsi), %r12
    sbbq 48(%rdx), %r12
    movq 56(%rsi), %rsi
    sbbq 56(%rdx), %rsi

    sbbq %rdx, %rdx
    andq $1, %rdx
    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %r12, %rsi, %rdx, %rbp, %rdi

    movq %r8, 0(%rbx)
    movq %r9, 8(%rbx)
    movq %r10, 16(%rbx)
    movq %r11, 24(%rbx)
    movq %rax, 32(%rbx)
    movq %rcx, 40(%rbx)
    movq %r12, 48(%rbx)
    movq %rsi, 56(%rbx)

    popq %r12
    popq %rbp
    popq %rbx
    ret

    .macro FP479_SUB_COMPONENT off
    movq \off(%r14), %r8
    subq \off(%r15), %r8
    movq \off+8(%r14), %r9
    sbbq \off+8(%r15), %r9
    movq \off+16(%r14), %r10
    sbbq \off+16(%r15), %r10
    movq \off+24(%r14), %r11
    sbbq \off+24(%r15), %r11
    movq \off+32(%r14), %rax
    sbbq \off+32(%r15), %rax
    movq \off+40(%r14), %rcx
    sbbq \off+40(%r15), %rcx
    movq \off+48(%r14), %r12
    sbbq \off+48(%r15), %r12
    movq \off+56(%r14), %rsi
    sbbq \off+56(%r15), %rsi

    sbbq %rdx, %rdx
    andq $1, %rdx
    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %r12, %rsi, %rdx, %rbp, %rdi

    movq %r8, \off(%r13)
    movq %r9, \off+8(%r13)
    movq %r10, \off+16(%r13)
    movq %r11, \off+24(%r13)
    movq %rax, \off+32(%r13)
    movq %rcx, \off+40(%r13)
    movq %r12, \off+48(%r13)
    movq %rsi, \off+56(%r13)
    .endm

    .macro FP479_ADD_FIELD out, outoff, a, aoff, b, boff
    movq \aoff(\a), %r8
    addq \boff(\b), %r8
    movq \aoff+8(\a), %r9
    adcq \boff+8(\b), %r9
    movq \aoff+16(\a), %r10
    adcq \boff+16(\b), %r10
    movq \aoff+24(\a), %r11
    adcq \boff+24(\b), %r11
    movq \aoff+32(\a), %rax
    adcq \boff+32(\b), %rax
    movq \aoff+40(\a), %rcx
    adcq \boff+40(\b), %rcx
    movq \aoff+48(\a), %rdi
    adcq \boff+48(\b), %rdi
    movq \aoff+56(\a), %rsi
    adcq \boff+56(\b), %rsi

    subq p479_const+0(%rip), %r8
    sbbq p479_const+8(%rip), %r9
    sbbq p479_const+16(%rip), %r10
    sbbq p479_const+24(%rip), %r11
    sbbq p479_const+32(%rip), %rax
    sbbq p479_const+40(%rip), %rcx
    sbbq p479_const+48(%rip), %rdi
    sbbq p479_const+56(%rip), %rsi

    sbbq %rdx, %rdx
    andq $1, %rdx
    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %rdi, %rsi, %rdx, %rbp, %r12

    movq %r8, \outoff(\out)
    movq %r9, \outoff+8(\out)
    movq %r10, \outoff+16(\out)
    movq %r11, \outoff+24(\out)
    movq %rax, \outoff+32(\out)
    movq %rcx, \outoff+40(\out)
    movq %rdi, \outoff+48(\out)
    movq %rsi, \outoff+56(\out)
    .endm

    .macro FP479_ADD_FIELD_LAZY out, outoff, a, aoff, b, boff
    movq \aoff(\a), %r8
    addq \boff(\b), %r8
    movq \aoff+8(\a), %r9
    adcq \boff+8(\b), %r9
    movq \aoff+16(\a), %r10
    adcq \boff+16(\b), %r10
    movq \aoff+24(\a), %r11
    adcq \boff+24(\b), %r11
    movq \aoff+32(\a), %rax
    adcq \boff+32(\b), %rax
    movq \aoff+40(\a), %rcx
    adcq \boff+40(\b), %rcx
    movq \aoff+48(\a), %rdi
    adcq \boff+48(\b), %rdi
    movq \aoff+56(\a), %rsi
    adcq \boff+56(\b), %rsi

    movq %r8, \outoff(\out)
    movq %r9, \outoff+8(\out)
    movq %r10, \outoff+16(\out)
    movq %r11, \outoff+24(\out)
    movq %rax, \outoff+32(\out)
    movq %rcx, \outoff+40(\out)
    movq %rdi, \outoff+48(\out)
    movq %rsi, \outoff+56(\out)
    .endm

    .macro FP479_ADD_FIELD_FP2 off
    movq \off(%rsi), %r8
    addq \off(%rdx), %r8
    movq \off+8(%rsi), %r9
    adcq \off+8(%rdx), %r9
    movq \off+16(%rsi), %r10
    adcq \off+16(%rdx), %r10
    movq \off+24(%rsi), %r11
    adcq \off+24(%rdx), %r11
    movq \off+32(%rsi), %rax
    adcq \off+32(%rdx), %rax
    movq \off+40(%rsi), %rcx
    adcq \off+40(%rdx), %rcx
    movq \off+48(%rsi), %r12
    adcq \off+48(%rdx), %r12
    movq \off+56(%rsi), %r13
    adcq \off+56(%rdx), %r13

    subq p479_const+0(%rip), %r8
    sbbq p479_const+8(%rip), %r9
    sbbq p479_const+16(%rip), %r10
    sbbq p479_const+24(%rip), %r11
    sbbq p479_const+32(%rip), %rax
    sbbq p479_const+40(%rip), %rcx
    sbbq p479_const+48(%rip), %r12
    sbbq p479_const+56(%rip), %r13

    sbbq %rdx, %rdx
    andq $1, %rdx
    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %r12, %r13, %rdx, %rbp, %r14

    movq %r8, \off(%rdi)
    movq %r9, \off+8(%rdi)
    movq %r10, \off+16(%rdi)
    movq %r11, \off+24(%rdi)
    movq %rax, \off+32(%rdi)
    movq %rcx, \off+40(%rdi)
    movq %r12, \off+48(%rdi)
    movq %r13, \off+56(%rdi)
    .endm

/* out = a + p - b, a lazy representative in [0, 2p) for mul inputs. */
    .macro FP479_SUB_FIELD_LAZY out, outoff, a, aoff, b, boff
    movq \aoff(\a), %r8
    addq p479_const+0(%rip), %r8
    movq \aoff+8(\a), %r9
    adcq p479_const+8(%rip), %r9
    movq \aoff+16(\a), %r10
    adcq p479_const+16(%rip), %r10
    movq \aoff+24(\a), %r11
    adcq p479_const+24(%rip), %r11
    movq \aoff+32(\a), %rax
    adcq p479_const+32(%rip), %rax
    movq \aoff+40(\a), %rcx
    adcq p479_const+40(%rip), %rcx
    movq \aoff+48(\a), %r12
    adcq p479_const+48(%rip), %r12
    movq \aoff+56(\a), %rsi
    adcq p479_const+56(%rip), %rsi

    subq \boff(\b), %r8
    sbbq \boff+8(\b), %r9
    sbbq \boff+16(\b), %r10
    sbbq \boff+24(\b), %r11
    sbbq \boff+32(\b), %rax
    sbbq \boff+40(\b), %rcx
    sbbq \boff+48(\b), %r12
    sbbq \boff+56(\b), %rsi

    movq %r8, \outoff(\out)
    movq %r9, \outoff+8(\out)
    movq %r10, \outoff+16(\out)
    movq %r11, \outoff+24(\out)
    movq %rax, \outoff+32(\out)
    movq %rcx, \outoff+40(\out)
    movq %r12, \outoff+48(\out)
    movq %rsi, \outoff+56(\out)
    .endm

    .macro FP479_SUB_FIELD out, outoff, a, aoff, b, boff
    movq \aoff(\a), %r8
    subq \boff(\b), %r8
    movq \aoff+8(\a), %r9
    sbbq \boff+8(\b), %r9
    movq \aoff+16(\a), %r10
    sbbq \boff+16(\b), %r10
    movq \aoff+24(\a), %r11
    sbbq \boff+24(\b), %r11
    movq \aoff+32(\a), %rax
    sbbq \boff+32(\b), %rax
    movq \aoff+40(\a), %rcx
    sbbq \boff+40(\b), %rcx
    movq \aoff+48(\a), %r12
    sbbq \boff+48(\b), %r12
    movq \aoff+56(\a), %rsi
    sbbq \boff+56(\b), %rsi

    sbbq %rdx, %rdx
    andq $1, %rdx
    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %r12, %rsi, %rdx, %rbp, %rdi

    movq %r8, \outoff(\out)
    movq %r9, \outoff+8(\out)
    movq %r10, \outoff+16(\out)
    movq %r11, \outoff+24(\out)
    movq %rax, \outoff+32(\out)
    movq %rcx, \outoff+40(\out)
    movq %r12, \outoff+48(\out)
    movq %rsi, \outoff+56(\out)
    .endm

    .macro FP479_SUB_SUB_FIELD out, outoff, a, aoff, b, boff, c, coff
    movq \aoff(\a), %r8
    subq \boff(\b), %r8
    movq \aoff+8(\a), %r9
    sbbq \boff+8(\b), %r9
    movq \aoff+16(\a), %r10
    sbbq \boff+16(\b), %r10
    movq \aoff+24(\a), %r11
    sbbq \boff+24(\b), %r11
    movq \aoff+32(\a), %rax
    sbbq \boff+32(\b), %rax
    movq \aoff+40(\a), %rcx
    sbbq \boff+40(\b), %rcx
    movq \aoff+48(\a), %r12
    sbbq \boff+48(\b), %r12
    movq \aoff+56(\a), %rsi
    sbbq \boff+56(\b), %rsi

    sbbq %rdx, %rdx
    andq $1, %rdx
    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %r12, %rsi, %rdx, %rbp, %rdi

    subq \coff(\c), %r8
    sbbq \coff+8(\c), %r9
    sbbq \coff+16(\c), %r10
    sbbq \coff+24(\c), %r11
    sbbq \coff+32(\c), %rax
    sbbq \coff+40(\c), %rcx
    sbbq \coff+48(\c), %r12
    sbbq \coff+56(\c), %rsi

    sbbq %rdx, %rdx
    andq $1, %rdx
    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %r12, %rsi, %rdx, %rbp, %rdi

    movq %r8, \outoff(\out)
    movq %r9, \outoff+8(\out)
    movq %r10, \outoff+16(\out)
    movq %r11, \outoff+24(\out)
    movq %rax, \outoff+32(\out)
    movq %rcx, \outoff+40(\out)
    movq %r12, \outoff+48(\out)
    movq %rsi, \outoff+56(\out)
    .endm

    .macro FP479_DOUBLE_FIELD out, outoff
    movq \outoff(\out), %r8
    addq %r8, %r8
    movq \outoff+8(\out), %r9
    adcq %r9, %r9
    movq \outoff+16(\out), %r10
    adcq %r10, %r10
    movq \outoff+24(\out), %r11
    adcq %r11, %r11
    movq \outoff+32(\out), %rax
    adcq %rax, %rax
    movq \outoff+40(\out), %rcx
    adcq %rcx, %rcx
    movq \outoff+48(\out), %r12
    adcq %r12, %r12
    movq \outoff+56(\out), %rsi
    adcq %rsi, %rsi

    subq p479_const+0(%rip), %r8
    sbbq p479_const+8(%rip), %r9
    sbbq p479_const+16(%rip), %r10
    sbbq p479_const+24(%rip), %r11
    sbbq p479_const+32(%rip), %rax
    sbbq p479_const+40(%rip), %rcx
    sbbq p479_const+48(%rip), %r12
    sbbq p479_const+56(%rip), %rsi

    sbbq %rdx, %rdx
    andq $1, %rdx
    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %r12, %rsi, %rdx, %rbp, %rdi

    movq %r8, \outoff(\out)
    movq %r9, \outoff+8(\out)
    movq %r10, \outoff+16(\out)
    movq %r11, \outoff+24(\out)
    movq %rax, \outoff+32(\out)
    movq %rcx, \outoff+40(\out)
    movq %r12, \outoff+48(\out)
    movq %rsi, \outoff+56(\out)
    .endm

/* x = y^2 in GF(p^2), with contiguous fp2_t layout. */
    .globl fp479_fp2_sqr_asm
    .type fp479_fp2_sqr_asm, @function
    .p2align 4
fp479_fp2_sqr_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $160, %rsp

    movq %rdi, %rbx
    movq %rsi, %r14
    movq %rbx, 128(%rsp)
    movq %r14, 136(%rsp)

    FP479_ADD_FIELD_LAZY %rsp, 0, %r14, 0, %r14, 64
    FP479_SUB_FIELD_LAZY %rsp, 64, %r14, 0, %r14, 64

    movq 128(%rsp), %rbx
    movq 136(%rsp), %r14
    leaq 64(%rbx), %rdi
    movq %r14, %rsi
    leaq 64(%r14), %rdx
    call fp479_mul_nosave_asm

    movq 128(%rsp), %rbx
    FP479_DOUBLE_FIELD %rbx, 64

    movq 128(%rsp), %rbx
    movq %rbx, %rdi
    movq %rsp, %rsi
    leaq 64(%rsp), %rdx
    call fp479_mul_nosave_asm

    addq $160, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

/* x = y + z in GF(p^2), with contiguous fp2_t layout. */
    .globl fp479_fp2_add_asm
    .type fp479_fp2_add_asm, @function
    .p2align 4
fp479_fp2_add_asm:
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rdx, %r15
    movq %r15, %rdx
    FP479_ADD_FIELD_FP2 0
    movq %r15, %rdx
    FP479_ADD_FIELD_FP2 64

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    ret

/* x = -y in GF(p^2), with contiguous fp2_t layout. */
    .globl fp479_fp2_neg_asm
    .type fp479_fp2_neg_asm, @function
    .p2align 4
fp479_fp2_neg_asm:
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rdi, %r13
    leaq p479_zero(%rip), %r14
    movq %rsi, %r15

    FP479_SUB_COMPONENT 0
    FP479_SUB_COMPONENT 64

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    ret

/* x = y * z in GF(p^2), with contiguous fp2_t layout. */
    .globl fp479_fp2_mul_asm
    .type fp479_fp2_mul_asm, @function
    .p2align 4
fp479_fp2_mul_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $168, %rsp

    movq %rdi, %rbx
    movq %rsi, %r14
    movq %rdx, %r15
    movq %rbx, 128(%rsp)
    movq %r14, 136(%rsp)
    movq %r15, 144(%rsp)

    FP479_ADD_FIELD_LAZY %rsp, 0, %r14, 0, %r14, 64
    FP479_ADD_FIELD_LAZY %rsp, 64, %r15, 0, %r15, 64

    movq %rsp, %rdi
    movq %rsp, %rsi
    leaq 64(%rsp), %rdx
    call fp479_mul_nosave_asm

    leaq 64(%rsp), %rdi
    movq 136(%rsp), %r14
    movq 144(%rsp), %r15
    leaq 64(%r14), %rsi
    leaq 64(%r15), %rdx
    call fp479_mul_nosave_asm

    movq 128(%rsp), %rbx
    movq 136(%rsp), %r14
    movq 144(%rsp), %r15
    movq %rbx, %rdi
    movq %r14, %rsi
    movq %r15, %rdx
    call fp479_mul_nosave_asm

    movq 128(%rsp), %rbx
    FP479_SUB_SUB_FIELD %rbx, 64, %rsp, 0, %rsp, 64, %rbx, 0
    FP479_SUB_FIELD %rbx, 0, %rbx, 0, %rsp, 64

    addq $168, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

/* x = y - z in GF(p^2), with contiguous fp2_t layout. */
    .globl fp479_fp2_sub_asm
    .type fp479_fp2_sub_asm, @function
    .p2align 4
fp479_fp2_sub_asm:
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rdi, %r13
    movq %rsi, %r14
    movq %rdx, %r15

    FP479_SUB_COMPONENT 0
    FP479_SUB_COMPONENT 64

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    ret

/* c = a * b * R^-1 mod p */
    .globl fp479_mul_asm
    .type fp479_mul_asm, @function
    .p2align 4
fp479_mul_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rdi, -8(%rsp)

    movq %rdx, %rbx
    xorl %r8d, %r8d
    xorl %r9d, %r9d
    xorl %r10d, %r10d
    xorl %r11d, %r11d
    xorl %r12d, %r12d
    xorl %r13d, %r13d
    xorl %r14d, %r14d
    xorl %r15d, %r15d
    xorl %edi, %edi

.macro CIOS_MUL_PHASE off
    movq \off(%rsi), %rdx
    xorl %eax, %eax
    mulxq 0(%rbx), %rax, %rcx
    adcxq %r8, %rax
    movq %rax, %r8
    mulxq 8(%rbx), %rax, %rbp
    adcxq %r9, %rax
    adoxq %rcx, %rax
    movq %rax, %r9
    mulxq 16(%rbx), %rax, %rcx
    adcxq %r10, %rax
    adoxq %rbp, %rax
    movq %rax, %r10
    mulxq 24(%rbx), %rax, %rbp
    adcxq %r11, %rax
    adoxq %rcx, %rax
    movq %rax, %r11
    mulxq 32(%rbx), %rax, %rcx
    adcxq %r12, %rax
    adoxq %rbp, %rax
    movq %rax, %r12
    mulxq 40(%rbx), %rax, %rbp
    adcxq %r13, %rax
    adoxq %rcx, %rax
    movq %rax, %r13
    mulxq 48(%rbx), %rax, %rcx
    adcxq %r14, %rax
    adoxq %rbp, %rax
    movq %rax, %r14
    mulxq 56(%rbx), %rax, %rbp
    adcxq %r15, %rax
    adoxq %rcx, %rax
    movq %rax, %r15
    movl $0, %ecx
    adcxq %rcx, %rbp
    adoxq %rcx, %rbp
    addq %rbp, %rdi
.endm

.macro CIOS_RED_PHASE
    /*
     * p[0] = p[1] = -1.  With m = T0, the p[0] term cancels T0 and
     * the combined p[0]/p[1] carry leaves T1 unchanged while injecting
     * m into the p[2] column.  Start the MULX chain at p[2].
     */
    movq %r8, %rdx
    xorl %eax, %eax
    mulxq p479_p2_plus_one(%rip), %rax, %rbp
    adcxq %r10, %rax
    movq %rax, %r10
    mulxq p479_const+24(%rip), %rax, %rcx
    adcxq %r11, %rax
    adoxq %rbp, %rax
    movq %rax, %r11
    mulxq p479_const+32(%rip), %rax, %rbp
    adcxq %r12, %rax
    adoxq %rcx, %rax
    movq %rax, %r12
    mulxq p479_const+40(%rip), %rax, %rcx
    adcxq %r13, %rax
    adoxq %rbp, %rax
    movq %rax, %r13
    mulxq p479_const+48(%rip), %rax, %rbp
    adcxq %r14, %rax
    adoxq %rcx, %rax
    movq %rax, %r14
    mulxq p479_const+56(%rip), %rax, %rcx
    adcxq %r15, %rax
    adoxq %rbp, %rax
    movq %rax, %r15
    movl $0, %ebp
    adcxq %rbp, %rcx
    adoxq %rbp, %rcx
    addq %rcx, %rdi
.endm

.macro CIOS_SHIFT
    movq %r9, %r8
    movq %r10, %r9
    movq %r11, %r10
    movq %r12, %r11
    movq %r13, %r12
    movq %r14, %r13
    movq %r15, %r14
    movq %rdi, %r15
    xorl %edi, %edi
.endm

    CIOS_MUL_PHASE 0
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 8
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 16
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 24
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 32
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 40
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 48
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 56
    CIOS_RED_PHASE
    CIOS_SHIFT

    subq p479_const+0(%rip), %r8
    sbbq p479_const+8(%rip), %r9
    sbbq p479_const+16(%rip), %r10
    sbbq p479_const+24(%rip), %r11
    sbbq p479_const+32(%rip), %r12
    sbbq p479_const+40(%rip), %r13
    sbbq p479_const+48(%rip), %r14
    sbbq p479_const+56(%rip), %r15

    sbbq %rax, %rax
    movq p479_const+16(%rip), %rdx
    andq %rax, %rdx
    movq p479_const+24(%rip), %rcx
    andq %rax, %rcx
    movq p479_const+32(%rip), %rbx
    andq %rax, %rbx
    movq p479_const+40(%rip), %rbp
    andq %rax, %rbp
    movq p479_const+48(%rip), %rsi
    andq %rax, %rsi
    movq p479_const+56(%rip), %rdi
    andq %rax, %rdi

    addq %rax, %r8
    adcq %rax, %r9
    adcq %rdx, %r10
    adcq %rcx, %r11
    adcq %rbx, %r12
    adcq %rbp, %r13
    adcq %rsi, %r14
    adcq %rdi, %r15

    movq -8(%rsp), %rax
    movq %r8, 0(%rax)
    movq %r9, 8(%rax)
    movq %r10, 16(%rax)
    movq %r11, 24(%rax)
    movq %r12, 32(%rax)
    movq %r13, 40(%rax)
    movq %r14, 48(%rax)
    movq %r15, 56(%rax)

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

/* Internal multiply variant for callers that already preserve callee-saved regs. */
    .type fp479_mul_nosave_asm, @function
    .p2align 4
fp479_mul_nosave_asm:
    movq %rdi, -8(%rsp)

    movq %rdx, %rbx
    xorl %r8d, %r8d
    xorl %r9d, %r9d
    xorl %r10d, %r10d
    xorl %r11d, %r11d
    xorl %r12d, %r12d
    xorl %r13d, %r13d
    xorl %r14d, %r14d
    xorl %r15d, %r15d
    xorl %edi, %edi

    CIOS_MUL_PHASE 0
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 8
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 16
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 24
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 32
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 40
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 48
    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_MUL_PHASE 56
    CIOS_RED_PHASE
    CIOS_SHIFT

    subq p479_const+0(%rip), %r8
    sbbq p479_const+8(%rip), %r9
    sbbq p479_const+16(%rip), %r10
    sbbq p479_const+24(%rip), %r11
    sbbq p479_const+32(%rip), %r12
    sbbq p479_const+40(%rip), %r13
    sbbq p479_const+48(%rip), %r14
    sbbq p479_const+56(%rip), %r15

    sbbq %rax, %rax
    movq p479_const+16(%rip), %rdx
    andq %rax, %rdx
    movq p479_const+24(%rip), %rcx
    andq %rax, %rcx
    movq p479_const+32(%rip), %rbx
    andq %rax, %rbx
    movq p479_const+40(%rip), %rbp
    andq %rax, %rbp
    movq p479_const+48(%rip), %rsi
    andq %rax, %rsi
    movq p479_const+56(%rip), %rdi
    andq %rax, %rdi

    addq %rax, %r8
    adcq %rax, %r9
    adcq %rdx, %r10
    adcq %rcx, %r11
    adcq %rbx, %r12
    adcq %rbp, %r13
    adcq %rsi, %r14
    adcq %rdi, %r15

    movq -8(%rsp), %rax
    movq %r8, 0(%rax)
    movq %r9, 8(%rax)
    movq %r10, 16(%rax)
    movq %r11, 24(%rax)
    movq %r12, 32(%rax)
    movq %r13, 40(%rax)
    movq %r14, 48(%rax)
    movq %r15, 56(%rax)

    ret

/* c = a * R^-1 mod p */
    .globl fp479_redc_asm
    .type fp479_redc_asm, @function
    .p2align 4
fp479_redc_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rdi, -8(%rsp)

    movq 0(%rsi), %r8
    movq 8(%rsi), %r9
    movq 16(%rsi), %r10
    movq 24(%rsi), %r11
    movq 32(%rsi), %r12
    movq 40(%rsi), %r13
    movq 48(%rsi), %r14
    movq 56(%rsi), %r15
    xorl %edi, %edi

    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_RED_PHASE
    CIOS_SHIFT

    CIOS_RED_PHASE
    CIOS_SHIFT

    subq p479_const+0(%rip), %r8
    sbbq p479_const+8(%rip), %r9
    sbbq p479_const+16(%rip), %r10
    sbbq p479_const+24(%rip), %r11
    sbbq p479_const+32(%rip), %r12
    sbbq p479_const+40(%rip), %r13
    sbbq p479_const+48(%rip), %r14
    sbbq p479_const+56(%rip), %r15

    sbbq %rax, %rax
    movq p479_const+16(%rip), %rdx
    andq %rax, %rdx
    movq p479_const+24(%rip), %rcx
    andq %rax, %rcx
    movq p479_const+32(%rip), %rbx
    andq %rax, %rbx
    movq p479_const+40(%rip), %rbp
    andq %rax, %rbp
    movq p479_const+48(%rip), %rsi
    andq %rax, %rsi
    movq p479_const+56(%rip), %rdi
    andq %rax, %rdi

    addq %rax, %r8
    adcq %rax, %r9
    adcq %rdx, %r10
    adcq %rcx, %r11
    adcq %rbx, %r12
    adcq %rbp, %r13
    adcq %rsi, %r14
    adcq %rdi, %r15

    movq -8(%rsp), %rax
    movq %r8, 0(%rax)
    movq %r9, 8(%rax)
    movq %r10, 16(%rax)
    movq %r11, 24(%rax)
    movq %r12, 32(%rax)
    movq %r13, 40(%rax)
    movq %r14, 48(%rax)
    movq %r15, 56(%rax)

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

    .macro FP479_ADC_PROP_FROM_2
    adcq $0, %r10
    adcq $0, %r11
    adcq $0, %r12
    adcq $0, %r13
    adcq $0, %r14
    adcq $0, %r15
    adcq $0, %rdi
    .endm

    .macro FP479_ADC_PROP_FROM_3
    adcq $0, %r11
    adcq $0, %r12
    adcq $0, %r13
    adcq $0, %r14
    adcq $0, %r15
    adcq $0, %rdi
    .endm

    .macro FP479_ADC_PROP_FROM_4
    adcq $0, %r12
    adcq $0, %r13
    adcq $0, %r14
    adcq $0, %r15
    adcq $0, %rdi
    .endm

    .macro FP479_ADC_PROP_FROM_5
    adcq $0, %r13
    adcq $0, %r14
    adcq $0, %r15
    adcq $0, %rdi
    .endm

    .macro FP479_ADC_PROP_FROM_6
    adcq $0, %r14
    adcq $0, %r15
    adcq $0, %rdi
    .endm

    .macro FP479_ADC_PROP_FROM_7
    adcq $0, %r15
    adcq $0, %rdi
    .endm

    .macro FP479_ADC_PROP_FROM_8
    adcq $0, %rdi
    .endm

    .macro FP479_SQR_DIAG_0 off
    mulxq \off(%rsi), %rax, %rcx
    addq %rax, %r8
    adcq %rcx, %r9
    FP479_ADC_PROP_FROM_2
    .endm

    .macro FP479_SQR_DIAG_1 off
    mulxq \off(%rsi), %rax, %rcx
    addq %rax, %r9
    adcq %rcx, %r10
    FP479_ADC_PROP_FROM_3
    .endm

    .macro FP479_SQR_DIAG_2 off
    mulxq \off(%rsi), %rax, %rcx
    addq %rax, %r10
    adcq %rcx, %r11
    FP479_ADC_PROP_FROM_4
    .endm

    .macro FP479_SQR_DIAG_3 off
    mulxq \off(%rsi), %rax, %rcx
    addq %rax, %r11
    adcq %rcx, %r12
    FP479_ADC_PROP_FROM_5
    .endm

    .macro FP479_SQR_DIAG_4 off
    mulxq \off(%rsi), %rax, %rcx
    addq %rax, %r12
    adcq %rcx, %r13
    FP479_ADC_PROP_FROM_6
    .endm

    .macro FP479_SQR_DIAG_5 off
    mulxq \off(%rsi), %rax, %rcx
    addq %rax, %r13
    adcq %rcx, %r14
    FP479_ADC_PROP_FROM_7
    .endm

    .macro FP479_SQR_DIAG_6 off
    mulxq \off(%rsi), %rax, %rcx
    addq %rax, %r14
    adcq %rcx, %r15
    FP479_ADC_PROP_FROM_8
    .endm

    .macro FP479_SQR_DIAG_7 off
    mulxq \off(%rsi), %rax, %rcx
    addq %rax, %r15
    adcq %rcx, %rdi
    .endm

    .macro FP479_SQR_DIAG_1_CROSS_2 diagoff, crossoff
    mulxq \diagoff(%rsi), %rax, %rcx
    addq %rax, %r9
    adcq %rcx, %r10
    movl $0, %ebx
    adcq %rbx, %rbx
    mulxq \crossoff(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rbx, %rcx
    adcq $0, %rbp
    addq %rax, %r10
    adcq %rcx, %r11
    adcq %rbp, %r12
    FP479_ADC_PROP_FROM_5
    .endm

    .macro FP479_SQR_DIAG_2_CROSS_3 diagoff, crossoff
    mulxq \diagoff(%rsi), %rax, %rcx
    addq %rax, %r10
    adcq %rcx, %r11
    movl $0, %ebx
    adcq %rbx, %rbx
    mulxq \crossoff(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rbx, %rcx
    adcq $0, %rbp
    addq %rax, %r11
    adcq %rcx, %r12
    adcq %rbp, %r13
    FP479_ADC_PROP_FROM_6
    .endm

    .macro FP479_SQR_CROSS_1 off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r9
    adcq %rcx, %r10
    adcq %rbp, %r11
    FP479_ADC_PROP_FROM_4
    .endm

    .macro FP479_SQR_CROSS_2 off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r10
    adcq %rcx, %r11
    adcq %rbp, %r12
    FP479_ADC_PROP_FROM_5
    .endm

    .macro FP479_SQR_CROSS_3 off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r11
    adcq %rcx, %r12
    adcq %rbp, %r13
    FP479_ADC_PROP_FROM_6
    .endm

    .macro FP479_SQR_CROSS_4 off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r12
    adcq %rcx, %r13
    adcq %rbp, %r14
    FP479_ADC_PROP_FROM_7
    .endm

    .macro FP479_SQR_CROSS_5 off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r13
    adcq %rcx, %r14
    adcq %rbp, %r15
    FP479_ADC_PROP_FROM_8
    .endm

    .macro FP479_SQR_CROSS_6 off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r14
    adcq %rcx, %r15
    adcq %rbp, %rdi
    .endm

    .macro FP479_SQR_CROSS_7 off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    addq %rax, %r15
    adcq %rcx, %rdi
    .endm

    .macro FP479_SQR_DIAG_0_NP off
    mulxq \off(%rsi), %r8, %r9
    .endm

    .macro FP479_SQR_CROSS_1_NP off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r9
    adcq %rcx, %r10
    adcq %rbp, %r11
    .endm

    .macro FP479_SQR_CROSS_2_NP off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r10
    adcq %rcx, %r11
    adcq %rbp, %r12
    .endm

    .macro FP479_SQR_CROSS_3_NP off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r11
    adcq %rcx, %r12
    adcq %rbp, %r13
    .endm

    .macro FP479_SQR_CROSS_4_NP off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r12
    adcq %rcx, %r13
    adcq %rbp, %r14
    .endm

    .macro FP479_SQR_CROSS_5_NP off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r13
    adcq %rcx, %r14
    adcq %rbp, %r15
    .endm

    .macro FP479_SQR_CROSS_6_NP off
    mulxq \off(%rsi), %rax, %rcx
    xorl %ebp, %ebp
    adoxq %rax, %rax
    adoxq %rcx, %rcx
    adoxq %rbp, %rbp
    addq %rax, %r14
    adcq %rcx, %r15
    adcq %rbp, %rdi
    .endm

    .macro FP479_SQR_PHASE_0
    movq 0(%rsi), %rdx
    FP479_SQR_DIAG_0_NP 0
    FP479_SQR_CROSS_1_NP 8
    FP479_SQR_CROSS_2_NP 16
    FP479_SQR_CROSS_3_NP 24
    FP479_SQR_CROSS_4_NP 32
    FP479_SQR_CROSS_5_NP 40
    FP479_SQR_CROSS_6_NP 48
    FP479_SQR_CROSS_7 56
    .endm

    .macro FP479_SQR_PHASE_1
    movq 8(%rsi), %rdx
    FP479_SQR_DIAG_1_CROSS_2 8, 16
    FP479_SQR_CROSS_3 24
    FP479_SQR_CROSS_4 32
    FP479_SQR_CROSS_5 40
    FP479_SQR_CROSS_6 48
    FP479_SQR_CROSS_7 56
    .endm

    .macro FP479_SQR_PHASE_2
    movq 16(%rsi), %rdx
    FP479_SQR_DIAG_2_CROSS_3 16, 24
    FP479_SQR_CROSS_4 32
    FP479_SQR_CROSS_5 40
    FP479_SQR_CROSS_6 48
    FP479_SQR_CROSS_7 56
    .endm

    .macro FP479_SQR_PHASE_3
    movq 24(%rsi), %rdx
    FP479_SQR_DIAG_3 24
    FP479_SQR_CROSS_4 32
    FP479_SQR_CROSS_5 40
    FP479_SQR_CROSS_6 48
    FP479_SQR_CROSS_7 56
    .endm

    .macro FP479_SQR_PHASE_4
    movq 32(%rsi), %rdx
    FP479_SQR_DIAG_4 32
    FP479_SQR_CROSS_5 40
    FP479_SQR_CROSS_6 48
    FP479_SQR_CROSS_7 56
    .endm

    .macro FP479_SQR_PHASE_5
    movq 40(%rsi), %rdx
    FP479_SQR_DIAG_5 40
    FP479_SQR_CROSS_6 48
    FP479_SQR_CROSS_7 56
    .endm

    .macro FP479_SQR_PHASE_6
    movq 48(%rsi), %rdx
    FP479_SQR_DIAG_6 48
    FP479_SQR_CROSS_7 56
    .endm

    .macro FP479_SQR_PHASE_7
    movq 56(%rsi), %rdx
    FP479_SQR_DIAG_7 56
    .endm

    .globl fp479_sqr_asm
    .type fp479_sqr_asm, @function
    .p2align 4
fp479_sqr_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rdi, -8(%rsp)

    xorl %r8d, %r8d
    xorl %r9d, %r9d
    xorl %r10d, %r10d
    xorl %r11d, %r11d
    xorl %r12d, %r12d
    xorl %r13d, %r13d
    xorl %r14d, %r14d
    xorl %r15d, %r15d
    xorl %edi, %edi

    FP479_SQR_PHASE_0
    CIOS_RED_PHASE
    CIOS_SHIFT

    FP479_SQR_PHASE_1
    CIOS_RED_PHASE
    CIOS_SHIFT

    FP479_SQR_PHASE_2
    CIOS_RED_PHASE
    CIOS_SHIFT

    FP479_SQR_PHASE_3
    CIOS_RED_PHASE
    CIOS_SHIFT

    FP479_SQR_PHASE_4
    CIOS_RED_PHASE
    CIOS_SHIFT

    FP479_SQR_PHASE_5
    CIOS_RED_PHASE
    CIOS_SHIFT

    FP479_SQR_PHASE_6
    CIOS_RED_PHASE
    CIOS_SHIFT

    FP479_SQR_PHASE_7
    CIOS_RED_PHASE
    CIOS_SHIFT

    subq p479_const+0(%rip), %r8
    sbbq p479_const+8(%rip), %r9
    sbbq p479_const+16(%rip), %r10
    sbbq p479_const+24(%rip), %r11
    sbbq p479_const+32(%rip), %r12
    sbbq p479_const+40(%rip), %r13
    sbbq p479_const+48(%rip), %r14
    sbbq p479_const+56(%rip), %r15

    sbbq %rax, %rax
    movq p479_const+16(%rip), %rdx
    andq %rax, %rdx
    movq p479_const+24(%rip), %rcx
    andq %rax, %rcx
    movq p479_const+32(%rip), %rbx
    andq %rax, %rbx
    movq p479_const+40(%rip), %rbp
    andq %rax, %rbp
    movq p479_const+48(%rip), %rsi
    andq %rax, %rsi
    movq p479_const+56(%rip), %rdi
    andq %rax, %rdi

    addq %rax, %r8
    adcq %rax, %r9
    adcq %rdx, %r10
    adcq %rcx, %r11
    adcq %rbx, %r12
    adcq %rbp, %r13
    adcq %rsi, %r14
    adcq %rdi, %r15

    movq -8(%rsp), %rax
    movq %r8, 0(%rax)
    movq %r9, 8(%rax)
    movq %r10, 16(%rax)
    movq %r11, 24(%rax)
    movq %r12, 32(%rax)
    movq %r13, 40(%rax)
    movq %r14, 48(%rax)
    movq %r15, 56(%rax)

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

    .globl fp479_half_asm
    .type fp479_half_asm, @function
    .p2align 4
fp479_half_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    movq %rdi, %rbx

    movq 0(%rsi), %r8
    movq 8(%rsi), %r9
    movq 16(%rsi), %r10
    movq 24(%rsi), %r11
    movq 32(%rsi), %rax
    movq 40(%rsi), %rcx
    movq 48(%rsi), %r12
    movq 56(%rsi), %rsi

    movq %r8, %rdx
    andq $1, %rdx

    FP479_COND_ADD_P %r8, %r9, %r10, %r11, %rax, %rcx, %r12, %rsi, %rdx, %rbp, %rdi

    shrdq $1, %r9, %r8
    shrdq $1, %r10, %r9
    shrdq $1, %r11, %r10
    shrdq $1, %rax, %r11
    shrdq $1, %rcx, %rax
    shrdq $1, %r12, %rcx
    shrdq $1, %rsi, %r12
    shrq $1, %rsi

    movq %r8, 0(%rbx)
    movq %r9, 8(%rbx)
    movq %r10, 16(%rbx)
    movq %r11, 24(%rbx)
    movq %rax, 32(%rbx)
    movq %rcx, 40(%rbx)
    movq %r12, 48(%rbx)
    movq %rsi, 56(%rbx)
    popq %r12
    popq %rbp
    popq %rbx
    ret

    .section .note.GNU-stack,"",@progbits
