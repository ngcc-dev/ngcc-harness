.intel_syntax noprefix

.equ NTT_AVX_PARAM_TOPMID_PRE_BYTES, 64
.equ NTT_AVX_PARAM_BOT6_ZETAS_BYTES, 128
.equ NTT_AVX_PARAM_BOT6_PRE_BYTES, 5248

#include "ntt_avx_macros.inc"

.macro butterfly l, h, pre_e=1, pre_o=1, zeta_e=2, zeta_o=2
    vmovshdup ymm12, ymm\h
    vpmuldq ymm13, ymm\h, ymm\zeta_e
    vpmuldq ymm14, ymm12, ymm\zeta_o
    vpmuldq ymm\h, ymm\h, ymm\pre_e
    vpmuldq ymm12, ymm12, ymm\pre_o
    vpmuldq ymm\h, ymm\h, ymm0
    vpmuldq ymm12, ymm12, ymm0
    vpaddq ymm13, ymm13, ymm\h
    vpaddq ymm14, ymm14, ymm12
    vmovshdup ymm13, ymm13
    vpblendd ymm13, ymm13, ymm14, 0xaa
    reduce_low_oct \l, 12, 14
    vpsubd ymm\h, ymm\l, ymm13
    vpaddd ymm\l, ymm\l, ymm13
.endm

.macro levels4t9_dilithium_redlow
    vmovdqa ymm4,  YMMWORD PTR [r11 +   0]
    vmovdqa ymm5,  YMMWORD PTR [r11 +  32]
    vmovdqa ymm6,  YMMWORD PTR [r11 +  64]
    vmovdqa ymm7,  YMMWORD PTR [r11 +  96]
    vmovdqa ymm8,  YMMWORD PTR [r11 + 128]
    vmovdqa ymm9,  YMMWORD PTR [r11 + 160]
    vmovdqa ymm10, YMMWORD PTR [r11 + 192]
    vmovdqa ymm11, YMMWORD PTR [r11 + 224]

    vmovdqa ymm1, YMMWORD PTR [r13 +   0]
    vmovdqa ymm2, YMMWORD PTR [r12 +   0]
    butterfly 4, 8
    butterfly 5, 9
    butterfly 6, 10
    butterfly 7, 11

    shuffle8 4, 8, 3, 8
    shuffle8 5, 9, 4, 9
    shuffle8 6, 10, 5, 10
    shuffle8 7, 11, 6, 11

    vmovdqa ymm1, YMMWORD PTR [r13 +  32]
    vmovdqa ymm2, YMMWORD PTR [r12 +  32]
    butterfly 3, 5
    butterfly 8, 10
    butterfly 4, 6
    butterfly 9, 11

    shuffle4 3, 5, 7, 5
    shuffle4 8, 10, 3, 10
    shuffle4 4, 6, 8, 6
    shuffle4 9, 11, 4, 11

    vmovdqa ymm1, YMMWORD PTR [r13 +  64]
    vmovdqa ymm2, YMMWORD PTR [r12 +  64]
    butterfly 7, 8
    butterfly 5, 6
    butterfly 3, 4
    butterfly 10, 11

    shuffle2 7, 8, 9, 8
    shuffle2 5, 6, 7, 6
    shuffle2 3, 4, 5, 4
    shuffle2 10, 11, 3, 11

    vmovdqa ymm1, YMMWORD PTR [r13 +  96]
    vmovdqa ymm2, YMMWORD PTR [r12 +  96]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 5, 1, 10, 2, 15
    butterfly 8, 4, 1, 10, 2, 15
    butterfly 7, 3, 1, 10, 2, 15
    butterfly 6, 11, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [r13 + 128]
    vmovdqa ymm2, YMMWORD PTR [r12 + 128]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 7, 1, 10, 2, 15
    butterfly 8, 6, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [r13 + 160]
    vmovdqa ymm2, YMMWORD PTR [r12 + 160]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 5, 3, 1, 10, 2, 15
    butterfly 4, 11, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [r13 + 192]
    vmovdqa ymm2, YMMWORD PTR [r12 + 192]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 8, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [r13 + 224]
    vmovdqa ymm2, YMMWORD PTR [r12 + 224]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 7, 6, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [r13 + 256]
    vmovdqa ymm2, YMMWORD PTR [r12 + 256]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 5, 4, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [r13 + 288]
    vmovdqa ymm2, YMMWORD PTR [r12 + 288]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 3, 11, 1, 10, 2, 15

    vmovdqa YMMWORD PTR [r11 +   0], ymm9
    vmovdqa YMMWORD PTR [r11 +  32], ymm8
    vmovdqa YMMWORD PTR [r11 +  64], ymm7
    vmovdqa YMMWORD PTR [r11 +  96], ymm6
    vmovdqa YMMWORD PTR [r11 + 128], ymm5
    vmovdqa YMMWORD PTR [r11 + 160], ymm4
    vmovdqa YMMWORD PTR [r11 + 192], ymm3
    vmovdqa YMMWORD PTR [r11 + 224], ymm11
.endm

.text
.globl poly_ntt32_top2_redlow_avx2
.type poly_ntt32_top2_redlow_avx2, @function
poly_ntt32_top2_redlow_avx2:

    vpbroadcastd ymm1, DWORD PTR [rcx + 4]
    vpbroadcastd ymm2, DWORD PTR [rdx + 4]

    mov r8d, 32
    mov r9, rdi
.Ltop2_loop:
    vmovdqa ymm4, YMMWORD PTR [r9]
    vmovdqa ymm5, YMMWORD PTR [r9 + 1024]
    vmovdqa ymm6, YMMWORD PTR [r9 + 2048]
    vmovdqa ymm7, YMMWORD PTR [r9 + 3072]

    butterfly 4, 6
    butterfly 5, 7

    vpbroadcastd ymm1, DWORD PTR [rcx + 8]
    vpbroadcastd ymm2, DWORD PTR [rdx + 8]
    butterfly 4, 5

    vpbroadcastd ymm1, DWORD PTR [rcx + 12]
    vpbroadcastd ymm2, DWORD PTR [rdx + 12]
    butterfly 6, 7

    vmovdqa YMMWORD PTR [r9], ymm4
    vmovdqa YMMWORD PTR [r9 + 1024], ymm5
    vmovdqa YMMWORD PTR [r9 + 2048], ymm6
    vmovdqa YMMWORD PTR [r9 + 3072], ymm7

    vpbroadcastd ymm1, DWORD PTR [rcx + 4]
    vpbroadcastd ymm2, DWORD PTR [rdx + 4]
    add r9, 32
    dec r8d
    jne .Ltop2_loop

    ret
.size poly_ntt32_top2_redlow_avx2, .-poly_ntt32_top2_redlow_avx2

.globl poly_ntt32_middle2_redlow_avx2
.type poly_ntt32_middle2_redlow_avx2, @function
poly_ntt32_middle2_redlow_avx2:

    xor r8d, r8d
    mov r11, rdi
.Lmiddle2_block_loop:
    mov r10d, 8
    mov r9, r11
.Lmiddle2_off_loop:
    vmovdqa ymm4, YMMWORD PTR [r9]
    vmovdqa ymm5, YMMWORD PTR [r9 + 256]
    vmovdqa ymm6, YMMWORD PTR [r9 + 512]
    vmovdqa ymm7, YMMWORD PTR [r9 + 768]

    mov eax, r8d
    add eax, 4
    vpbroadcastd ymm1, DWORD PTR [rcx + rax*4]
    vpbroadcastd ymm2, DWORD PTR [rdx + rax*4]
    butterfly 4, 6
    butterfly 5, 7

    mov eax, r8d
    shl eax, 1
    add eax, 8
    vpbroadcastd ymm1, DWORD PTR [rcx + rax*4]
    vpbroadcastd ymm2, DWORD PTR [rdx + rax*4]
    butterfly 4, 5

    add eax, 1
    vpbroadcastd ymm1, DWORD PTR [rcx + rax*4]
    vpbroadcastd ymm2, DWORD PTR [rdx + rax*4]
    butterfly 6, 7

    vmovdqa YMMWORD PTR [r9], ymm4
    vmovdqa YMMWORD PTR [r9 + 256], ymm5
    vmovdqa YMMWORD PTR [r9 + 512], ymm6
    vmovdqa YMMWORD PTR [r9 + 768], ymm7

    add r9, 32
    dec r10d
    jne .Lmiddle2_off_loop

    add r11, 1024
    inc r8d
    cmp r8d, 4
    jne .Lmiddle2_block_loop

    ret
.size poly_ntt32_middle2_redlow_avx2, .-poly_ntt32_middle2_redlow_avx2

.globl poly_ntt32_bottom6_dilithium_redlow_avx2
.type poly_ntt32_bottom6_dilithium_redlow_avx2, @function
poly_ntt32_bottom6_dilithium_redlow_avx2:

    mov r8d, 16
.Lblock_loop:
    mov r11, rdi
    mov r12, rdx
    mov r13, rcx
    levels4t9_dilithium_redlow
    add rdi, 256
    add rdx, 320
    add rcx, 320
    dec r8d
    jne .Lblock_loop

    ret
.size poly_ntt32_bottom6_dilithium_redlow_avx2, .-poly_ntt32_bottom6_dilithium_redlow_avx2

.globl ntt_avx
.type ntt_avx, @function
ntt_avx:
    push r12
    push r13
    sub rsp, 8

    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0

    lea rcx, [rdx + NTT_AVX_PARAM_TOPMID_PRE_BYTES]
    call poly_ntt32_top2_redlow_avx2
    call poly_ntt32_middle2_redlow_avx2

    lea rcx, [rdx + NTT_AVX_PARAM_BOT6_PRE_BYTES]
    lea rdx, [rdx + NTT_AVX_PARAM_BOT6_ZETAS_BYTES]
    call poly_ntt32_bottom6_dilithium_redlow_avx2

    add rsp, 8
    pop r13
    pop r12
    ret
.size ntt_avx, .-ntt_avx
