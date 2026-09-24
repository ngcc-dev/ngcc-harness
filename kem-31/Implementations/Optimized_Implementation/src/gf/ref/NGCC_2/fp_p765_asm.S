/*
 * NGCC_2 p765 optimized saturated Montgomery paths, x86-64.
 *
 * Field elements use the first 12 limbs of fp_t; limb 12 is cleared for
 * compatibility with the existing NWORDS_FIELD=13 NGCC_2 headers.
 */

    .text

    .section .rodata
    .p2align 3
p765_12_const:
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffff
    .quad 0x30eb7aa16fc48c0f
    .quad 0xe7bb8e9933551de5
    .quad 0x4acf5820369b15b0
    .quad 0x140243d6c8bdb274
    .quad 0xa1434ee88e42e1e6
    .quad 0x301e3eeaebbf1ce8
    .quad 0xbf54fba3dbb47692
    .quad 0x1bfa60de9c0419b7

    .p2align 3
p765_12_zero:
    .zero 96

    .p2align 3
p765_12_red_const:
    .quad 0x30eb7aa16fc48c10
    .quad 0xe7bb8e9933551de5
    .quad 0x4acf5820369b15b0
    .quad 0x140243d6c8bdb274
    .quad 0xa1434ee88e42e1e6
    .quad 0x301e3eeaebbf1ce8
    .quad 0xbf54fba3dbb47692
    .quad 0x1bfa60de9c0419b7

    .text

.macro FP765_12_ADD_COMPONENT off
    movq \off+0(%rsi), %rax
    addq \off+0(%rdx), %rax
    movq %rax, \off+0(%rdi)
.irp idx,1,2,3,4,5,6,7,8,9,10,11
    movq \off+\idx*8(%rsi), %rax
    adcq \off+\idx*8(%rdx), %rax
    movq %rax, \off+\idx*8(%rdi)
.endr
    movq $0, \off+96(%rdi)

    movq \off+0(%rdi), %rax
    subq %r9, %rax
    movq %rax, 0(%rsp)
.irp idx,1,2,3
    movq \off+\idx*8(%rdi), %rax
    sbbq %r9, %rax
    movq %rax, \idx*8(%rsp)
.endr
.irp idx,4,5,6,7,8,9,10,11
    movq \off+\idx*8(%rdi), %rax
    sbbq \idx*8(%rcx), %rax
    movq %rax, \idx*8(%rsp)
.endr

.irp idx,0,1,2,3,4,5,6,7,8,9,10,11
    movq \off+\idx*8(%rdi), %rax
    movq \idx*8(%rsp), %r8
    cmovncq %r8, %rax
    movq %rax, \off+\idx*8(%rdi)
.endr
.endm

.macro FP765_12_DOUBLE_COMPONENT off
    movq \off+0(%rsi), %rax
    addq %rax, %rax
    movq %rax, \off+0(%rdi)
.irp idx,1,2,3,4,5,6,7,8,9,10,11
    movq \off+\idx*8(%rsi), %rax
    adcq %rax, %rax
    movq %rax, \off+\idx*8(%rdi)
.endr
    movq $0, \off+96(%rdi)

    movq \off+0(%rdi), %rax
    subq %r9, %rax
    movq %rax, 0(%rsp)
.irp idx,1,2,3
    movq \off+\idx*8(%rdi), %rax
    sbbq %r9, %rax
    movq %rax, \idx*8(%rsp)
.endr
.irp idx,4,5,6,7,8,9,10,11
    movq \off+\idx*8(%rdi), %rax
    sbbq \idx*8(%rcx), %rax
    movq %rax, \idx*8(%rsp)
.endr

.irp idx,0,1,2,3,4,5,6,7,8,9,10,11
    movq \off+\idx*8(%rdi), %rax
    movq \idx*8(%rsp), %r8
    cmovncq %r8, %rax
    movq %rax, \off+\idx*8(%rdi)
.endr
.endm

.macro FP765_12_SUB_COMPONENT off
    movq \off+0(%rsi), %rax
    subq \off+0(%rdx), %rax
    movq %rax, \off+0(%rdi)
.irp idx,1,2,3,4,5,6,7,8,9,10,11
    movq \off+\idx*8(%rsi), %rax
    sbbq \off+\idx*8(%rdx), %rax
    movq %rax, \off+\idx*8(%rdi)
.endr

    sbbq %r9, %r9
    movq %r10, %r8
    testq %r9, %r9
    cmovnzq %rcx, %r8

    addq %r9, \off+0(%rdi)
.irp idx,1,2,3
    adcq %r9, \off+\idx*8(%rdi)
.endr
.irp idx,4,5,6,7,8,9,10,11
    movq \idx*8(%r8), %rax
    adcq %rax, \off+\idx*8(%rdi)
.endr
    movq $0, \off+96(%rdi)
.endm

.macro FP765_12_NEG_COMPONENT off
    xorl %eax, %eax
    subq \off+0(%rsi), %rax
    movq %rax, \off+0(%rdi)
.irp idx,1,2,3,4,5,6,7,8,9,10,11
    movq %r11, %rax
    sbbq \off+\idx*8(%rsi), %rax
    movq %rax, \off+\idx*8(%rdi)
.endr

    sbbq %r9, %r9
    movq %r10, %r8
    testq %r9, %r9
    cmovnzq %rcx, %r8

    addq %r9, \off+0(%rdi)
.irp idx,1,2,3
    adcq %r9, \off+\idx*8(%rdi)
.endr
.irp idx,4,5,6,7,8,9,10,11
    movq \idx*8(%r8), %rax
    adcq %rax, \off+\idx*8(%rdi)
.endr
    movq $0, \off+96(%rdi)
.endm

.macro FP765_12_HALF_COMPONENT off
    movq \off+0(%rsi), %r9
    andq $1, %r9
    negq %r9

    movq %r11, %r8
    testq %r9, %r9
    cmovnzq %rcx, %r8

    movq \off+0(%rsi), %rax
    addq %r9, %rax
    movq %rax, \off+0(%rdi)
.irp idx,1,2,3
    movq \off+\idx*8(%rsi), %rax
    adcq %r9, %rax
    movq %rax, \off+\idx*8(%rdi)
.endr
.irp idx,4,5,6,7,8,9,10,11
    movq \off+\idx*8(%rsi), %rax
    adcq \idx*8(%r8), %rax
    movq %rax, \off+\idx*8(%rdi)
.endr

.irp idx,0,1,2,3,4,5,6,7,8,9,10
    movq \off+\idx*8(%rdi), %rax
    movq \off+(\idx+1)*8(%rdi), %r10
    shrdq $1, %r10, %rax
    movq %rax, \off+\idx*8(%rdi)
.endr
    shrq $1, \off+88(%rdi)
    movq $0, \off+96(%rdi)
.endm

/* c = a + b mod p */
    .globl fp765_12_add_asm
    .type fp765_12_add_asm, @function
    .p2align 4
fp765_12_add_asm:
    subq $96, %rsp
    leaq p765_12_const(%rip), %rcx
    movq $-1, %r9
    FP765_12_ADD_COMPONENT 0
    addq $96, %rsp
    ret

/* c = 2 * a mod p */
    .globl fp765_12_double_asm
    .type fp765_12_double_asm, @function
    .p2align 4
fp765_12_double_asm:
    subq $96, %rsp
    leaq p765_12_const(%rip), %rcx
    movq $-1, %r9
    FP765_12_DOUBLE_COMPONENT 0
    addq $96, %rsp
    ret

/* c = a - b mod p */
    .globl fp765_12_sub_asm
    .type fp765_12_sub_asm, @function
    .p2align 4
fp765_12_sub_asm:
    leaq p765_12_const(%rip), %rcx
    leaq p765_12_zero(%rip), %r10
    FP765_12_SUB_COMPONENT 0
    ret

/* c = -a mod p */
    .globl fp765_12_neg_asm
    .type fp765_12_neg_asm, @function
    .p2align 4
fp765_12_neg_asm:
    leaq p765_12_const(%rip), %rcx
    leaq p765_12_zero(%rip), %r10
    xorl %r11d, %r11d
    FP765_12_NEG_COMPONENT 0
    ret

/* x = y + z in GF(p^2), with contiguous fp2_t layout. */
    .globl fp765_12_fp2_add_asm
    .type fp765_12_fp2_add_asm, @function
    .p2align 4
fp765_12_fp2_add_asm:
    subq $96, %rsp
    leaq p765_12_const(%rip), %rcx
    movq $-1, %r9
    FP765_12_ADD_COMPONENT 0
    FP765_12_ADD_COMPONENT 104
    addq $96, %rsp
    ret

/* x = y - z in GF(p^2), with contiguous fp2_t layout. */
    .globl fp765_12_fp2_sub_asm
    .type fp765_12_fp2_sub_asm, @function
    .p2align 4
fp765_12_fp2_sub_asm:
    leaq p765_12_const(%rip), %rcx
    leaq p765_12_zero(%rip), %r10
    FP765_12_SUB_COMPONENT 0
    FP765_12_SUB_COMPONENT 104
    ret

/* x = -y in GF(p^2), with contiguous fp2_t layout. */
    .globl fp765_12_fp2_neg_asm
    .type fp765_12_fp2_neg_asm, @function
    .p2align 4
fp765_12_fp2_neg_asm:
    leaq p765_12_const(%rip), %rcx
    leaq p765_12_zero(%rip), %r10
    xorl %r11d, %r11d
    FP765_12_NEG_COMPONENT 0
    FP765_12_NEG_COMPONENT 104
    ret

/* c = a / 2 mod p */
    .globl fp765_12_half_asm
    .type fp765_12_half_asm, @function
    .p2align 4
fp765_12_half_asm:
    leaq p765_12_const(%rip), %rcx
    leaq p765_12_zero(%rip), %r11
    FP765_12_HALF_COMPONENT 0
    ret

/* x = y / 2 in GF(p^2), with contiguous fp2_t layout. */
    .globl fp765_12_fp2_half_asm
    .type fp765_12_fp2_half_asm, @function
    .p2align 4
fp765_12_fp2_half_asm:
    leaq p765_12_const(%rip), %rcx
    leaq p765_12_zero(%rip), %r11
    FP765_12_HALF_COMPONENT 0
    FP765_12_HALF_COMPONENT 104
    ret

/* c = a * b * R^-1 mod p, CIOS Montgomery over 12 saturated limbs */
    .globl fp765_12_montmul_asm
    .type fp765_12_montmul_asm, @function
    .p2align 4
fp765_12_montmul_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    call fp765_12_montmul_nosave_asm
    movq $0, 96(%rdi)
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

/* Internal entry: clobbers callee-saved registers and leaves limb 12 undefined.
 * Public wrappers and modpro finalization clear limb 12 for fp_t compatibility.
 */
    .type fp765_12_montmul_nosave_asm, @function
    .p2align 4
fp765_12_montmul_nosave_asm:
    movq %rdx, %rbx
    subq $200, %rsp

    xorl %eax, %eax
.irp idx,0,1,2,3,4,5,6,7,8,9,10,11,12
    movq %rax, \idx*8(%rsp)
.endr

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

.macro CIOS_MUL_PHASE off, base
    movq \off(%rsi), %rdx
    xorl %eax, %eax
    mulxq 0(%rbx), %rax, %rcx
    adcxq (\base+0)*8(%rsp), %rax
    movq %rax, (\base+0)*8(%rsp)
    mulxq 8(%rbx), %rax, %rbp
    adcxq (\base+1)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+1)*8(%rsp)
    mulxq 16(%rbx), %rax, %rcx
    adcxq (\base+2)*8(%rsp), %rax
    adoxq %rbp, %rax
    movq %rax, (\base+2)*8(%rsp)
    mulxq 24(%rbx), %rax, %rbp
    adcxq (\base+3)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+3)*8(%rsp)
    mulxq 32(%rbx), %rax, %rcx
    adcxq (\base+4)*8(%rsp), %rax
    adoxq %rbp, %rax
    movq %rax, (\base+4)*8(%rsp)
    mulxq 40(%rbx), %rax, %rbp
    adcxq (\base+5)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+5)*8(%rsp)
    mulxq 48(%rbx), %rax, %rcx
    adcxq (\base+6)*8(%rsp), %rax
    adoxq %rbp, %rax
    movq %rax, (\base+6)*8(%rsp)
    mulxq 56(%rbx), %rax, %rbp
    adcxq (\base+7)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+7)*8(%rsp)
    mulxq 64(%rbx), %rax, %rcx
    adcxq (\base+8)*8(%rsp), %rax
    adoxq %rbp, %rax
    movq %rax, (\base+8)*8(%rsp)
    mulxq 72(%rbx), %rax, %rbp
    adcxq (\base+9)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+9)*8(%rsp)
    mulxq 80(%rbx), %rax, %rcx
    adcxq (\base+10)*8(%rsp), %rax
    adoxq %rbp, %rax
    movq %rax, (\base+10)*8(%rsp)
    mulxq 88(%rbx), %rax, %rbp
    adcxq (\base+11)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+11)*8(%rsp)
    movq $0, %rcx
    adcxq %rcx, %rbp
    adoxq %rcx, %rbp
    addq %rbp, (\base+12)*8(%rsp)
.endm

.macro CIOS_RED_PHASE base
    movq (\base+0)*8(%rsp), %rdx
    xorl %eax, %eax
    mulxq p765_12_red_const+0(%rip), %rax, %rcx
    adcxq (\base+4)*8(%rsp), %rax
    movq %rax, (\base+4)*8(%rsp)
    mulxq p765_12_red_const+8(%rip), %rax, %rbp
    adcxq (\base+5)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+5)*8(%rsp)
    mulxq p765_12_red_const+16(%rip), %rax, %rcx
    adcxq (\base+6)*8(%rsp), %rax
    adoxq %rbp, %rax
    movq %rax, (\base+6)*8(%rsp)
    mulxq p765_12_red_const+24(%rip), %rax, %rbp
    adcxq (\base+7)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+7)*8(%rsp)
    mulxq p765_12_red_const+32(%rip), %rax, %rcx
    adcxq (\base+8)*8(%rsp), %rax
    adoxq %rbp, %rax
    movq %rax, (\base+8)*8(%rsp)
    mulxq p765_12_red_const+40(%rip), %rax, %rbp
    adcxq (\base+9)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+9)*8(%rsp)
    mulxq p765_12_red_const+48(%rip), %rax, %rcx
    adcxq (\base+10)*8(%rsp), %rax
    adoxq %rbp, %rax
    movq %rax, (\base+10)*8(%rsp)
    mulxq p765_12_red_const+56(%rip), %rax, %rbp
    adcxq (\base+11)*8(%rsp), %rax
    adoxq %rcx, %rax
    movq %rax, (\base+11)*8(%rsp)
    movq $0, %rcx
    adcxq %rcx, %rbp
    adoxq %rcx, %rbp
    addq %rbp, (\base+12)*8(%rsp)
.endm

.macro CIOS_ROUND off, base
    CIOS_MUL_PHASE \off, \base
    CIOS_RED_PHASE \base
    movq $0, (\base+13)*8(%rsp)
.endm

.macro CIOS_ROUND_LAST off, base
    CIOS_MUL_PHASE \off, \base
    CIOS_RED_PHASE \base
.endm

    CIOS_ROUND 0, 0
    CIOS_ROUND 8, 1
    CIOS_ROUND 16, 2
    CIOS_ROUND 24, 3
    CIOS_ROUND 32, 4
    CIOS_ROUND 40, 5
    CIOS_ROUND 48, 6
    CIOS_ROUND 56, 7
    CIOS_ROUND 64, 8
    CIOS_ROUND 72, 9
    CIOS_ROUND 80, 10
    CIOS_ROUND_LAST 88, 11

    leaq p765_12_const(%rip), %rcx
    movq $-1, %r9
    movq 96(%rsp), %rax
    subq %r9, %rax
    movq %rax, 0(%rdi)
.irp idx,1,2,3
    movq 96+\idx*8(%rsp), %rax
    sbbq %r9, %rax
    movq %rax, \idx*8(%rdi)
.endr
.irp idx,4,5,6,7,8,9,10,11
    movq 96+\idx*8(%rsp), %rax
    sbbq \idx*8(%rcx), %rax
    movq %rax, \idx*8(%rdi)
.endr

    sbbq %r9, %r9
    testq %r9, %r9
.irp idx,0,1,2,3,4,5,6,7,8,9,10,11
    movq \idx*8(%rdi), %rax
    movq 96+\idx*8(%rsp), %r10
    cmovnzq %r10, %rax
    movq %rax, \idx*8(%rdi)
.endr

    addq $200, %rsp
    ret

/* c = a^2 * R^-1 mod p */
    .globl fp765_12_montsqr_asm
    .type fp765_12_montsqr_asm, @function
    .p2align 4
fp765_12_montsqr_asm:
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    call fp765_12_montsqr_fast_nosave_asm
    movq $0, 96(%rdi)
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    ret

/* Internal in-place repeated squaring: rdi = rsi = field element, ecx = count. */
    .type fp765_12_nsqr_inplace_nosave_asm, @function
    .p2align 4
fp765_12_nsqr_inplace_nosave_asm:
    subq $200, %rsp
    movl %ecx, %ebx

.Lfp765_12_nsqr_loop:
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

/* The .if blocks in this macro are assembler-time filters. They select which
 * terms are emitted for each unrolled square column and do not generate runtime
 * branches in the object code.
 */
.macro SQR_FUSED_PAIR k
    xorl %eax, %eax
    xorl %ecx, %ecx
    xorl %r12d, %r12d
.irp i,0,1,2,3,4,5,6,7,8,9,10,11
.if (((\k - \i) >= 0) && ((\k - \i) < 12) && (\i <= (\k - \i))) || (((\k + 1 - \i) >= 0) && ((\k + 1 - \i) < 12) && (\i <= (\k + 1 - \i)))
    movq \i*8(%rsi), %rdx
.if ((\k - \i) >= 0) && ((\k - \i) < 12) && (\i <= (\k - \i))
.if (\i == (\k - \i))
    SQR_ACC0_DIAG \i
.else
    SQR_ACC0_CROSS (\k - \i)
.endif
.endif
.if ((\k + 1 - \i) >= 0) && ((\k + 1 - \i) < 12) && (\i <= (\k + 1 - \i))
.if (\i == (\k + 1 - \i))
    SQR_ACC1_DIAG \i
.else
    SQR_ACC1_CROSS (\k + 1 - \i)
.endif
.endif
.endif
.endr
.irp i,0,1,2,3,4,5,6,7,8,9,10,11
.if (((\k - 4 - \i) >= 0) && ((\k - 4 - \i) < 8)) || (((\k + 1 - 4 - \i) >= 0) && ((\k + 1 - 4 - \i) < 8))
    movq \i*8(%rsp), %rdx
.if ((\k - 4 - \i) >= 0) && ((\k - 4 - \i) < 8)
    mulxq p765_12_red_const+(\k - 4 - \i)*8(%rip), %r10, %r11
    ADD_ACC0
.endif
.if ((\k + 1 - 4 - \i) >= 0) && ((\k + 1 - 4 - \i) < 8)
    mulxq p765_12_red_const+(\k + 1 - 4 - \i)*8(%rip), %r10, %r11
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

.irp col,0,2,4,6,8,10,12,14,16,18,20,22
    SQR_FUSED_PAIR \col
.endr

    leaq p765_12_const(%rip), %rcx
    movq $-1, %r9
    movq 96(%rsp), %rax
    subq %r9, %rax
    movq %rax, 0(%rdi)
.irp idx,1,2,3
    movq 96+\idx*8(%rsp), %rax
    sbbq %r9, %rax
    movq %rax, \idx*8(%rdi)
.endr
.irp idx,4,5,6,7,8,9,10,11
    movq 96+\idx*8(%rsp), %rax
    sbbq \idx*8(%rcx), %rax
    movq %rax, \idx*8(%rdi)
.endr

    sbbq %r9, %r9
    testq %r9, %r9
.irp idx,0,1,2,3,4,5,6,7,8,9,10,11
    movq \idx*8(%rdi), %rax
    movq 96+\idx*8(%rsp), %r10
    cmovnzq %r10, %rax
    movq %rax, \idx*8(%rdi)
.endr

    decl %ebx
    jnz .Lfp765_12_nsqr_loop

.Lfp765_12_nsqr_done:
    addq $200, %rsp
    ret

/* Internal single-square fast path: clobbers callee-saved registers and leaves limb 12 undefined. */
    .type fp765_12_montsqr_fast_nosave_asm, @function
    .p2align 4
fp765_12_montsqr_fast_nosave_asm:
    subq $200, %rsp
    xorl %r8d, %r8d

    xorl %r13d, %r13d
    xorl %r14d, %r14d
    xorl %r15d, %r15d

.irp col,0,2,4,6,8,10,12,14,16,18,20,22
    SQR_FUSED_PAIR \col
.endr

    leaq p765_12_const(%rip), %rcx
    movq $-1, %r9
    movq 96(%rsp), %rax
    subq %r9, %rax
    movq %rax, 0(%rdi)
.irp idx,1,2,3
    movq 96+\idx*8(%rsp), %rax
    sbbq %r9, %rax
    movq %rax, \idx*8(%rdi)
.endr
.irp idx,4,5,6,7,8,9,10,11
    movq 96+\idx*8(%rsp), %rax
    sbbq \idx*8(%rcx), %rax
    movq %rax, \idx*8(%rdi)
.endr

    sbbq %r9, %r9
    testq %r9, %r9
.irp idx,0,1,2,3,4,5,6,7,8,9,10,11
    movq \idx*8(%rdi), %rax
    movq 96+\idx*8(%rsp), %r10
    cmovnzq %r10, %rax
    movq %rax, \idx*8(%rdi)
.endr

    addq $200, %rsp
    ret

/* z = x^((p-3)/4), generated NGCC-2 progenitor chain over 12x64 Montgomery asm. */
    .globl fp765_12_modpro_asm
    .type fp765_12_modpro_asm, @function
    .p2align 4
fp765_12_modpro_asm:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $3336, %rsp
    movq %rdi, 3328(%rsp)
    movq 0(%rsi), %rax
    movq %rax, 0(%rsp)
    movq 8(%rsi), %rax
    movq %rax, 8(%rsp)
    movq 16(%rsi), %rax
    movq %rax, 16(%rsp)
    movq 24(%rsi), %rax
    movq %rax, 24(%rsp)
    movq 32(%rsi), %rax
    movq %rax, 32(%rsp)
    movq 40(%rsi), %rax
    movq %rax, 40(%rsp)
    movq 48(%rsi), %rax
    movq %rax, 48(%rsp)
    movq 56(%rsi), %rax
    movq %rax, 56(%rsp)
    movq 64(%rsi), %rax
    movq %rax, 64(%rsp)
    movq 72(%rsi), %rax
    movq %rax, 72(%rsp)
    movq 80(%rsi), %rax
    movq %rax, 80(%rsp)
    movq 88(%rsi), %rax
    movq %rax, 88(%rsp)
    leaq 1664(%rsp), %rdi
    leaq 0(%rsp), %rsi
    call fp765_12_montsqr_fast_nosave_asm
    leaq 104(%rsp), %rdi
    leaq 0(%rsp), %rsi
    leaq 1664(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 104(%rsp), %rsi
    call fp765_12_montsqr_fast_nosave_asm
    leaq 624(%rsp), %rdi
    leaq 104(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 208(%rsp), %rdi
    leaq 0(%rsp), %rsi
    leaq 624(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1040(%rsp), %rdi
    leaq 312(%rsp), %rsi
    leaq 208(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 416(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    leaq 1040(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 0(%rsp), %rsi
    leaq 416(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1144(%rsp), %rdi
    leaq 0(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1248(%rsp), %rdi
    leaq 104(%rsp), %rsi
    leaq 1144(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 728(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    leaq 1248(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2184(%rsp), %rdi
    leaq 0(%rsp), %rsi
    leaq 728(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 832(%rsp), %rdi
    leaq 624(%rsp), %rsi
    leaq 2184(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1768(%rsp), %rdi
    leaq 104(%rsp), %rsi
    leaq 832(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2080(%rsp), %rdi
    leaq 416(%rsp), %rsi
    leaq 1768(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1352(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    leaq 2080(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 520(%rsp), %rdi
    leaq 728(%rsp), %rsi
    leaq 1768(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1456(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 520(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1560(%rsp), %rdi
    leaq 2184(%rsp), %rsi
    leaq 1352(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 208(%rsp), %rdi
    leaq 728(%rsp), %rsi
    leaq 520(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 728(%rsp), %rdi
    leaq 728(%rsp), %rsi
    leaq 208(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 936(%rsp), %rdi
    leaq 312(%rsp), %rsi
    leaq 728(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 416(%rsp), %rsi
    leaq 936(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2288(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1872(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    leaq 2288(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1664(%rsp), %rdi
    leaq 1040(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1040(%rsp), %rdi
    leaq 1144(%rsp), %rsi
    leaq 1872(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2496(%rsp), %rdi
    leaq 416(%rsp), %rsi
    leaq 1040(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 416(%rsp), %rdi
    leaq 416(%rsp), %rsi
    leaq 2496(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 3120(%rsp), %rdi
    leaq 1352(%rsp), %rsi
    leaq 416(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1976(%rsp), %rdi
    leaq 1144(%rsp), %rsi
    leaq 3120(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1248(%rsp), %rdi
    leaq 1248(%rsp), %rsi
    leaq 1976(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 520(%rsp), %rdi
    leaq 520(%rsp), %rsi
    leaq 1248(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1352(%rsp), %rdi
    leaq 1144(%rsp), %rsi
    leaq 520(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1248(%rsp), %rdi
    leaq 1248(%rsp), %rsi
    leaq 1352(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1456(%rsp), %rdi
    leaq 1456(%rsp), %rsi
    leaq 1248(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2600(%rsp), %rdi
    leaq 2288(%rsp), %rsi
    leaq 1456(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1768(%rsp), %rdi
    leaq 1768(%rsp), %rsi
    leaq 2600(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2288(%rsp), %rdi
    leaq 2184(%rsp), %rsi
    leaq 1768(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2392(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 2288(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 208(%rsp), %rdi
    leaq 2184(%rsp), %rsi
    leaq 2392(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1560(%rsp), %rdi
    leaq 1560(%rsp), %rsi
    leaq 208(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2808(%rsp), %rdi
    leaq 728(%rsp), %rsi
    leaq 208(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 624(%rsp), %rdi
    leaq 624(%rsp), %rsi
    leaq 2808(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    leaq 624(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1144(%rsp), %rdi
    leaq 1144(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2912(%rsp), %rdi
    leaq 2496(%rsp), %rsi
    leaq 1144(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 3016(%rsp), %rdi
    leaq 2184(%rsp), %rsi
    leaq 2912(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2184(%rsp), %rdi
    leaq 1976(%rsp), %rsi
    leaq 3016(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 832(%rsp), %rdi
    leaq 832(%rsp), %rsi
    leaq 2184(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1976(%rsp), %rdi
    leaq 2080(%rsp), %rsi
    leaq 2184(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1352(%rsp), %rdi
    leaq 1352(%rsp), %rsi
    leaq 1976(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1872(%rsp), %rdi
    leaq 1872(%rsp), %rsi
    leaq 1352(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 3224(%rsp), %rdi
    leaq 2392(%rsp), %rsi
    leaq 3016(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1664(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    leaq 3224(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1040(%rsp), %rdi
    leaq 1040(%rsp), %rsi
    leaq 1664(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2704(%rsp), %rdi
    leaq 1768(%rsp), %rsi
    leaq 1040(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 416(%rsp), %rdi
    leaq 416(%rsp), %rsi
    leaq 2704(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2496(%rsp), %rdi
    leaq 832(%rsp), %rsi
    leaq 416(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1768(%rsp), %rdi
    leaq 520(%rsp), %rsi
    leaq 2496(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 624(%rsp), %rdi
    leaq 624(%rsp), %rsi
    leaq 1768(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 832(%rsp), %rdi
    leaq 728(%rsp), %rsi
    leaq 624(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 728(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 832(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2808(%rsp), %rdi
    leaq 2808(%rsp), %rsi
    leaq 728(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2288(%rsp), %rdi
    leaq 2288(%rsp), %rsi
    leaq 2808(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    leaq 2288(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1144(%rsp), %rdi
    leaq 1144(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 3120(%rsp), %rdi
    leaq 3120(%rsp), %rsi
    leaq 1144(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2600(%rsp), %rdi
    leaq 2600(%rsp), %rsi
    leaq 3120(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 3016(%rsp), %rdi
    leaq 3016(%rsp), %rsi
    leaq 2600(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1352(%rsp), %rdi
    leaq 1352(%rsp), %rsi
    leaq 3016(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 832(%rsp), %rdi
    leaq 832(%rsp), %rsi
    leaq 1352(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1456(%rsp), %rdi
    leaq 1456(%rsp), %rsi
    leaq 832(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1976(%rsp), %rdi
    leaq 1976(%rsp), %rsi
    leaq 1456(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 520(%rsp), %rdi
    leaq 520(%rsp), %rsi
    leaq 1976(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2912(%rsp), %rdi
    leaq 2912(%rsp), %rsi
    leaq 520(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1872(%rsp), %rdi
    leaq 1872(%rsp), %rsi
    leaq 2912(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1664(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    leaq 1872(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 416(%rsp), %rdi
    leaq 416(%rsp), %rsi
    leaq 1664(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2080(%rsp), %rdi
    leaq 2080(%rsp), %rsi
    leaq 416(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1040(%rsp), %rdi
    leaq 1040(%rsp), %rsi
    leaq 2080(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 1560(%rsp), %rdi
    leaq 1560(%rsp), %rsi
    leaq 1040(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 728(%rsp), %rdi
    leaq 728(%rsp), %rsi
    leaq 1560(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 3224(%rsp), %rdi
    leaq 3224(%rsp), %rsi
    leaq 728(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2184(%rsp), %rdi
    leaq 2184(%rsp), %rsi
    leaq 3224(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2704(%rsp), %rdi
    leaq 2704(%rsp), %rsi
    leaq 2184(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 936(%rsp), %rdi
    leaq 936(%rsp), %rsi
    leaq 2704(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 2392(%rsp), %rdi
    leaq 2392(%rsp), %rsi
    leaq 936(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    leaq 208(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 2392(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $19, %ecx
    leaq 3224(%rsp), %rdi
    leaq 3224(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 3120(%rsp), %rdi
    leaq 3120(%rsp), %rsi
    leaq 3224(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $22, %ecx
    leaq 3120(%rsp), %rdi
    leaq 3120(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 3016(%rsp), %rdi
    leaq 3016(%rsp), %rsi
    leaq 3120(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 3016(%rsp), %rdi
    leaq 3016(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2912(%rsp), %rdi
    leaq 2912(%rsp), %rsi
    leaq 3016(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $15, %ecx
    leaq 2912(%rsp), %rdi
    leaq 2912(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2808(%rsp), %rdi
    leaq 2808(%rsp), %rsi
    leaq 2912(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $19, %ecx
    leaq 2808(%rsp), %rdi
    leaq 2808(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2704(%rsp), %rdi
    leaq 2704(%rsp), %rsi
    leaq 2808(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $17, %ecx
    leaq 2704(%rsp), %rdi
    leaq 2704(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2600(%rsp), %rdi
    leaq 2600(%rsp), %rsi
    leaq 2704(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 2600(%rsp), %rdi
    leaq 2600(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2496(%rsp), %rdi
    leaq 2496(%rsp), %rsi
    leaq 2600(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $19, %ecx
    leaq 2496(%rsp), %rdi
    leaq 2496(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2392(%rsp), %rdi
    leaq 2392(%rsp), %rsi
    leaq 2496(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $14, %ecx
    leaq 2392(%rsp), %rdi
    leaq 2392(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2288(%rsp), %rdi
    leaq 2288(%rsp), %rsi
    leaq 2392(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $19, %ecx
    leaq 2288(%rsp), %rdi
    leaq 2288(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2184(%rsp), %rdi
    leaq 2184(%rsp), %rsi
    leaq 2288(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $20, %ecx
    leaq 2184(%rsp), %rdi
    leaq 2184(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 2080(%rsp), %rdi
    leaq 2080(%rsp), %rsi
    leaq 2184(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 2080(%rsp), %rdi
    leaq 2080(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1976(%rsp), %rdi
    leaq 1976(%rsp), %rsi
    leaq 2080(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $18, %ecx
    leaq 1976(%rsp), %rdi
    leaq 1976(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1872(%rsp), %rdi
    leaq 1872(%rsp), %rsi
    leaq 1976(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $13, %ecx
    leaq 1872(%rsp), %rdi
    leaq 1872(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1768(%rsp), %rdi
    leaq 1768(%rsp), %rsi
    leaq 1872(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $24, %ecx
    leaq 1768(%rsp), %rdi
    leaq 1768(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1664(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    leaq 1768(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 1664(%rsp), %rdi
    leaq 1664(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1560(%rsp), %rdi
    leaq 1560(%rsp), %rsi
    leaq 1664(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 1560(%rsp), %rdi
    leaq 1560(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1456(%rsp), %rdi
    leaq 1456(%rsp), %rsi
    leaq 1560(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $18, %ecx
    leaq 1456(%rsp), %rdi
    leaq 1456(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1352(%rsp), %rdi
    leaq 1352(%rsp), %rsi
    leaq 1456(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $11, %ecx
    leaq 1352(%rsp), %rdi
    leaq 1352(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1248(%rsp), %rdi
    leaq 1248(%rsp), %rsi
    leaq 1352(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $21, %ecx
    leaq 1248(%rsp), %rdi
    leaq 1248(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1144(%rsp), %rdi
    leaq 1144(%rsp), %rsi
    leaq 1248(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $19, %ecx
    leaq 1144(%rsp), %rdi
    leaq 1144(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 1040(%rsp), %rdi
    leaq 1040(%rsp), %rsi
    leaq 1144(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $18, %ecx
    leaq 1040(%rsp), %rdi
    leaq 1040(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 936(%rsp), %rdi
    leaq 936(%rsp), %rsi
    leaq 1040(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $15, %ecx
    leaq 936(%rsp), %rdi
    leaq 936(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 832(%rsp), %rdi
    leaq 832(%rsp), %rsi
    leaq 936(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $18, %ecx
    leaq 832(%rsp), %rdi
    leaq 832(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 728(%rsp), %rdi
    leaq 728(%rsp), %rsi
    leaq 832(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $14, %ecx
    leaq 728(%rsp), %rdi
    leaq 728(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 624(%rsp), %rdi
    leaq 624(%rsp), %rsi
    leaq 728(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $19, %ecx
    leaq 624(%rsp), %rdi
    leaq 624(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 520(%rsp), %rdi
    leaq 520(%rsp), %rsi
    leaq 624(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $17, %ecx
    leaq 520(%rsp), %rdi
    leaq 520(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 416(%rsp), %rdi
    leaq 416(%rsp), %rsi
    leaq 520(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $14, %ecx
    leaq 416(%rsp), %rdi
    leaq 416(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    leaq 416(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $22, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 312(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $16, %ecx
    leaq 312(%rsp), %rdi
    leaq 312(%rsp), %rsi
    call fp765_12_nsqr_inplace_nosave_asm
    leaq 208(%rsp), %rdi
    leaq 208(%rsp), %rsi
    leaq 312(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movl $2, %ecx
    leaq 208(%rsp), %rsi
    leaq 208(%rsp), %rdi
    call fp765_12_nsqr_inplace_nosave_asm
    movq 3328(%rsp), %rdi
    leaq 104(%rsp), %rsi
    leaq 208(%rsp), %rdx
    call fp765_12_montmul_nosave_asm
    movq 3328(%rsp), %rdi
    movq $0, 96(%rdi)
    addq $3336, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

    .section .note.GNU-stack,"",@progbits
