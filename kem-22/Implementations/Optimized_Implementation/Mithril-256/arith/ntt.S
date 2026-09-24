.intel_syntax noprefix
.include "arith/ntt_avx_macros.inc"

.equ NTT_AVX_PARAM_TOP1_QINV_BYTES, 4
.equ NTT_AVX_PARAM_BOTTOM6_ZETA_BYTES, 32
.equ NTT_AVX_PARAM_BOTTOM6_QINV_BYTES, 672

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
    vpsubd ymm\h, ymm\l, ymm13
    vpaddd ymm\l, ymm\l, ymm13
.endm

.macro levels4t9_dilithium_redlow off
    vmovdqa ymm4,  YMMWORD PTR [rdi + 256*\off +   0]
    vmovdqa ymm5,  YMMWORD PTR [rdi + 256*\off +  32]
    vmovdqa ymm6,  YMMWORD PTR [rdi + 256*\off +  64]
    vmovdqa ymm7,  YMMWORD PTR [rdi + 256*\off +  96]
    vmovdqa ymm8,  YMMWORD PTR [rdi + 256*\off + 128]
    vmovdqa ymm9,  YMMWORD PTR [rdi + 256*\off + 160]
    vmovdqa ymm10, YMMWORD PTR [rdi + 256*\off + 192]
    vmovdqa ymm11, YMMWORD PTR [rdi + 256*\off + 224]

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off +   0]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off +   0]
    butterfly 4, 8
    butterfly 5, 9
    butterfly 6, 10
    butterfly 7, 11

    shuffle8 4, 8, 3, 8
    shuffle8 5, 9, 4, 9
    shuffle8 6, 10, 5, 10
    shuffle8 7, 11, 6, 11

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off +  32]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off +  32]
    butterfly 3, 5
    butterfly 8, 10
    butterfly 4, 6
    butterfly 9, 11

    shuffle4 3, 5, 7, 5
    shuffle4 8, 10, 3, 10
    shuffle4 4, 6, 8, 6
    shuffle4 9, 11, 4, 11

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off +  64]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off +  64]
    butterfly 7, 8
    butterfly 5, 6
    butterfly 3, 4
    butterfly 10, 11

    shuffle2 7, 8, 9, 8
    shuffle2 5, 6, 7, 6
    shuffle2 3, 4, 5, 4
    shuffle2 10, 11, 3, 11

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off +  96]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off +  96]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 5, 1, 10, 2, 15
    butterfly 8, 4, 1, 10, 2, 15
    butterfly 7, 3, 1, 10, 2, 15
    butterfly 6, 11, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off + 128]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off + 128]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 7, 1, 10, 2, 15
    butterfly 8, 6, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off + 160]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off + 160]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 5, 3, 1, 10, 2, 15
    butterfly 4, 11, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off + 192]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off + 192]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 9, 8, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off + 224]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off + 224]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 7, 6, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off + 256]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off + 256]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 5, 4, 1, 10, 2, 15

    vmovdqa ymm1, YMMWORD PTR [rcx + 320*\off + 288]
    vmovdqa ymm2, YMMWORD PTR [rdx + 320*\off + 288]
    vpsrlq ymm10, ymm1, 32
    vmovshdup ymm15, ymm2
    butterfly 3, 11, 1, 10, 2, 15

    vmovdqa YMMWORD PTR [rdi + 256*\off +   0], ymm9
    vmovdqa YMMWORD PTR [rdi + 256*\off +  32], ymm8
    vmovdqa YMMWORD PTR [rdi + 256*\off +  64], ymm7
    vmovdqa YMMWORD PTR [rdi + 256*\off +  96], ymm6
    vmovdqa YMMWORD PTR [rdi + 256*\off + 128], ymm5
    vmovdqa YMMWORD PTR [rdi + 256*\off + 160], ymm4
    vmovdqa YMMWORD PTR [rdi + 256*\off + 192], ymm3
    vmovdqa YMMWORD PTR [rdi + 256*\off + 224], ymm11
.endm

.macro poly_ntt32_top1_redlow_avx2m
    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0
    vpbroadcastd ymm1, DWORD PTR [rcx]
    vpbroadcastd ymm2, DWORD PTR [rdx]

    vmovdqa ymm4, YMMWORD PTR [rdi]
    vmovdqa ymm5, YMMWORD PTR [rdi + 256]
    butterfly 4, 5
    vmovdqa YMMWORD PTR [rdi], ymm4
    vmovdqa YMMWORD PTR [rdi + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 1*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 1*32 + 256]
    butterfly 4, 5
    vmovdqa YMMWORD PTR [rdi + 1*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 1*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 2*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 2*32 + 256]
    butterfly 4, 5
    vmovdqa YMMWORD PTR [rdi + 2*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 2*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 3*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 3*32 + 256]
    butterfly 4, 5
    vmovdqa YMMWORD PTR [rdi + 3*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 3*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 4*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 4*32 + 256]
    butterfly 4, 5
    vmovdqa YMMWORD PTR [rdi + 4*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 4*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 5*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 5*32 + 256]
    butterfly 4, 5
    vmovdqa YMMWORD PTR [rdi + 5*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 5*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 6*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 6*32 + 256]
    butterfly 4, 5
    vmovdqa YMMWORD PTR [rdi + 6*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 6*32 + 256], ymm5

    vmovdqa ymm4, YMMWORD PTR [rdi + 7*32]
    vmovdqa ymm5, YMMWORD PTR [rdi + 7*32 + 256]
    butterfly 4, 5
    vmovdqa YMMWORD PTR [rdi + 7*32], ymm4
    vmovdqa YMMWORD PTR [rdi + 7*32 + 256], ymm5
.endm

.globl ntt_avx
.type ntt_avx, @function
ntt_avx:

     lea rcx, [rdx + NTT_AVX_PARAM_TOP1_QINV_BYTES]
     poly_ntt32_top1_redlow_avx2m
 
     lea rcx, [rdx + NTT_AVX_PARAM_BOTTOM6_QINV_BYTES]
     lea rdx, [rdx + NTT_AVX_PARAM_BOTTOM6_ZETA_BYTES]
     levels4t9_dilithium_redlow 0
     levels4t9_dilithium_redlow 1
 
     vzeroupper
     ret
.size ntt_avx, .-ntt_avx
