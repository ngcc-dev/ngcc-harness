/*
 * NGCC_3 p1534 optimized saturated Montgomery paths, x86-64.
 *
 * Field elements use the first 24 limbs of fp_t; limbs 24 and 25 are cleared
 * for compatibility with the existing NWORDS_FIELD=26 NGCC_3 headers.
 */

    .text

    .section .rodata
    .p2align 3
p1534_24_const:
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0x74f745f71f7b456b
    .quad 0x4e9c3615eab1df2a
    .quad 0xe3cd124452453081
    .quad 0xdae93fda6645a281
    .quad 0x3a5035c534b5f3af
    .quad 0x8cd6ed92b85bd0cb
    .quad 0xb3e2fa38f5419baf
    .quad 0xef81c420b2dfbb3f
    .quad 0x76f58123de8fab63
    .quad 0xf4c3a493432f0854
    .quad 0x07a2821a8511c268
    .quad 0xbdfbff3ede30e076
    .quad 0x14541bc051219e91
    .quad 0xa886397f71da19e5
    .quad 0x5554f51ce0e783f6
    .quad 0x22606deb050ed144

    .p2align 3
p1534_24_zero:
    .zero 192

    .p2align 3
p1534_24_red_const:
    .quad 0x74f745f71f7b456c
    .quad 0x4e9c3615eab1df2a
    .quad 0xe3cd124452453081
    .quad 0xdae93fda6645a281
    .quad 0x3a5035c534b5f3af
    .quad 0x8cd6ed92b85bd0cb
    .quad 0xb3e2fa38f5419baf
    .quad 0xef81c420b2dfbb3f
    .quad 0x76f58123de8fab63
    .quad 0xf4c3a493432f0854
    .quad 0x07a2821a8511c268
    .quad 0xbdfbff3ede30e076
    .quad 0x14541bc051219e91
    .quad 0xa886397f71da19e5
    .quad 0x5554f51ce0e783f6
    .quad 0x22606deb050ed144

    .text

/* c = a + b mod p */
    .globl fp1534_24_add_asm
    .type fp1534_24_add_asm, @function
    .p2align 4
fp1534_24_add_asm:
    subq $192, %rsp

    movq 0(%rsi), %rax
    addq 0(%rdx), %rax
    movq %rax, 0(%rdi)
.irp idx,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq \idx*8(%rsi), %rax
    adcq \idx*8(%rdx), %rax
    movq %rax, \idx*8(%rdi)
.endr
    movq $0, 192(%rdi)
    movq $0, 200(%rdi)

    leaq p1534_24_const(%rip), %rcx
    movq $-1, %r9
    movq 0(%rdi), %rax
    subq %r9, %rax
    movq %rax, 0(%rsp)
.irp idx,1,2,3,4,5,6,7
    movq \idx*8(%rdi), %rax
    sbbq %r9, %rax
    movq %rax, \idx*8(%rsp)
.endr
.irp idx,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq \idx*8(%rdi), %rax
    sbbq \idx*8(%rcx), %rax
    movq %rax, \idx*8(%rsp)
.endr

.irp idx,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq \idx*8(%rdi), %rax
    movq \idx*8(%rsp), %r8
    cmovncq %r8, %rax
    movq %rax, \idx*8(%rdi)
.endr

    addq $192, %rsp
    ret

/* c = a - b mod p */
    .globl fp1534_24_sub_asm
    .type fp1534_24_sub_asm, @function
    .p2align 4
fp1534_24_sub_asm:
    movq 0(%rsi), %rax
    subq 0(%rdx), %rax
    movq %rax, 0(%rdi)
.irp idx,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq \idx*8(%rsi), %rax
    sbbq \idx*8(%rdx), %rax
    movq %rax, \idx*8(%rdi)
.endr

    sbbq %r9, %r9
    leaq p1534_24_const(%rip), %rcx
    leaq p1534_24_zero(%rip), %r8
    testq %r9, %r9
    cmovnzq %rcx, %r8

    addq %r9, 0(%rdi)
.irp idx,1,2,3,4,5,6,7
    adcq %r9, \idx*8(%rdi)
.endr
.irp idx,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq \idx*8(%r8), %rax
    adcq %rax, \idx*8(%rdi)
.endr
    movq $0, 192(%rdi)
    movq $0, 200(%rdi)
    ret

/* c = a / 2 mod p */
    .globl fp1534_24_half_asm
    .type fp1534_24_half_asm, @function
    .p2align 4
fp1534_24_half_asm:
    movq 0(%rsi), %r9
    andq $1, %r9
    negq %r9

    leaq p1534_24_const(%rip), %rcx
    leaq p1534_24_zero(%rip), %r8
    testq %r9, %r9
    cmovnzq %rcx, %r8

    movq 0(%rsi), %rax
    addq %r9, %rax
    movq %rax, 0(%rdi)
.irp idx,1,2,3,4,5,6,7
    movq \idx*8(%rsi), %rax
    adcq %r9, %rax
    movq %rax, \idx*8(%rdi)
.endr
.irp idx,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq \idx*8(%r8), %rax
    adcq \idx*8(%rsi), %rax
    movq %rax, \idx*8(%rdi)
.endr

.irp idx,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22
    movq \idx*8(%rdi), %rax
    movq (\idx+1)*8(%rdi), %r10
    shrdq $1, %r10, %rax
    movq %rax, \idx*8(%rdi)
.endr
    shrq $1, 184(%rdi)
    movq $0, 192(%rdi)
    movq $0, 200(%rdi)
    ret

/* c = a * b * R^-1 mod p, fused two-column product/reduction */
    .globl fp1534_24_montmul_pair_adx_asm
    .type fp1534_24_montmul_pair_adx_asm, @function
    .p2align 4
fp1534_24_montmul_pair_adx_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rdx, %rbx
    leaq p1534_24_red_const(%rip), %rbp
    subq $392, %rsp
    xorl %r8d, %r8d

    xorl %r13d, %r13d
    xorl %r14d, %r14d
    xorl %r15d, %r15d

.macro ADD_ACC0
    addq %r10, %r13
    adcq %r11, %r14
    adcq %r8, %r15
.endm

.macro ADD_ACC1
    addq %r10, %rax
    adcq %r11, %rcx
    adcq %r8, %r12
.endm

/* The .if blocks in this macro are assembler-time filters. They select which
 * multiplication and reduction terms are emitted for each unrolled column and
 * do not generate runtime branches in the object code.
 */
.macro FUSED_PAIR k
    xorl %eax, %eax
    xorl %ecx, %ecx
    xorl %r12d, %r12d
.irp i,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
.if (((\k - \i) >= 0) && ((\k - \i) < 24)) || (((\k + 1 - \i) >= 0) && ((\k + 1 - \i) < 24))
    movq \i*8(%rsi), %rdx
.if ((\k - \i) >= 0) && ((\k - \i) < 24)
    mulxq (\k - \i)*8(%rbx), %r10, %r11
    ADD_ACC0
.endif
.if ((\k + 1 - \i) >= 0) && ((\k + 1 - \i) < 24)
    mulxq (\k + 1 - \i)*8(%rbx), %r10, %r11
    ADD_ACC1
.endif
.endif
.endr
.irp i,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
.if (((\k - 8 - \i) >= 0) && ((\k - 8 - \i) < 16)) || (((\k + 1 - 8 - \i) >= 0) && ((\k + 1 - 8 - \i) < 16))
    movq \i*8(%rsp), %rdx
.if ((\k - 8 - \i) >= 0) && ((\k - 8 - \i) < 16)
    mulxq (\k - 8 - \i)*8(%rbp), %r10, %r11
    ADD_ACC0
.endif
.if ((\k + 1 - 8 - \i) >= 0) && ((\k + 1 - 8 - \i) < 16)
    mulxq (\k + 1 - 8 - \i)*8(%rbp), %r10, %r11
    ADD_ACC1
.endif
.endif
.endr
    movq %r13, \k*8(%rsp)
    addq %r14, %rax
    adcq %r15, %rcx
    adcq %r8, %r12
    movq %rax, (\k+1)*8(%rsp)
    movq %rcx, %r13
    movq %r12, %r14
    xorl %r15d, %r15d
.endm

.irp col,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46
    FUSED_PAIR \col
.endr

    leaq p1534_24_const(%rip), %rcx
    movq $-1, %r9
    movq 192(%rsp), %rax
    subq %r9, %rax
    movq %rax, 0(%rdi)
.irp idx,1,2,3,4,5,6,7
    movq 192+\idx*8(%rsp), %rax
    sbbq %r9, %rax
    movq %rax, \idx*8(%rdi)
.endr
.irp idx,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq 192+\idx*8(%rsp), %rax
    sbbq \idx*8(%rcx), %rax
    movq %rax, \idx*8(%rdi)
.endr

    sbbq %r9, %r9
    testq %r9, %r9
.irp idx,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq \idx*8(%rdi), %rax
    movq 192+\idx*8(%rsp), %r10
    cmovnzq %r10, %rax
    movq %rax, \idx*8(%rdi)
.endr
    movq $0, 192(%rdi)
    movq $0, 200(%rdi)

    addq $392, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

/* c = a^2 * R^-1 mod p */
    .globl fp1534_24_montsqr_adx_asm
    .type fp1534_24_montsqr_adx_asm, @function
    .p2align 4
fp1534_24_montsqr_adx_asm:
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    subq $392, %rsp
    xorl %r8d, %r8d

    xorl %r13d, %r13d
    xorl %r14d, %r14d
    xorl %r15d, %r15d

.macro SQR_ACC0_DIAG i
    mulxq \i*8(%rsi), %r10, %r11
    addq %r10, %r13
    adcq %r11, %r14
    adcq %r8, %r15
.endm

.macro SQR_ACC0_CROSS j
    mulxq \j*8(%rsi), %r10, %r11
    addq %r10, %r10
    adcq %r11, %r11
    adcq %r8, %r15
    addq %r10, %r13
    adcq %r11, %r14
    adcq %r8, %r15
.endm

.macro SQR_ACC1_DIAG i
    mulxq \i*8(%rsi), %r10, %r11
    addq %r10, %rax
    adcq %r11, %rcx
    adcq %r8, %r12
.endm

.macro SQR_ACC1_CROSS j
    mulxq \j*8(%rsi), %r10, %r11
    addq %r10, %r10
    adcq %r11, %r11
    adcq %r8, %r12
    addq %r10, %rax
    adcq %r11, %rcx
    adcq %r8, %r12
.endm

/* These .if blocks are assembler-time filters evaluated while expanding the
 * macro. They emit only the valid square/reduction terms for each fixed column.
 */
.macro SQR_FUSED_PAIR k
    xorl %eax, %eax
    xorl %ecx, %ecx
    xorl %r12d, %r12d
.irp i,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
.if (((\k - \i) >= 0) && ((\k - \i) < 24) && (\i <= (\k - \i))) || (((\k + 1 - \i) >= 0) && ((\k + 1 - \i) < 24) && (\i <= (\k + 1 - \i)))
    movq \i*8(%rsi), %rdx
.if ((\k - \i) >= 0) && ((\k - \i) < 24) && (\i <= (\k - \i))
.if (\i == (\k - \i))
    SQR_ACC0_DIAG \i
.else
    SQR_ACC0_CROSS (\k - \i)
.endif
.endif
.if ((\k + 1 - \i) >= 0) && ((\k + 1 - \i) < 24) && (\i <= (\k + 1 - \i))
.if (\i == (\k + 1 - \i))
    SQR_ACC1_DIAG \i
.else
    SQR_ACC1_CROSS (\k + 1 - \i)
.endif
.endif
.endif
.endr
.irp i,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
.if (((\k - 8 - \i) >= 0) && ((\k - 8 - \i) < 16)) || (((\k + 1 - 8 - \i) >= 0) && ((\k + 1 - 8 - \i) < 16))
    movq \i*8(%rsp), %rdx
.if ((\k - 8 - \i) >= 0) && ((\k - 8 - \i) < 16)
    mulxq p1534_24_red_const+(\k - 8 - \i)*8(%rip), %r10, %r11
    ADD_ACC0
.endif
.if ((\k + 1 - 8 - \i) >= 0) && ((\k + 1 - 8 - \i) < 16)
    mulxq p1534_24_red_const+(\k + 1 - 8 - \i)*8(%rip), %r10, %r11
    ADD_ACC1
.endif
.endif
.endr
    movq %r13, \k*8(%rsp)
    addq %r14, %rax
    adcq %r15, %rcx
    adcq %r8, %r12
    movq %rax, (\k+1)*8(%rsp)
    movq %rcx, %r13
    movq %r12, %r14
    xorl %r15d, %r15d
.endm

.irp col,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46
    SQR_FUSED_PAIR \col
.endr

    leaq p1534_24_const(%rip), %rcx
    movq $-1, %r9
    movq 192(%rsp), %rax
    subq %r9, %rax
    movq %rax, 0(%rdi)
.irp idx,1,2,3,4,5,6,7
    movq 192+\idx*8(%rsp), %rax
    sbbq %r9, %rax
    movq %rax, \idx*8(%rdi)
.endr
.irp idx,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq 192+\idx*8(%rsp), %rax
    sbbq \idx*8(%rcx), %rax
    movq %rax, \idx*8(%rdi)
.endr

    sbbq %r9, %r9
    testq %r9, %r9
.irp idx,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
    movq \idx*8(%rdi), %rax
    movq 192+\idx*8(%rsp), %r10
    cmovnzq %r10, %rax
    movq %rax, \idx*8(%rdi)
.endr
    movq $0, 192(%rdi)
    movq $0, 200(%rdi)

    addq $392, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    ret

    .section .note.GNU-stack,"",@progbits
