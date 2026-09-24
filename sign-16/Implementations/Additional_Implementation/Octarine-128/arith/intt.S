.intel_syntax noprefix

.equ INTT_AVX_PARAM_FIRST6_PRE_BYTES, 5120
.equ INTT_AVX_PARAM_TAIL_ZETAS_BYTES, 10240
.equ INTT_AVX_PARAM_TAIL_PRE_BYTES, 10304
.equ INTT_AVX_PARAM_META_BYTES, 10368
.equ INTT_AVX_FIRST6_BLOCK_BYTES, 320

#include "ntt_avx_macros.inc"

.macro montmul_oct_inplace_noblend x
    vmovshdup ymm12, ymm\x
    vpmuldq ymm13, ymm\x, ymm2
    vpmuldq ymm14, ymm12, ymm2
    vpmuldq ymm\x, ymm\x, ymm1
    vpmuldq ymm12, ymm12, ymm1
    vpmuldq ymm\x, ymm\x, ymm0
    vpmuldq ymm12, ymm12, ymm0
    vpaddq ymm13, ymm13, ymm\x
    vpaddq ymm14, ymm14, ymm12
    vmovshdup ymm13, ymm13
.endm

.macro montmul_oct_inplace x
    montmul_oct_inplace_noblend \x
    vpblendd ymm\x, ymm13, ymm14, 0xaa
.endm

.macro gs_cross l, h
    vpsubd ymm3, ymm\h, ymm\l
    vpaddd ymm\l, ymm\l, ymm\h
    reduce_low_oct \l, 12, 14
    montmul_oct_inplace_noblend 3
    vpblendd ymm\h, ymm13, ymm14, 0xaa
.endm

.macro gs_packed l, h, pre_e=1, pre_o=3, zeta_e=2, zeta_o=15
    vpsubd ymm12, ymm\h, ymm\l
    vpaddd ymm\l, ymm\l, ymm\h
    vmovshdup ymm13, ymm12
    vpmuldq ymm14, ymm12, ymm\zeta_e
    vpmuldq ymm\h, ymm13, ymm\zeta_o
    vpmuldq ymm12, ymm12, ymm\pre_e
    vpmuldq ymm13, ymm13, ymm\pre_o
    vpmuldq ymm12, ymm12, ymm0
    vpmuldq ymm13, ymm13, ymm0
    vpaddq ymm14, ymm14, ymm12
    vpaddq ymm\h, ymm\h, ymm13
    vmovshdup ymm14, ymm14
    vpblendd ymm\h, ymm14, ymm\h, 0xaa
    reduce_low_oct \l, 12, 13
.endm

.macro load_roots_levels0t5_early off
    vmovdqa ymm1, YMMWORD PTR [r13 + \off]
    vmovdqa ymm2, YMMWORD PTR [r12 + \off]
    vmovshdup ymm3, ymm1
    vmovshdup ymm15, ymm2
.endm

.macro load_roots_levels0t5_late off, pre_o
    vmovdqa ymm1, YMMWORD PTR [r13 + \off]
    vmovdqa ymm2, YMMWORD PTR [r12 + \off]
    vmovshdup ymm\pre_o, ymm1
    vmovshdup ymm15, ymm2
.endm

.macro store_levels0t5_natural
    vmovdqa YMMWORD PTR [r11 +   0], ymm9
    vmovdqa YMMWORD PTR [r11 +  32], ymm10
    vmovdqa YMMWORD PTR [r11 +  64], ymm6
    vmovdqa YMMWORD PTR [r11 +  96], ymm4
    vmovdqa YMMWORD PTR [r11 + 128], ymm3
    vmovdqa YMMWORD PTR [r11 + 160], ymm5
    vmovdqa YMMWORD PTR [r11 + 192], ymm8
    vmovdqa YMMWORD PTR [r11 + 224], ymm11
.endm

.macro gs_final_cross_fused l, h
    vpsubd ymm3, ymm\h, ymm\l
    vpaddd ymm\l, ymm\l, ymm\h
    reduce_low_oct \l, 12, 14
    vmovdqa ymm10, ymm1
    vmovdqa ymm11, ymm2
    vmovdqa ymm1, ymm8
    vmovdqa ymm2, ymm9
    montmul_oct_inplace \l
    vmovdqa ymm1, ymm10
    vmovdqa ymm2, ymm11
    montmul_oct_inplace 3
    vmovdqa ymm\h, ymm3
.endm

.text
.globl poly_intt32_first6_levels0t5_redlow_avx2
.type poly_intt32_first6_levels0t5_redlow_avx2, @function
poly_intt32_first6_levels0t5_redlow_avx2:

    mov r8d, 16
    mov r11, rdi
    mov r12, rdx
    mov r13, rcx

.Lfirst6_levels0t5_block_loop:
    vmovdqa ymm4,  YMMWORD PTR [r11 +   0]
    vmovdqa ymm5,  YMMWORD PTR [r11 +  32]
    vmovdqa ymm6,  YMMWORD PTR [r11 +  64]
    vmovdqa ymm7,  YMMWORD PTR [r11 +  96]
    vmovdqa ymm8,  YMMWORD PTR [r11 + 128]
    vmovdqa ymm9,  YMMWORD PTR [r11 + 160]
    vmovdqa ymm10, YMMWORD PTR [r11 + 192]
    vmovdqa ymm11, YMMWORD PTR [r11 + 224]

    /* level 0, len = 1 */
    load_roots_levels0t5_early 0
    gs_packed 4, 5, 1, 3, 2, 15
    load_roots_levels0t5_early 32
    gs_packed 6, 7, 1, 3, 2, 15
    load_roots_levels0t5_early 64
    gs_packed 8, 9, 1, 3, 2, 15
    load_roots_levels0t5_early 96
    gs_packed 10, 11, 1, 3, 2, 15

    /* level 1, len = 2 */
    load_roots_levels0t5_early 128
    gs_packed 4, 6, 1, 3, 2, 15
    gs_packed 5, 7, 1, 3, 2, 15
    load_roots_levels0t5_early 160
    gs_packed 8, 10, 1, 3, 2, 15
    gs_packed 9, 11, 1, 3, 2, 15

    /* level 2, len = 4 */
    load_roots_levels0t5_early 192
    gs_packed 4, 8, 1, 3, 2, 15
    gs_packed 5, 9, 1, 3, 2, 15
    gs_packed 6, 10, 1, 3, 2, 15
    gs_packed 7, 11, 1, 3, 2, 15

    shuffle2 4, 5, 3, 5
    shuffle2 6, 7, 4, 7
    shuffle2 8, 9, 6, 9
    shuffle2 10, 11, 8, 11

    /* level 3, len = 8 */
    load_roots_levels0t5_late 224, 10
    gs_packed 3, 5, 1, 10, 2, 15
    gs_packed 4, 7, 1, 10, 2, 15
    gs_packed 6, 9, 1, 10, 2, 15
    gs_packed 8, 11, 1, 10, 2, 15

    shuffle4 3, 4, 10, 4
    shuffle4 6, 8, 3, 8
    shuffle4 5, 7, 6, 7
    shuffle4 9, 11, 5, 11

    /* level 4, len = 16 */
    load_roots_levels0t5_late 256, 9
    gs_packed 10, 4, 1, 9, 2, 15
    gs_packed 3, 8, 1, 9, 2, 15
    gs_packed 6, 7, 1, 9, 2, 15
    gs_packed 5, 11, 1, 9, 2, 15

    shuffle8 10, 3, 9, 3
    shuffle8 6, 5, 10, 5
    shuffle8 4, 8, 6, 8
    shuffle8 7, 11, 4, 11

    /* level 5, len = 32 */
    load_roots_levels0t5_late 288, 7
    gs_packed 9, 3, 1, 7, 2, 15
    gs_packed 10, 5, 1, 7, 2, 15
    gs_packed 6, 8, 1, 7, 2, 15
    gs_packed 4, 11, 1, 7, 2, 15

    store_levels0t5_natural

    add r11, 256
    add r12, INTT_AVX_FIRST6_BLOCK_BYTES
    add r13, INTT_AVX_FIRST6_BLOCK_BYTES
    dec r8d
    jne .Lfirst6_levels0t5_block_loop

    ret
.size poly_intt32_first6_levels0t5_redlow_avx2, .-poly_intt32_first6_levels0t5_redlow_avx2

.globl poly_intt32_layers6_7_redlow_avx2
.type poly_intt32_layers6_7_redlow_avx2, @function
poly_intt32_layers6_7_redlow_avx2:

    xor r8d, r8d
    mov r11, rdi
.Llayers67_block_loop:
    mov r10d, 8
    mov r9, r11
.Llayers67_off_loop:
    vmovdqa ymm4, YMMWORD PTR [r9]
    vmovdqa ymm5, YMMWORD PTR [r9 + 256]
    vmovdqa ymm6, YMMWORD PTR [r9 + 512]
    vmovdqa ymm7, YMMWORD PTR [r9 + 768]

    mov eax, r8d
    shl eax, 1
    vpbroadcastd ymm1, DWORD PTR [rcx + rax*4]
    vpbroadcastd ymm2, DWORD PTR [rdx + rax*4]
    gs_cross 4, 5

    add eax, 1
    vpbroadcastd ymm1, DWORD PTR [rcx + rax*4]
    vpbroadcastd ymm2, DWORD PTR [rdx + rax*4]
    gs_cross 6, 7

    mov eax, r8d
    add eax, 8
    vpbroadcastd ymm1, DWORD PTR [rcx + rax*4]
    vpbroadcastd ymm2, DWORD PTR [rdx + rax*4]
    gs_cross 4, 6
    gs_cross 5, 7

    vmovdqa YMMWORD PTR [r9], ymm4
    vmovdqa YMMWORD PTR [r9 + 256], ymm5
    vmovdqa YMMWORD PTR [r9 + 512], ymm6
    vmovdqa YMMWORD PTR [r9 + 768], ymm7

    add r9, 32
    dec r10d
    jne .Llayers67_off_loop

    add r11, 1024
    inc r8d
    cmp r8d, 4
    jne .Llayers67_block_loop

    ret
.size poly_intt32_layers6_7_redlow_avx2, .-poly_intt32_layers6_7_redlow_avx2

.globl poly_intt32_layers8_9_final_fused_avx2
.type poly_intt32_layers8_9_final_fused_avx2, @function
poly_intt32_layers8_9_final_fused_avx2:

    mov r9, rdi
    mov r8d, 32
.Llayers89_fused_loop:
    vmovdqa ymm4, YMMWORD PTR [r9]
    vmovdqa ymm5, YMMWORD PTR [r9 + 1024]
    vmovdqa ymm6, YMMWORD PTR [r9 + 2048]
    vmovdqa ymm7, YMMWORD PTR [r9 + 3072]

    vpbroadcastd ymm1, DWORD PTR [rcx + 48]
    vpbroadcastd ymm2, DWORD PTR [rdx + 48]
    gs_cross 4, 5

    vpbroadcastd ymm1, DWORD PTR [rcx + 52]
    vpbroadcastd ymm2, DWORD PTR [rdx + 52]
    gs_cross 6, 7

    vpbroadcastd ymm1, DWORD PTR [rcx + 56]
    vpbroadcastd ymm2, DWORD PTR [rdx + 56]
    vpbroadcastd ymm8, DWORD PTR [rcx + 60]
    vpbroadcastd ymm9, DWORD PTR [rdx + 60]
    gs_final_cross_fused 4, 6
    gs_final_cross_fused 5, 7

    vmovdqa YMMWORD PTR [r9], ymm4
    vmovdqa YMMWORD PTR [r9 + 1024], ymm5
    vmovdqa YMMWORD PTR [r9 + 2048], ymm6
    vmovdqa YMMWORD PTR [r9 + 3072], ymm7

    add r9, 32
    dec r8d
    jne .Llayers89_fused_loop

    ret
.size poly_intt32_layers8_9_final_fused_avx2, .-poly_intt32_layers8_9_final_fused_avx2

.globl intt_avx
.type intt_avx, @function
intt_avx:
    push r12
    push r13
    sub rsp, 8

    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0

    lea rcx, [rdx + INTT_AVX_PARAM_FIRST6_PRE_BYTES]
    call poly_intt32_first6_levels0t5_redlow_avx2

    lea rcx, [rdx + INTT_AVX_PARAM_TAIL_PRE_BYTES]
    lea rdx, [rdx + INTT_AVX_PARAM_TAIL_ZETAS_BYTES]
    call poly_intt32_layers6_7_redlow_avx2
    call poly_intt32_layers8_9_final_fused_avx2

.Lintt_avx_done:
    add rsp, 8
    pop r13
    pop r12
    ret
.size intt_avx, .-intt_avx

.section .note.GNU-stack,"",@progbits
