#ifndef ZUD1536_rc12
#define ZUD1536_rc12 0x0000000000000058
#endif
#ifndef ZUD1536_rc11
#define ZUD1536_rc11 0x0000000000000038
#endif
#ifndef ZUD1536_rc10
#define ZUD1536_rc10 0x00000000000003c0
#endif
#ifndef ZUD1536_rc9
#define ZUD1536_rc9 0x00000000000000d0
#endif
#ifndef ZUD1536_rc8
#define ZUD1536_rc8 0x0000000000000120
#endif
#ifndef ZUD1536_rc7
#define ZUD1536_rc7 0x0000000000000014
#endif
#ifndef ZUD1536_rc6
#define ZUD1536_rc6 0x0000000000000060
#endif
#ifndef ZUD1536_rc5
#define ZUD1536_rc5 0x000000000000002c
#endif
#ifndef ZUD1536_rc4
#define ZUD1536_rc4 0x0000000000000380
#endif
#ifndef ZUD1536_rc3
#define ZUD1536_rc3 0x00000000000000f0
#endif
#ifndef ZUD1536_rc2
#define ZUD1536_rc2 0x00000000000001a0
#endif
#ifndef ZUD1536_rc1
#define ZUD1536_rc1 0x0000000000000012
#endif

.section .note.GNU-stack,"",@progbits
.text

.macro Z1536_VZERO
#ifdef ZUD1536_VZEROUPPER
    vzeroupper
#endif
.endm

.macro Z1536_CHI3 lo0, hi0, lo1, hi1, lo2, hi2
    vpandn \lo2, \lo1, %ymm12
    vpxor %ymm12, \lo0, \lo0
    vpandn \lo0, \lo2, %ymm12
    vpxor %ymm12, \lo1, \lo1
    vpandn \lo1, \lo0, %ymm12
    vpxor %ymm12, \lo2, \lo2
    vpandn \hi2, \hi1, %ymm12
    vpxor %ymm12, \hi0, \hi0
    vpandn \hi0, \hi2, %ymm12
    vpxor %ymm12, \hi1, \hi1
    vpandn \hi1, \hi0, %ymm12
    vpxor %ymm12, \hi2, \hi2
.endm

.macro Z1536_RHO
    vpermq $0x93, %ymm4, %ymm12
    vpermq $0x93, %ymm5, %ymm13
    vblendpd $0x0e, %ymm13, %ymm12, %ymm14
    vblendpd $0x0e, %ymm12, %ymm13, %ymm5
    vmovdqa %ymm14, %ymm4
    vpermq $0x39, %ymm2, %ymm12
    vpermq $0x39, %ymm3, %ymm13
    vblendpd $0x07, %ymm13, %ymm12, %ymm14
    vblendpd $0x07, %ymm12, %ymm13, %ymm15
    vpsllq $3, %ymm14, %ymm12
    vpsrlq $61, %ymm14, %ymm14
    vpor %ymm12, %ymm14, %ymm2
    vpsllq $3, %ymm15, %ymm12
    vpsrlq $61, %ymm15, %ymm15
    vpor %ymm12, %ymm15, %ymm3
.endm

.macro Z1536_THETA
    vpxor %ymm2, %ymm0, %ymm12
    vpxor %ymm4, %ymm12, %ymm12
    vpxor %ymm3, %ymm1, %ymm13
    vpxor %ymm5, %ymm13, %ymm13
    vpermq $0x93, %ymm12, %ymm14
    vpermq $0x93, %ymm13, %ymm15
    vblendpd $0x0e, %ymm14, %ymm15, %ymm12
    vblendpd $0x0e, %ymm15, %ymm14, %ymm13
    vpsllq $20, %ymm12, %ymm14
    vpsrlq $44, %ymm12, %ymm15
    vpor %ymm15, %ymm14, %ymm14
    vpshufb %ymm11, %ymm12, %ymm12
    vpxor %ymm14, %ymm12, %ymm12
    vpsllq $20, %ymm13, %ymm14
    vpsrlq $44, %ymm13, %ymm15
    vpor %ymm15, %ymm14, %ymm14
    vpshufb %ymm11, %ymm13, %ymm13
    vpxor %ymm14, %ymm13, %ymm13
    vpxor %ymm12, %ymm0, %ymm0
    vpxor %ymm13, %ymm1, %ymm1
    vpxor %ymm12, %ymm2, %ymm2
    vpxor %ymm13, %ymm3, %ymm3
    vpxor %ymm12, %ymm4, %ymm4
    vpxor %ymm13, %ymm5, %ymm5
.endm

.macro Z1536_ROUND rcidx
    vpxor zud1536_rcv_\rcidx(%rip), %ymm0, %ymm0
    Z1536_CHI3 %ymm0, %ymm1, %ymm2, %ymm3, %ymm4, %ymm5
    Z1536_RHO
    Z1536_THETA
    Z1536_RHO
.endm

.macro Z1536_ROUND_PTR
    vpxor (%r8), %ymm0, %ymm0
    Z1536_CHI3 %ymm0, %ymm1, %ymm2, %ymm3, %ymm4, %ymm5
    Z1536_RHO
    Z1536_THETA
    Z1536_RHO
.endm

.macro Z1536_LOAD_STATE ptr
    vmovdqu 0*32(\ptr), %ymm0
    vmovdqu 1*32(\ptr), %ymm1
    vmovdqu 2*32(\ptr), %ymm2
    vmovdqu 3*32(\ptr), %ymm3
    vmovdqu 4*32(\ptr), %ymm4
    vmovdqu 5*32(\ptr), %ymm5
.endm

.macro Z1536_STORE_STATE ptr
    vmovdqu %ymm0, 0*32(\ptr)
    vmovdqu %ymm1, 1*32(\ptr)
    vmovdqu %ymm2, 2*32(\ptr)
    vmovdqu %ymm3, 3*32(\ptr)
    vmovdqu %ymm4, 4*32(\ptr)
    vmovdqu %ymm5, 5*32(\ptr)
.endm

.macro Z1536_PERM12
    Z1536_ROUND 12
    Z1536_ROUND 11
    Z1536_ROUND 10
    Z1536_ROUND 9
    Z1536_ROUND 8
    Z1536_ROUND 7
    Z1536_ROUND 6
    Z1536_ROUND 5
    Z1536_ROUND 4
    Z1536_ROUND 3
    Z1536_ROUND 2
    Z1536_ROUND 1
.endm

.macro Z1536_PERM6
    Z1536_ROUND 6
    Z1536_ROUND 5
    Z1536_ROUND 4
    Z1536_ROUND 3
    Z1536_ROUND 2
    Z1536_ROUND 1
.endm

.macro Z1536_LOAD24 off, dst
    vmovdqu \off(%r8), %xmm12
    vmovq \off+16(%r8), %xmm13
    vinserti128 $1, %xmm13, %ymm12, \dst
.endm

.macro Z1536_SAVE_FF
    vmovdqa %ymm0, %ymm6
    vmovdqa %ymm1, %ymm7
    vmovdqa %ymm2, %ymm8
    vmovdqa %ymm3, %ymm9
    vmovdqa %ymm4, %ymm10
    vmovdqu %ymm5, (%rsp)
.endm

.macro Z1536_XOR_FF
    vpxor %ymm6, %ymm0, %ymm0
    vpxor %ymm7, %ymm1, %ymm1
    vpxor %ymm8, %ymm2, %ymm2
    vpxor %ymm9, %ymm3, %ymm3
    vpxor %ymm10, %ymm4, %ymm4
    vpxor (%rsp), %ymm5, %ymm5
.endm

.globl ZuD1536_avx2_Initialize
.type ZuD1536_avx2_Initialize,@function
.balign 32
ZuD1536_avx2_Initialize:
    vpxor %ymm0, %ymm0, %ymm0
    vmovdqu %ymm0, 0*32(%rdi)
    vmovdqu %ymm0, 1*32(%rdi)
    vmovdqu %ymm0, 2*32(%rdi)
    vmovdqu %ymm0, 3*32(%rdi)
    vmovdqu %ymm0, 4*32(%rdi)
    vmovdqu %ymm0, 5*32(%rdi)
    Z1536_VZERO
    ret
.size ZuD1536_avx2_Initialize,.-ZuD1536_avx2_Initialize

.globl ZuD1536_avx2_AddBytes
.type ZuD1536_avx2_AddBytes,@function
.balign 32
ZuD1536_avx2_AddBytes:
    test %ecx, %ecx
    jz .Ladd_done
    lea (%rdi,%rdx), %rax
.Ladd_loop:
    movzbl (%rsi), %r8d
    xorb %r8b, (%rax)
    inc %rsi
    inc %rax
    dec %ecx
    jnz .Ladd_loop
.Ladd_done:
    ret
.size ZuD1536_avx2_AddBytes,.-ZuD1536_avx2_AddBytes

.globl ZuD1536_avx2_OverwriteBytes
.type ZuD1536_avx2_OverwriteBytes,@function
.balign 32
ZuD1536_avx2_OverwriteBytes:
    lea (%rdi,%rdx), %rdi
    mov %ecx, %ecx
    rep movsb
    ret
.size ZuD1536_avx2_OverwriteBytes,.-ZuD1536_avx2_OverwriteBytes

.globl ZuD1536_avx2_OverwriteWithZeroes
.type ZuD1536_avx2_OverwriteWithZeroes,@function
.balign 32
ZuD1536_avx2_OverwriteWithZeroes:
    mov %esi, %ecx
    xor %eax, %eax
    rep stosb
    ret
.size ZuD1536_avx2_OverwriteWithZeroes,.-ZuD1536_avx2_OverwriteWithZeroes

.globl ZuD1536_avx2_ExtractBytes
.type ZuD1536_avx2_ExtractBytes,@function
.balign 32
ZuD1536_avx2_ExtractBytes:
    lea (%rdi,%rdx), %r8
    mov %rsi, %rdi
    mov %r8, %rsi
    mov %ecx, %ecx
    rep movsb
    ret
.size ZuD1536_avx2_ExtractBytes,.-ZuD1536_avx2_ExtractBytes

.globl ZuD1536_avx2_ExtractAndAddBytes
.type ZuD1536_avx2_ExtractAndAddBytes,@function
.balign 32
ZuD1536_avx2_ExtractAndAddBytes:
    test %r8d, %r8d
    jz .Leaa_done
    lea (%rdi,%rcx), %rdi
.Leaa_loop:
    movzbl (%rdi), %r9d
    xorb (%rsi), %r9b
    mov %r9b, (%rdx)
    inc %rdi
    inc %rsi
    inc %rdx
    dec %r8d
    jnz .Leaa_loop
.Leaa_done:
    ret
.size ZuD1536_avx2_ExtractAndAddBytes,.-ZuD1536_avx2_ExtractAndAddBytes

.globl ZuD1536_avx2_Permute_12rounds
.type ZuD1536_avx2_Permute_12rounds,@function
.balign 32
ZuD1536_avx2_Permute_12rounds:
    Z1536_LOAD_STATE %rdi
    vmovdqa zud1536_rol56_mask(%rip), %ymm11
    Z1536_PERM12
    Z1536_STORE_STATE %rdi
    Z1536_VZERO
    ret
.size ZuD1536_avx2_Permute_12rounds,.-ZuD1536_avx2_Permute_12rounds

.globl ZuD1536_avx2_Permute_6rounds
.type ZuD1536_avx2_Permute_6rounds,@function
.balign 32
ZuD1536_avx2_Permute_6rounds:
    Z1536_LOAD_STATE %rdi
    vmovdqa zud1536_rol56_mask(%rip), %ymm11
    Z1536_PERM6
    Z1536_STORE_STATE %rdi
    Z1536_VZERO
    ret
.size ZuD1536_avx2_Permute_6rounds,.-ZuD1536_avx2_Permute_6rounds

.globl ZuD1536_avx2_Permute_Nrounds
.type ZuD1536_avx2_Permute_Nrounds,@function
.balign 32
ZuD1536_avx2_Permute_Nrounds:
    test %esi, %esi
    jz .Ln_done_empty
    cmp $12, %esi
    je ZuD1536_avx2_Permute_12rounds
    cmp $6, %esi
    je ZuD1536_avx2_Permute_6rounds
    cmp $12, %esi
    cmova .Lnr12(%rip), %esi
    Z1536_LOAD_STATE %rdi
    vmovdqa zud1536_rol56_mask(%rip), %ymm11
    mov $12, %eax
    sub %esi, %eax
    shl $5, %eax
    lea zud1536_rcv_table(%rip), %r8
    add %rax, %r8
.Ln_loop:
    Z1536_ROUND_PTR
    add $32, %r8
    dec %esi
    jnz .Ln_loop
    Z1536_STORE_STATE %rdi
    Z1536_VZERO
.Ln_done_empty:
    ret
.Lnr12:
    .long 12
.size ZuD1536_avx2_Permute_Nrounds,.-ZuD1536_avx2_Permute_Nrounds

.globl ZuD1536_avx2_DMLoop15_Absorb
.globl ZuD1536_avx2_12rounds_DMLoop15_Absorb
.type ZuD1536_avx2_DMLoop15_Absorb,@function
.type ZuD1536_avx2_12rounds_DMLoop15_Absorb,@function
.balign 32
ZuD1536_avx2_12rounds_DMLoop15_Absorb:
ZuD1536_avx2_DMLoop15_Absorb:
    mov %rsi, %r8
    mov %rdx, %rcx
    xor %eax, %eax
    cmp $120, %rcx
    jb .Ldm15_fast_ret
    sub $32, %rsp
    Z1536_LOAD_STATE %rdi
    vmovdqa zud1536_rol56_mask(%rip), %ymm11
.Ldm15_fast_loop:
    vpxor 0(%r8), %ymm0, %ymm0
    vpxor 32(%r8), %ymm1, %ymm1
    vpxor 64(%r8), %ymm2, %ymm2
    Z1536_LOAD24 96, %ymm12
    vpxor %ymm12, %ymm3, %ymm3
    Z1536_SAVE_FF
    Z1536_PERM12
    Z1536_XOR_FF
    add $120, %r8
    add $120, %rax
    sub $120, %rcx
    cmp $120, %rcx
    jae .Ldm15_fast_loop
    Z1536_STORE_STATE %rdi
    add $32, %rsp
    Z1536_VZERO
.Ldm15_fast_ret:
    ret
.size ZuD1536_avx2_DMLoop15_Absorb,.-ZuD1536_avx2_DMLoop15_Absorb

.globl ZuD1536_avx2_DMLoop_Absorb
.globl ZuD1536_avx2_12rounds_DMLoop_Absorb
.type ZuD1536_avx2_DMLoop_Absorb,@function
.type ZuD1536_avx2_12rounds_DMLoop_Absorb,@function
.balign 32
ZuD1536_avx2_12rounds_DMLoop_Absorb:
ZuD1536_avx2_DMLoop_Absorb:
    mov %rdx, %r8
    cmp $15, %esi
    je .Ldm15_entry
    cmp $11, %esi
    je .Ldm11_entry
    cmp $7, %esi
    je .Ldm7_entry
    xor %eax, %eax
    ret
.Ldm15_entry:
    xor %eax, %eax
    cmp $120, %rcx
    jb .Ldm_ret0
    sub $32, %rsp
    Z1536_LOAD_STATE %rdi
    vmovdqa zud1536_rol56_mask(%rip), %ymm11
.Ldm15_loop:
    vpxor 0(%r8), %ymm0, %ymm0
    vpxor 32(%r8), %ymm1, %ymm1
    vpxor 64(%r8), %ymm2, %ymm2
    Z1536_LOAD24 96, %ymm12
    vpxor %ymm12, %ymm3, %ymm3
    Z1536_SAVE_FF
    Z1536_PERM12
    Z1536_XOR_FF
    add $120, %r8
    add $120, %rax
    sub $120, %rcx
    cmp $120, %rcx
    jae .Ldm15_loop
    Z1536_STORE_STATE %rdi
    add $32, %rsp
    Z1536_VZERO
    ret
.Ldm11_entry:
    mov $88, %r9
    mov %rcx, %rax
    xor %edx, %edx
    div %r9
    test %rax, %rax
    jz .Ldm_ret0
    mov %rax, %r10
    imul %r9, %rax
    mov %rax, %r11
    sub $32, %rsp
    Z1536_LOAD_STATE %rdi
    vmovdqa zud1536_rol56_mask(%rip), %ymm11
.Ldm11_loop:
    vpxor 0(%r8), %ymm0, %ymm0
    vpxor 32(%r8), %ymm1, %ymm1
    Z1536_LOAD24 64, %ymm12
    vpxor %ymm12, %ymm2, %ymm2
    Z1536_SAVE_FF
    Z1536_PERM12
    Z1536_XOR_FF
    add $88, %r8
    dec %r10
    jnz .Ldm11_loop
    Z1536_STORE_STATE %rdi
    mov %r11, %rax
    add $32, %rsp
    Z1536_VZERO
    ret
.Ldm7_entry:
    mov $56, %r9
    mov %rcx, %rax
    xor %edx, %edx
    div %r9
    test %rax, %rax
    jz .Ldm_ret0
    mov %rax, %r10
    imul %r9, %rax
    mov %rax, %r11
    sub $32, %rsp
    Z1536_LOAD_STATE %rdi
    vmovdqa zud1536_rol56_mask(%rip), %ymm11
.Ldm7_loop:
    vpxor 0(%r8), %ymm0, %ymm0
    Z1536_LOAD24 32, %ymm12
    vpxor %ymm12, %ymm1, %ymm1
    Z1536_SAVE_FF
    Z1536_PERM12
    Z1536_XOR_FF
    add $56, %r8
    dec %r10
    jnz .Ldm7_loop
    Z1536_STORE_STATE %rdi
    mov %r11, %rax
    add $32, %rsp
    Z1536_VZERO
    ret
.Ldm_ret0:
    xor %eax, %eax
    ret
.size ZuD1536_avx2_DMLoop_Absorb,.-ZuD1536_avx2_DMLoop_Absorb

.section .rodata
.balign 32
zud1536_rol56_mask:
    .byte 1,2,3,4,5,6,7,0, 9,10,11,12,13,14,15,8, 17,18,19,20,21,22,23,16, 25,26,27,28,29,30,31,24
.balign 32
zud1536_rcv_table:
zud1536_rcv_12:
    .quad ZUD1536_rc12,0,0,0
zud1536_rcv_11:
    .quad ZUD1536_rc11,0,0,0
zud1536_rcv_10:
    .quad ZUD1536_rc10,0,0,0
zud1536_rcv_9:
    .quad ZUD1536_rc9,0,0,0
zud1536_rcv_8:
    .quad ZUD1536_rc8,0,0,0
zud1536_rcv_7:
    .quad ZUD1536_rc7,0,0,0
zud1536_rcv_6:
    .quad ZUD1536_rc6,0,0,0
zud1536_rcv_5:
    .quad ZUD1536_rc5,0,0,0
zud1536_rcv_4:
    .quad ZUD1536_rc4,0,0,0
zud1536_rcv_3:
    .quad ZUD1536_rc3,0,0,0
zud1536_rcv_2:
    .quad ZUD1536_rc2,0,0,0
zud1536_rcv_1:
    .quad ZUD1536_rc1,0,0,0
