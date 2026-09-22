#ifndef ZUD1280_rc12
#define ZUD1280_rc12 0x0000000000000058
#endif
#ifndef ZUD1280_rc11
#define ZUD1280_rc11 0x0000000000000038
#endif
#ifndef ZUD1280_rc10
#define ZUD1280_rc10 0x00000000000003c0
#endif
#ifndef ZUD1280_rc9
#define ZUD1280_rc9 0x00000000000000d0
#endif
#ifndef ZUD1280_rc8
#define ZUD1280_rc8 0x0000000000000120
#endif
#ifndef ZUD1280_rc7
#define ZUD1280_rc7 0x0000000000000014
#endif
#ifndef ZUD1280_rc6
#define ZUD1280_rc6 0x0000000000000060
#endif
#ifndef ZUD1280_rc5
#define ZUD1280_rc5 0x000000000000002c
#endif
#ifndef ZUD1280_rc4
#define ZUD1280_rc4 0x0000000000000380
#endif
#ifndef ZUD1280_rc3
#define ZUD1280_rc3 0x00000000000000f0
#endif
#ifndef ZUD1280_rc2
#define ZUD1280_rc2 0x00000000000001a0
#endif
#ifndef ZUD1280_rc1
#define ZUD1280_rc1 0x0000000000000012
#endif

.section .note.GNU-stack,"",@progbits
.text

.macro ZUD_VZERO
#ifdef ZUD1280_VZEROUPPER
    vzeroupper
#endif
.endm

.macro ZUD_CHI
    vmovdqa %ymm0, %ymm5
    vmovdqa %ymm1, %ymm6
    vpandn %ymm2, %ymm1, %ymm7
    vpxor %ymm7, %ymm0, %ymm0
    vpandn %ymm3, %ymm2, %ymm8
    vpxor %ymm8, %ymm1, %ymm1
    vpandn %ymm4, %ymm3, %ymm9
    vpxor %ymm9, %ymm2, %ymm2
    vpandn %ymm5, %ymm4, %ymm10
    vpxor %ymm10, %ymm3, %ymm3
    # keep %ymm11 for DM feed-forward save; reuse %ymm7 as temporary
    vpandn %ymm6, %ymm5, %ymm7
    vpxor %ymm7, %ymm4, %ymm4
.endm

.macro ZUD_RHO
    vpermq $0x4e, %ymm4, %ymm4
    vpermq $0x39, %ymm2, %ymm2
    vpaddq %ymm3, %ymm3, %ymm6
    vpermq $0x93, %ymm1, %ymm1
    vpsllq $3, %ymm4, %ymm5
    vpsrlq $61, %ymm4, %ymm4
    vpsllq $5, %ymm1, %ymm8
    vpsrlq $59, %ymm1, %ymm1
    vpshufb zud_rol24_mask(%rip), %ymm2, %ymm2
    vpsrlq $63, %ymm3, %ymm3
    vpor %ymm5, %ymm4, %ymm4
    vpor %ymm8, %ymm1, %ymm1
    vpor %ymm6, %ymm3, %ymm3
.endm

.macro ZUD_THETA_E
    vpxor %ymm1, %ymm0, %ymm5
    vpxor %ymm3, %ymm2, %ymm6
    vpxor %ymm4, %ymm5, %ymm5
    vpxor %ymm6, %ymm5, %ymm5
    vpsllq $5, %ymm5, %ymm7
    vpsllq $14, %ymm5, %ymm9
    vpsrlq $59, %ymm5, %ymm8
    vpsrlq $50, %ymm5, %ymm10
    vpxor %ymm8, %ymm7, %ymm7
    vpxor %ymm10, %ymm9, %ymm9
    vpxor %ymm9, %ymm7, %ymm7
    vpermq $0x93, %ymm7, %ymm7
.endm

.macro ZUD_THETA_X
    vpxor %ymm7, %ymm0, %ymm0
    vpxor %ymm7, %ymm1, %ymm1
    vpxor %ymm7, %ymm2, %ymm2
    vpxor %ymm7, %ymm3, %ymm3
    vpxor %ymm7, %ymm4, %ymm4
.endm

.macro ZUD_THETA
    ZUD_THETA_E
    ZUD_THETA_X
.endm

.macro ZUD_ROUND off
    vpxor zud1280_rcvec+\off(%rip), %ymm0, %ymm0
    ZUD_CHI
    ZUD_RHO
    ZUD_THETA
    ZUD_RHO
.endm

.macro ZUD_ROUND_NEXT off
    ZUD_CHI
    ZUD_RHO
    ZUD_THETA_E
    vpxor zud1280_rcvec+\off(%rip), %ymm0, %ymm0
    ZUD_THETA_X
    ZUD_RHO
.endm

.macro ZUD_ROUND_LAST
    ZUD_CHI
    ZUD_RHO
    ZUD_THETA
    ZUD_RHO
.endm

.macro ZUD_PERM12_BODY
    vpxor zud1280_rcvec+0(%rip), %ymm0, %ymm0
    ZUD_ROUND_NEXT 32
    ZUD_ROUND_NEXT 64
    ZUD_ROUND_NEXT 96
    ZUD_ROUND_NEXT 128
    ZUD_ROUND_NEXT 160
    ZUD_ROUND_NEXT 192
    ZUD_ROUND_NEXT 224
    ZUD_ROUND_NEXT 256
    ZUD_ROUND_NEXT 288
    ZUD_ROUND_NEXT 320
    ZUD_ROUND_NEXT 352
    ZUD_ROUND_LAST
.endm

.macro ZUD_PERM6_BODY
    vpxor zud1280_rcvec+192(%rip), %ymm0, %ymm0
    ZUD_ROUND_NEXT 224
    ZUD_ROUND_NEXT 256
    ZUD_ROUND_NEXT 288
    ZUD_ROUND_NEXT 320
    ZUD_ROUND_NEXT 352
    ZUD_ROUND_LAST
.endm

.macro ZUD_SAVE_FF
    vmovdqa %ymm0, %ymm11
    vmovdqa %ymm1, %ymm12
    vmovdqa %ymm2, %ymm13
    vmovdqa %ymm3, %ymm14
    vmovdqa %ymm4, %ymm15
.endm

.macro ZUD_XOR_FF
    vpxor %ymm11, %ymm0, %ymm0
    vpxor %ymm12, %ymm1, %ymm1
    vpxor %ymm13, %ymm2, %ymm2
    vpxor %ymm14, %ymm3, %ymm3
    vpxor %ymm15, %ymm4, %ymm4
.endm

.macro ZUD_LOAD_STATE
    vmovdqu 0(%rdi), %ymm0
    vmovdqu 32(%rdi), %ymm1
    vmovdqu 64(%rdi), %ymm2
    vmovdqu 96(%rdi), %ymm3
    vmovdqu 128(%rdi), %ymm4
.endm

.macro ZUD_STORE_STATE
    vmovdqu %ymm0, 0(%rdi)
    vmovdqu %ymm1, 32(%rdi)
    vmovdqu %ymm2, 64(%rdi)
    vmovdqu %ymm3, 96(%rdi)
    vmovdqu %ymm4, 128(%rdi)
.endm

.macro ZUD_ADD_BLOCK rows
.if \rows >= 1
    vpxor 0(%rdx), %ymm0, %ymm0
.endif
.if \rows >= 2
    vpxor 32(%rdx), %ymm1, %ymm1
.endif
.if \rows >= 3
    vpxor 64(%rdx), %ymm2, %ymm2
.endif
.if \rows >= 4
    vpxor 96(%rdx), %ymm3, %ymm3
.endif
.if \rows >= 5
    vpxor 128(%rdx), %ymm4, %ymm4
.endif
.endm

.macro ZUD_ADD_BLOCK10
    vpxor 0(%rdx), %ymm0, %ymm0
    vpxor 32(%rdx), %ymm1, %ymm1
    vmovdqu 64(%rdx), %xmm5
    vpxor %ymm5, %ymm2, %ymm2
.endm

.macro ZUD_ADD_BLOCK11
    vpxor 0(%rdx), %ymm0, %ymm0
    vpxor 32(%rdx), %ymm1, %ymm1
    vmovdqu 64(%rdx), %xmm5
    vmovq 80(%rdx), %xmm6
    vinserti128 $1, %xmm6, %ymm5, %ymm5
    vpxor %ymm5, %ymm2, %ymm2
.endm

.macro ZUD_ADD_BLOCK11_FAST
    vpxor 0(%rdx), %ymm0, %ymm0
    vpxor 32(%rdx), %ymm1, %ymm1
    vmovdqu 64(%rdx), %ymm5
    vpand zud_lane11_mask(%rip), %ymm5, %ymm5
    vpxor %ymm5, %ymm2, %ymm2
.endm

.globl ZuD1280_avx2_Initialize
.globl _ZuD1280_avx2_Initialize
.type ZuD1280_avx2_Initialize,@function
.balign 32
ZuD1280_avx2_Initialize:
_ZuD1280_avx2_Initialize:
    vpxor %ymm0, %ymm0, %ymm0
    vmovdqu %ymm0, 0(%rdi)
    vmovdqu %ymm0, 32(%rdi)
    vmovdqu %ymm0, 64(%rdi)
    vmovdqu %ymm0, 96(%rdi)
    vmovdqu %ymm0, 128(%rdi)
    ZUD_VZERO
    ret
.size ZuD1280_avx2_Initialize,.-ZuD1280_avx2_Initialize

.globl ZuD1280_avx2_AddBytes
.globl _ZuD1280_avx2_AddBytes
.type ZuD1280_avx2_AddBytes,@function
.balign 32
ZuD1280_avx2_AddBytes:
_ZuD1280_avx2_AddBytes:
    test %ecx, %ecx
    jz .Ladd_done
    lea (%rdi,%rdx), %rdi
    mov %ecx, %r8d
    cmp $32, %r8d
    jb .Ladd_q
.Ladd_y:
    vmovdqu (%rsi), %ymm5
    vpxor (%rdi), %ymm5, %ymm5
    vmovdqu %ymm5, (%rdi)
    add $32, %rsi
    add $32, %rdi
    sub $32, %r8d
    cmp $32, %r8d
    jae .Ladd_y
.Ladd_q:
    cmp $8, %r8d
    jb .Ladd_b
.Ladd_q_loop:
    mov (%rsi), %rax
    xor %rax, (%rdi)
    add $8, %rsi
    add $8, %rdi
    sub $8, %r8d
    cmp $8, %r8d
    jae .Ladd_q_loop
.Ladd_b:
    test %r8d, %r8d
    jz .Ladd_done
.Ladd_b_loop:
    movzbl (%rsi), %eax
    xorb %al, (%rdi)
    inc %rsi
    inc %rdi
    dec %r8d
    jnz .Ladd_b_loop
.Ladd_done:
    ret
.size ZuD1280_avx2_AddBytes,.-ZuD1280_avx2_AddBytes

.globl ZuD1280_avx2_OverwriteBytes
.globl _ZuD1280_avx2_OverwriteBytes
.type ZuD1280_avx2_OverwriteBytes,@function
.balign 32
ZuD1280_avx2_OverwriteBytes:
_ZuD1280_avx2_OverwriteBytes:
    test %ecx, %ecx
    jz .Lovw_done
    lea (%rdi,%rdx), %rdi
    rep movsb
.Lovw_done:
    ret
.size ZuD1280_avx2_OverwriteBytes,.-ZuD1280_avx2_OverwriteBytes

.globl ZuD1280_avx2_OverwriteWithZeroes
.globl _ZuD1280_avx2_OverwriteWithZeroes
.type ZuD1280_avx2_OverwriteWithZeroes,@function
.balign 32
ZuD1280_avx2_OverwriteWithZeroes:
_ZuD1280_avx2_OverwriteWithZeroes:
    mov %esi, %ecx
    test %ecx, %ecx
    jz .Lzero_done
    cmp $32, %ecx
    jb .Lzero_b
    vpxor %ymm0, %ymm0, %ymm0
.Lzero_y:
    vmovdqu %ymm0, (%rdi)
    add $32, %rdi
    sub $32, %ecx
    cmp $32, %ecx
    jae .Lzero_y
.Lzero_b:
    test %ecx, %ecx
    jz .Lzero_done
    xor %eax, %eax
    rep stosb
.Lzero_done:
    ret
.size ZuD1280_avx2_OverwriteWithZeroes,.-ZuD1280_avx2_OverwriteWithZeroes

.globl ZuD1280_avx2_ExtractBytes
.globl _ZuD1280_avx2_ExtractBytes
.type ZuD1280_avx2_ExtractBytes,@function
.balign 32
ZuD1280_avx2_ExtractBytes:
_ZuD1280_avx2_ExtractBytes:
    test %ecx, %ecx
    jz .Lext_done
    lea (%rdi,%rdx), %rax
    mov %rsi, %rdi
    mov %rax, %rsi
    rep movsb
.Lext_done:
    ret
.size ZuD1280_avx2_ExtractBytes,.-ZuD1280_avx2_ExtractBytes

.globl ZuD1280_avx2_ExtractAndAddBytes
.globl _ZuD1280_avx2_ExtractAndAddBytes
.type ZuD1280_avx2_ExtractAndAddBytes,@function
.balign 32
ZuD1280_avx2_ExtractAndAddBytes:
_ZuD1280_avx2_ExtractAndAddBytes:
    test %r8d, %r8d
    jz .Lexa_done
    lea (%rdi,%rcx), %rdi
    mov %r8d, %r9d
    cmp $32, %r9d
    jb .Lexa_q
.Lexa_y:
    vmovdqu (%rdi), %ymm5
    vpxor (%rsi), %ymm5, %ymm5
    vmovdqu %ymm5, (%rdx)
    add $32, %rdi
    add $32, %rsi
    add $32, %rdx
    sub $32, %r9d
    cmp $32, %r9d
    jae .Lexa_y
.Lexa_q:
    cmp $8, %r9d
    jb .Lexa_b
.Lexa_q_loop:
    mov (%rdi), %rax
    xor (%rsi), %rax
    mov %rax, (%rdx)
    add $8, %rdi
    add $8, %rsi
    add $8, %rdx
    sub $8, %r9d
    cmp $8, %r9d
    jae .Lexa_q_loop
.Lexa_b:
    test %r9d, %r9d
    jz .Lexa_done
.Lexa_b_loop:
    movzbl (%rdi), %eax
    xorb (%rsi), %al
    movb %al, (%rdx)
    inc %rdi
    inc %rsi
    inc %rdx
    dec %r9d
    jnz .Lexa_b_loop
.Lexa_done:
    ret
.size ZuD1280_avx2_ExtractAndAddBytes,.-ZuD1280_avx2_ExtractAndAddBytes

.globl ZuD1280_avx2_Permute_12rounds
.globl _ZuD1280_avx2_Permute_12rounds
.type ZuD1280_avx2_Permute_12rounds,@function
.balign 32
ZuD1280_avx2_Permute_12rounds:
_ZuD1280_avx2_Permute_12rounds:
    ZUD_LOAD_STATE
    ZUD_PERM12_BODY
    ZUD_STORE_STATE
    ZUD_VZERO
    ret
.size ZuD1280_avx2_Permute_12rounds,.-ZuD1280_avx2_Permute_12rounds

.globl ZuD1280_avx2_Permute_6rounds
.globl _ZuD1280_avx2_Permute_6rounds
.type ZuD1280_avx2_Permute_6rounds,@function
.balign 32
ZuD1280_avx2_Permute_6rounds:
_ZuD1280_avx2_Permute_6rounds:
    ZUD_LOAD_STATE
    ZUD_PERM6_BODY
    ZUD_STORE_STATE
    ZUD_VZERO
    ret
.size ZuD1280_avx2_Permute_6rounds,.-ZuD1280_avx2_Permute_6rounds

.globl ZuD1280_avx2_Permute_Nrounds
.globl _ZuD1280_avx2_Permute_Nrounds
.type ZuD1280_avx2_Permute_Nrounds,@function
.balign 32
ZuD1280_avx2_Permute_Nrounds:
_ZuD1280_avx2_Permute_Nrounds:
    mov %esi, %eax
    cmp $12, %eax
    jbe .Ln_clamped
    mov $12, %eax
.Ln_clamped:
    test %eax, %eax
    jz .Ln_return
    ZUD_LOAD_STATE
    lea .Ln_table(%rip), %rdx
    movslq (%rdx,%rax,4), %rax
    add %rdx, %rax
    jmp *%rax
.Ln_12:
    ZUD_ROUND 0
.Ln_11:
    ZUD_ROUND 32
.Ln_10:
    ZUD_ROUND 64
.Ln_9:
    ZUD_ROUND 96
.Ln_8:
    ZUD_ROUND 128
.Ln_7:
    ZUD_ROUND 160
.Ln_6:
    ZUD_ROUND 192
.Ln_5:
    ZUD_ROUND 224
.Ln_4:
    ZUD_ROUND 256
.Ln_3:
    ZUD_ROUND 288
.Ln_2:
    ZUD_ROUND 320
.Ln_1:
    ZUD_ROUND 352
    ZUD_STORE_STATE
    ZUD_VZERO
.Ln_return:
    ret
.balign 4
.Ln_table:
    .long .Ln_return-.Ln_table
    .long .Ln_1-.Ln_table
    .long .Ln_2-.Ln_table
    .long .Ln_3-.Ln_table
    .long .Ln_4-.Ln_table
    .long .Ln_5-.Ln_table
    .long .Ln_6-.Ln_table
    .long .Ln_7-.Ln_table
    .long .Ln_8-.Ln_table
    .long .Ln_9-.Ln_table
    .long .Ln_10-.Ln_table
    .long .Ln_11-.Ln_table
    .long .Ln_12-.Ln_table
.size ZuD1280_avx2_Permute_Nrounds,.-ZuD1280_avx2_Permute_Nrounds

.globl ZuD1280_avx2_FastLoop_Absorb
.globl _ZuD1280_avx2_FastLoop_Absorb
.globl ZuD1280_avx2_12rounds_FastLoop_Absorb
.globl _ZuD1280_avx2_12rounds_FastLoop_Absorb
.globl ZuD1280_12rounds_avx2_FastLoop_Absorb
.globl _ZuD1280_12rounds_avx2_FastLoop_Absorb
.type ZuD1280_avx2_FastLoop_Absorb,@function
.balign 32
ZuD1280_avx2_FastLoop_Absorb:
_ZuD1280_avx2_FastLoop_Absorb:
ZuD1280_avx2_12rounds_FastLoop_Absorb:
_ZuD1280_avx2_12rounds_FastLoop_Absorb:
ZuD1280_12rounds_avx2_FastLoop_Absorb:
_ZuD1280_12rounds_avx2_FastLoop_Absorb:
    cmp $20, %esi
    je .Lfl12_20
    cmp $16, %esi
    je .Lfl12_16
    cmp $12, %esi
    je .Lfl12_12
    cmp $11, %esi
    je .Lfl12_11
    cmp $10, %esi
    je .Lfl12_10
    cmp $8, %esi
    je .Lfl12_8
    xor %eax, %eax
    ret
.Lfl12_20:
    cmp $160, %rcx
    jb .Lfl12_zero
    mov %rdx, %rax
    lea -160(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl12_20_loop:
    ZUD_ADD_BLOCK 5
    ZUD_PERM12_BODY
    add $160, %rdx
    cmp %r10, %rdx
    jbe .Lfl12_20_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl12_16:
    cmp $128, %rcx
    jb .Lfl12_zero
    mov %rdx, %rax
    lea -128(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl12_16_loop:
    ZUD_ADD_BLOCK 4
    ZUD_PERM12_BODY
    add $128, %rdx
    cmp %r10, %rdx
    jbe .Lfl12_16_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl12_12:
    cmp $96, %rcx
    jb .Lfl12_zero
    mov %rdx, %rax
    lea -96(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl12_12_loop:
    ZUD_ADD_BLOCK 3
    ZUD_PERM12_BODY
    add $96, %rdx
    cmp %r10, %rdx
    jbe .Lfl12_12_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl12_11:
    cmp $88, %rcx
    jb .Lfl12_zero
    mov %rdx, %rax
    lea -96(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
    cmp %r10, %rdx
    ja .Lfl12_11_last_check
.Lfl12_11_loop_fast:
    ZUD_ADD_BLOCK11_FAST
    ZUD_PERM12_BODY
    add $88, %rdx
    cmp %r10, %rdx
    jbe .Lfl12_11_loop_fast
.Lfl12_11_last_check:
    lea -88(%rax,%rcx), %r11
    cmp %r11, %rdx
    ja .Lfl12_11_done
.Lfl12_11_last:
    ZUD_ADD_BLOCK11
    ZUD_PERM12_BODY
    add $88, %rdx
    cmp %r11, %rdx
    jbe .Lfl12_11_last
.Lfl12_11_done:
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl12_10:
    cmp $80, %rcx
    jb .Lfl12_zero
    mov %rdx, %rax
    lea -80(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl12_10_loop:
    ZUD_ADD_BLOCK10
    ZUD_PERM12_BODY
    add $80, %rdx
    cmp %r10, %rdx
    jbe .Lfl12_10_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl12_8:
    cmp $64, %rcx
    jb .Lfl12_zero
    mov %rdx, %rax
    lea -64(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl12_8_loop:
    ZUD_ADD_BLOCK 2
    ZUD_PERM12_BODY
    add $64, %rdx
    cmp %r10, %rdx
    jbe .Lfl12_8_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl12_zero:
    xor %eax, %eax
    ret
.size ZuD1280_avx2_FastLoop_Absorb,.-ZuD1280_avx2_FastLoop_Absorb

.globl ZuD1280_avx2_6rounds_FastLoop_Absorb
.globl _ZuD1280_avx2_6rounds_FastLoop_Absorb
.globl ZuD1280_6rounds_avx2_FastLoop_Absorb
.globl _ZuD1280_6rounds_avx2_FastLoop_Absorb
.type ZuD1280_avx2_6rounds_FastLoop_Absorb,@function
.balign 32
ZuD1280_avx2_6rounds_FastLoop_Absorb:
_ZuD1280_avx2_6rounds_FastLoop_Absorb:
ZuD1280_6rounds_avx2_FastLoop_Absorb:
_ZuD1280_6rounds_avx2_FastLoop_Absorb:
    cmp $20, %esi
    je .Lfl6_20
    cmp $16, %esi
    je .Lfl6_16
    cmp $12, %esi
    je .Lfl6_12
    cmp $11, %esi
    je .Lfl6_11
    cmp $10, %esi
    je .Lfl6_10
    cmp $8, %esi
    je .Lfl6_8
    xor %eax, %eax
    ret
.Lfl6_20:
    cmp $160, %rcx
    jb .Lfl6_zero
    mov %rdx, %rax
    lea -160(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl6_20_loop:
    ZUD_ADD_BLOCK 5
    ZUD_PERM6_BODY
    add $160, %rdx
    cmp %r10, %rdx
    jbe .Lfl6_20_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl6_16:
    cmp $128, %rcx
    jb .Lfl6_zero
    mov %rdx, %rax
    lea -128(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl6_16_loop:
    ZUD_ADD_BLOCK 4
    ZUD_PERM6_BODY
    add $128, %rdx
    cmp %r10, %rdx
    jbe .Lfl6_16_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl6_12:
    cmp $96, %rcx
    jb .Lfl6_zero
    mov %rdx, %rax
    lea -96(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl6_12_loop:
    ZUD_ADD_BLOCK 3
    ZUD_PERM6_BODY
    add $96, %rdx
    cmp %r10, %rdx
    jbe .Lfl6_12_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl6_11:
    cmp $88, %rcx
    jb .Lfl6_zero
    mov %rdx, %rax
    lea -96(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
    cmp %r10, %rdx
    ja .Lfl6_11_last_check
.Lfl6_11_loop_fast:
    ZUD_ADD_BLOCK11_FAST
    ZUD_PERM6_BODY
    add $88, %rdx
    cmp %r10, %rdx
    jbe .Lfl6_11_loop_fast
.Lfl6_11_last_check:
    lea -88(%rax,%rcx), %r11
    cmp %r11, %rdx
    ja .Lfl6_11_done
.Lfl6_11_last:
    ZUD_ADD_BLOCK11
    ZUD_PERM6_BODY
    add $88, %rdx
    cmp %r11, %rdx
    jbe .Lfl6_11_last
.Lfl6_11_done:
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl6_10:
    cmp $80, %rcx
    jb .Lfl6_zero
    mov %rdx, %rax
    lea -80(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl6_10_loop:
    ZUD_ADD_BLOCK10
    ZUD_PERM6_BODY
    add $80, %rdx
    cmp %r10, %rdx
    jbe .Lfl6_10_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl6_8:
    cmp $64, %rcx
    jb .Lfl6_zero
    mov %rdx, %rax
    lea -64(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Lfl6_8_loop:
    ZUD_ADD_BLOCK 2
    ZUD_PERM6_BODY
    add $64, %rdx
    cmp %r10, %rdx
    jbe .Lfl6_8_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Lfl6_zero:
    xor %eax, %eax
    ret
.size ZuD1280_avx2_6rounds_FastLoop_Absorb,.-ZuD1280_avx2_6rounds_FastLoop_Absorb

.globl ZuD1280_avx2_DMLoop_Absorb
.globl _ZuD1280_avx2_DMLoop_Absorb
.globl ZuD1280_avx2_12rounds_DMLoop_Absorb
.globl _ZuD1280_avx2_12rounds_DMLoop_Absorb
.globl ZuD1280_12rounds_avx2_DMLoop_Absorb
.globl _ZuD1280_12rounds_avx2_DMLoop_Absorb
.type ZuD1280_avx2_DMLoop_Absorb,@function
.balign 32
ZuD1280_avx2_DMLoop_Absorb:
_ZuD1280_avx2_DMLoop_Absorb:
ZuD1280_avx2_12rounds_DMLoop_Absorb:
_ZuD1280_avx2_12rounds_DMLoop_Absorb:
ZuD1280_12rounds_avx2_DMLoop_Absorb:
_ZuD1280_12rounds_avx2_DMLoop_Absorb:
    cmp $20, %esi
    je .Ldm12_20
    cmp $16, %esi
    je .Ldm12_16
    cmp $12, %esi
    je .Ldm12_12
    cmp $11, %esi
    je .Ldm12_11
    cmp $10, %esi
    je .Ldm12_10
    cmp $8, %esi
    je .Ldm12_8
    xor %eax, %eax
    ret
.Ldm12_20:
    cmp $160, %rcx
    jb .Ldm12_zero
    mov %rdx, %rax
    lea -160(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm12_20_loop:
    ZUD_ADD_BLOCK 5
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $160, %rdx
    cmp %r10, %rdx
    jbe .Ldm12_20_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm12_16:
    cmp $128, %rcx
    jb .Ldm12_zero
    mov %rdx, %rax
    lea -128(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm12_16_loop:
    ZUD_ADD_BLOCK 4
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $128, %rdx
    cmp %r10, %rdx
    jbe .Ldm12_16_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm12_12:
    cmp $96, %rcx
    jb .Ldm12_zero
    mov %rdx, %rax
    lea -96(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm12_12_loop:
    ZUD_ADD_BLOCK 3
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $96, %rdx
    cmp %r10, %rdx
    jbe .Ldm12_12_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm12_11:
    cmp $88, %rcx
    jb .Ldm12_zero
    mov %rdx, %rax
    lea -96(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
    cmp %r10, %rdx
    ja .Ldm12_11_last_check
.Ldm12_11_loop_fast:
    ZUD_ADD_BLOCK11_FAST
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $88, %rdx
    cmp %r10, %rdx
    jbe .Ldm12_11_loop_fast
.Ldm12_11_last_check:
    lea -88(%rax,%rcx), %r11
    cmp %r11, %rdx
    ja .Ldm12_11_done
.Ldm12_11_last:
    ZUD_ADD_BLOCK11
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $88, %rdx
    cmp %r11, %rdx
    jbe .Ldm12_11_last
.Ldm12_11_done:
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm12_10:
    cmp $80, %rcx
    jb .Ldm12_zero
    mov %rdx, %rax
    lea -80(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm12_10_loop:
    ZUD_ADD_BLOCK10
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $80, %rdx
    cmp %r10, %rdx
    jbe .Ldm12_10_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm12_8:
    cmp $64, %rcx
    jb .Ldm12_zero
    mov %rdx, %rax
    lea -64(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm12_8_loop:
    ZUD_ADD_BLOCK 2
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $64, %rdx
    cmp %r10, %rdx
    jbe .Ldm12_8_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm12_zero:
    xor %eax, %eax
    ret
.size ZuD1280_avx2_DMLoop_Absorb,.-ZuD1280_avx2_DMLoop_Absorb

.globl ZuD1280_avx2_6rounds_DMLoop_Absorb
.globl _ZuD1280_avx2_6rounds_DMLoop_Absorb
.globl ZuD1280_6rounds_avx2_DMLoop_Absorb
.globl _ZuD1280_6rounds_avx2_DMLoop_Absorb
.type ZuD1280_avx2_6rounds_DMLoop_Absorb,@function
.balign 32
ZuD1280_avx2_6rounds_DMLoop_Absorb:
_ZuD1280_avx2_6rounds_DMLoop_Absorb:
ZuD1280_6rounds_avx2_DMLoop_Absorb:
_ZuD1280_6rounds_avx2_DMLoop_Absorb:
    cmp $20, %esi
    je .Ldm6_20
    cmp $16, %esi
    je .Ldm6_16
    cmp $12, %esi
    je .Ldm6_12
    cmp $11, %esi
    je .Ldm6_11
    cmp $10, %esi
    je .Ldm6_10
    cmp $8, %esi
    je .Ldm6_8
    xor %eax, %eax
    ret
.Ldm6_20:
    cmp $160, %rcx
    jb .Ldm6_zero
    mov %rdx, %rax
    lea -160(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm6_20_loop:
    ZUD_ADD_BLOCK 5
    ZUD_SAVE_FF
    ZUD_PERM6_BODY
    ZUD_XOR_FF
    add $160, %rdx
    cmp %r10, %rdx
    jbe .Ldm6_20_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm6_16:
    cmp $128, %rcx
    jb .Ldm6_zero
    mov %rdx, %rax
    lea -128(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm6_16_loop:
    ZUD_ADD_BLOCK 4
    ZUD_SAVE_FF
    ZUD_PERM6_BODY
    ZUD_XOR_FF
    add $128, %rdx
    cmp %r10, %rdx
    jbe .Ldm6_16_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm6_12:
    cmp $96, %rcx
    jb .Ldm6_zero
    mov %rdx, %rax
    lea -96(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm6_12_loop:
    ZUD_ADD_BLOCK 3
    ZUD_SAVE_FF
    ZUD_PERM6_BODY
    ZUD_XOR_FF
    add $96, %rdx
    cmp %r10, %rdx
    jbe .Ldm6_12_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm6_11:
    cmp $88, %rcx
    jb .Ldm6_zero
    mov %rdx, %rax
    lea -96(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
    cmp %r10, %rdx
    ja .Ldm6_11_last_check
.Ldm6_11_loop_fast:
    ZUD_ADD_BLOCK11_FAST
    ZUD_SAVE_FF
    ZUD_PERM6_BODY
    ZUD_XOR_FF
    add $88, %rdx
    cmp %r10, %rdx
    jbe .Ldm6_11_loop_fast
.Ldm6_11_last_check:
    lea -88(%rax,%rcx), %r11
    cmp %r11, %rdx
    ja .Ldm6_11_done
.Ldm6_11_last:
    ZUD_ADD_BLOCK11
    ZUD_SAVE_FF
    ZUD_PERM6_BODY
    ZUD_XOR_FF
    add $88, %rdx
    cmp %r11, %rdx
    jbe .Ldm6_11_last
.Ldm6_11_done:
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm6_10:
    cmp $80, %rcx
    jb .Ldm6_zero
    mov %rdx, %rax
    lea -80(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm6_10_loop:
    ZUD_ADD_BLOCK10
    ZUD_SAVE_FF
    ZUD_PERM6_BODY
    ZUD_XOR_FF
    add $80, %rdx
    cmp %r10, %rdx
    jbe .Ldm6_10_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm6_8:
    cmp $64, %rcx
    jb .Ldm6_zero
    mov %rdx, %rax
    lea -64(%rdx,%rcx), %r10
    ZUD_LOAD_STATE
.Ldm6_8_loop:
    ZUD_ADD_BLOCK 2
    ZUD_SAVE_FF
    ZUD_PERM6_BODY
    ZUD_XOR_FF
    add $64, %rdx
    cmp %r10, %rdx
    jbe .Ldm6_8_loop
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm6_zero:
    xor %eax, %eax
    ret
.size ZuD1280_avx2_6rounds_DMLoop_Absorb,.-ZuD1280_avx2_6rounds_DMLoop_Absorb

.globl ZuD1280_avx2_DMLoop11_Absorb
.globl _ZuD1280_avx2_DMLoop11_Absorb
.globl ZuD1280_avx2_DMLoop11_Absorb_safe
.globl _ZuD1280_avx2_DMLoop11_Absorb_safe
.type ZuD1280_avx2_DMLoop11_Absorb,@function
.type ZuD1280_avx2_DMLoop11_Absorb_safe,@function
.balign 32
ZuD1280_avx2_DMLoop11_Absorb:
_ZuD1280_avx2_DMLoop11_Absorb:
ZuD1280_avx2_DMLoop11_Absorb_safe:
_ZuD1280_avx2_DMLoop11_Absorb_safe:
    cmp $88, %rdx
    jb .Ldm11s_zero
    mov %rsi, %rax
    mov %rdx, %rcx
    mov %rsi, %rdx
    lea -96(%rax,%rcx), %r10
    ZUD_LOAD_STATE
    cmp %r10, %rdx
    ja .Ldm11s_last_check
.Ldm11s_loop_fast:
    ZUD_ADD_BLOCK11_FAST
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $88, %rdx
    cmp %r10, %rdx
    jbe .Ldm11s_loop_fast
.Ldm11s_last_check:
    lea -88(%rax,%rcx), %r11
    cmp %r11, %rdx
    ja .Ldm11s_done
.Ldm11s_last:
    ZUD_ADD_BLOCK11
    ZUD_SAVE_FF
    ZUD_PERM12_BODY
    ZUD_XOR_FF
    add $88, %rdx
    cmp %r11, %rdx
    jbe .Ldm11s_last
.Ldm11s_done:
    ZUD_STORE_STATE
    ZUD_VZERO
    sub %rax, %rdx
    mov %rdx, %rax
    ret
.Ldm11s_zero:
    xor %eax, %eax
    ret
.size ZuD1280_avx2_DMLoop11_Absorb,.-ZuD1280_avx2_DMLoop11_Absorb
.size ZuD1280_avx2_DMLoop11_Absorb_safe,.-ZuD1280_avx2_DMLoop11_Absorb_safe

.section .rodata
.balign 32
zud_rol24_mask:
    .byte 5,6,7,0,1,2,3,4,13,14,15,8,9,10,11,12
    .byte 5,6,7,0,1,2,3,4,13,14,15,8,9,10,11,12

.balign 32
zud_lane11_mask:
    .quad 0xffffffffffffffff,0xffffffffffffffff,0xffffffffffffffff,0x0000000000000000

.balign 32
zud1280_rcvec:
    .quad ZUD1280_rc12,0,0,0
    .quad ZUD1280_rc11,0,0,0
    .quad ZUD1280_rc10,0,0,0
    .quad ZUD1280_rc9,0,0,0
    .quad ZUD1280_rc8,0,0,0
    .quad ZUD1280_rc7,0,0,0
    .quad ZUD1280_rc6,0,0,0
    .quad ZUD1280_rc5,0,0,0
    .quad ZUD1280_rc4,0,0,0
    .quad ZUD1280_rc3,0,0,0
    .quad ZUD1280_rc2,0,0,0
    .quad ZUD1280_rc1,0,0,0