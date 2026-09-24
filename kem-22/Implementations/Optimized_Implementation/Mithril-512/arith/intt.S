.intel_syntax noprefix
.include "arith/ntt_avx_macros.inc"

.equ INTT_AVX_PARAM_FIRST6_PRE_BYTES, 640
.equ INTT_AVX_PARAM_TAIL_ZETAS_BYTES, 1280
.equ INTT_AVX_PARAM_TAIL_PRE_BYTES, 1288
.equ INTT_AVX_FIRST6_BLOCK_BYTES, 320

.macro montmul_oct x y
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
    vpblendd ymm\y, ymm13, ymm14, 0xaa
.endm

.macro gs_packed_lazy l, h, pre_e=1, pre_o=3, zeta_e=2, zeta_o=15
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
    reduce_low_oct \l, 12, 13 /* Reduce back to [-p, p] */
.endm

.macro load_roots_levels0t5_early off_block off
    vmovdqa ymm1, YMMWORD PTR [rcx + INTT_AVX_FIRST6_BLOCK_BYTES*\off_block + \off]
    vmovdqa ymm2, YMMWORD PTR [rdx + INTT_AVX_FIRST6_BLOCK_BYTES*\off_block + \off]
    vmovshdup ymm3, ymm1
    vmovshdup ymm15, ymm2
.endm

.macro load_roots_levels0t5_late off_block off, pre_o
    vmovdqa ymm1, YMMWORD PTR [rcx + INTT_AVX_FIRST6_BLOCK_BYTES*\off_block + \off]
    vmovdqa ymm2, YMMWORD PTR [rdx + INTT_AVX_FIRST6_BLOCK_BYTES*\off_block + \off]
    vmovshdup ymm\pre_o, ymm1
    vmovshdup ymm15, ymm2
.endm

.macro store_levels0t5_natural off
    vmovdqa YMMWORD PTR [rdi + 256*\off +   0], ymm9
    vmovdqa YMMWORD PTR [rdi + 256*\off +  32], ymm10
    vmovdqa YMMWORD PTR [rdi + 256*\off +  64], ymm6
    vmovdqa YMMWORD PTR [rdi + 256*\off +  96], ymm4
    vmovdqa YMMWORD PTR [rdi + 256*\off + 128], ymm3
    vmovdqa YMMWORD PTR [rdi + 256*\off + 160], ymm5
    vmovdqa YMMWORD PTR [rdi + 256*\off + 192], ymm8
    vmovdqa YMMWORD PTR [rdi + 256*\off + 224], ymm11
.endm

.macro gs_final_cross_fused l, h
    vpsubd ymm3, ymm\h, ymm\l
    vpaddd ymm\l, ymm\l, ymm\h
    vmovdqa ymm10, ymm1
    vmovdqa ymm11, ymm2
    vmovdqa ymm1, ymm8
    vmovdqa ymm2, ymm9
    montmul_oct \l \l
    vmovdqa ymm1, ymm10
    vmovdqa ymm2, ymm11
    montmul_oct 3 \h
.endm

.macro first6_levels0t5_block off
    vmovdqa ymm4,  YMMWORD PTR [rdi + 256*\off +   0]
    vmovdqa ymm5,  YMMWORD PTR [rdi + 256*\off +  32]
    vmovdqa ymm6,  YMMWORD PTR [rdi + 256*\off +  64]
    vmovdqa ymm7,  YMMWORD PTR [rdi + 256*\off +  96]
    vmovdqa ymm8,  YMMWORD PTR [rdi + 256*\off + 128]
    vmovdqa ymm9,  YMMWORD PTR [rdi + 256*\off + 160]
    vmovdqa ymm10, YMMWORD PTR [rdi + 256*\off + 192]
    vmovdqa ymm11, YMMWORD PTR [rdi + 256*\off + 224]

    /* Level 0 inputs guaranteed in [-p, p] */
    load_roots_levels0t5_early \off 0
    gs_packed 4, 5, 1, 3, 2, 15
    load_roots_levels0t5_early \off 32
    gs_packed 6, 7, 1, 3, 2, 15
    load_roots_levels0t5_early \off 64
    gs_packed 8, 9, 1, 3, 2, 15
    load_roots_levels0t5_early \off 96
    gs_packed 10, 11, 1, 3, 2, 15

    /* Level 1 inputs guaranteed in [-p, p] */
    load_roots_levels0t5_early \off 128
    gs_packed 4, 6, 1, 3, 2, 15
    gs_packed 5, 7, 1, 3, 2, 15
    load_roots_levels0t5_early \off 160
    gs_packed 8, 10, 1, 3, 2, 15
    gs_packed 9, 11, 1, 3, 2, 15

    /* Level 2 inputs guaranteed in [-p, p] */
    load_roots_levels0t5_early \off 192
    gs_packed_lazy 4, 8, 1, 3, 2, 15
    gs_packed_lazy 5, 9, 1, 3, 2, 15
    gs_packed_lazy 6, 10, 1, 3, 2, 15
    gs_packed_lazy 7, 11, 1, 3, 2, 15

    shuffle2 4, 5, 3, 5
    shuffle2 6, 7, 4, 7
    shuffle2 8, 9, 6, 9
    shuffle2 10, 11, 8, 11

    /* Level 4 inputs guaranteed in [-2p, 2p] due to lazy reduction */
    load_roots_levels0t5_late \off 224, 10
    gs_packed_lazy 3, 5, 1, 10, 2, 15
    gs_packed_lazy 4, 7, 1, 10, 2, 15
    gs_packed_lazy 6, 9, 1, 10, 2, 15
    gs_packed_lazy 8, 11, 1, 10, 2, 15

    shuffle4 3, 4, 10, 4
    shuffle4 6, 8, 3, 8
    shuffle4 5, 7, 6, 7
    shuffle4 9, 11, 5, 11

    /* Level 5 inputs guaranteed in [-4p, 4p] */
    load_roots_levels0t5_late \off 256, 9
    gs_packed_lazy 10, 4, 1, 9, 2, 15
    gs_packed_lazy 3, 8, 1, 9, 2, 15
    gs_packed_lazy 6, 7, 1, 9, 2, 15
    gs_packed_lazy 5, 11, 1, 9, 2, 15

    shuffle8 10, 3, 9, 3
    shuffle8 6, 5, 10, 5
    shuffle8 4, 8, 6, 8
    shuffle8 7, 11, 4, 11

    /* Level 5 inputs guaranteed in [-8p, 8p] */
    load_roots_levels0t5_late \off 288, 7
    gs_packed_lazy 9, 3, 1, 7, 2, 15
    gs_packed_lazy 10, 5, 1, 7, 2, 15
    gs_packed_lazy 6, 8, 1, 7, 2, 15
    gs_packed_lazy 4, 11, 1, 7, 2, 15

    store_levels0t5_natural \off
.endm

.macro poly_intt32_last1_final_fused_avx2
    vpbroadcastd ymm1, DWORD PTR [rcx]
    vpbroadcastd ymm2, DWORD PTR [rdx]
    vpbroadcastd ymm8, DWORD PTR [rcx + 4]
    vpbroadcastd ymm9, DWORD PTR [rdx + 4]

    /* Level 6 inputs guaranteed in [-16p, 16p] */
    vmovdqa ymm4, YMMWORD PTR [rdi + 0*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 0*32 + 256]
    gs_final_cross_fused 4, 5
    vmovdqa YMMWORD PTR [rdi + 0*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 0*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 1*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 1*32 + 256]
    gs_final_cross_fused 4, 5
    vmovdqa YMMWORD PTR [rdi + 1*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 1*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 2*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 2*32 + 256]
    gs_final_cross_fused 4, 5
    vmovdqa YMMWORD PTR [rdi + 2*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 2*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 3*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 3*32 + 256]
    gs_final_cross_fused 4, 5
    vmovdqa YMMWORD PTR [rdi + 3*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 3*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 4*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 4*32 + 256]
    gs_final_cross_fused 4, 5
    vmovdqa YMMWORD PTR [rdi + 4*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 4*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 5*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 5*32 + 256]
    gs_final_cross_fused 4, 5
    vmovdqa YMMWORD PTR [rdi + 5*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 5*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 6*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 6*32 + 256]
    gs_final_cross_fused 4, 5
    vmovdqa YMMWORD PTR [rdi + 6*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 6*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 7*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 7*32 + 256]
    gs_final_cross_fused 4, 5
    vmovdqa YMMWORD PTR [rdi + 7*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 7*32 + 256], ymm5
.endm

/* This function assumes that inputs are in [-p, p], otherwise lazy reduction may cause overflows */
.globl intt_avx
.type intt_avx, @function
intt_avx:
    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0

    lea rcx, [rdx + INTT_AVX_PARAM_FIRST6_PRE_BYTES]
    first6_levels0t5_block 0
    first6_levels0t5_block 1

    lea rcx, [rdx + INTT_AVX_PARAM_TAIL_PRE_BYTES]
    lea rdx, [rdx + INTT_AVX_PARAM_TAIL_ZETAS_BYTES]
    poly_intt32_last1_final_fused_avx2

    ret
.size intt_avx, .-intt_avx

.section .note.GNU-stack,"",@progbits
