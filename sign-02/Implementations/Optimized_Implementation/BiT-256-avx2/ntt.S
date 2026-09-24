/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */

/* ============================================================
   BIT NTT (forward) – AVX2, q=119297, n=512
   Correct BIT-256 factorization: strides 256,128,64,32,16,8,4,2
   (m = 1,2,4 | 8,16,32,64,128).  Output in NATURAL order.
   ============================================================ */

#include "consts.h"
.include "shuffle.inc"

.macro butterfly_fwd l,h,zl0=1,zl1=1,zh0=2,zh1=2
    vpmuldq    %ymm\zl0, %ymm\h, %ymm13
    vmovshdup  %ymm\h, %ymm12
    vpmuldq    %ymm\zl1, %ymm12, %ymm14
    vpmuldq    %ymm\zh0, %ymm\h, %ymm\h
    vpmuldq    %ymm\zh1, %ymm12, %ymm12
    vpmuldq    %ymm0, %ymm13, %ymm13
    vpmuldq    %ymm0, %ymm14, %ymm14
    vmovshdup  %ymm\h, %ymm\h
    vpblendd   $0xAA, %ymm12, %ymm\h, %ymm\h
    vpsubd     %ymm\h, %ymm\l, %ymm12
    vpaddd     %ymm\h, %ymm\l, %ymm\l
    vmovshdup  %ymm13, %ymm13
    vpblendd   $0xAA, %ymm14, %ymm13, %ymm13
    vpaddd     %ymm13, %ymm12, %ymm\h
    vpsubd     %ymm13, %ymm\l, %ymm\l
.endm

/* Phase A: cross-register levels m=1,2,4 (strides 256,128,64).
   Registers ymm4..11 hold R0..R7 ; Ri lane j = a[i*64 + 8*off + j]. */
.macro fwd_phaseA off
    vmovdqa    0+32*\off(%rdi), %ymm4
    vmovdqa  256+32*\off(%rdi), %ymm5
    vmovdqa  512+32*\off(%rdi), %ymm6
    vmovdqa  768+32*\off(%rdi), %ymm7
    vmovdqa 1024+32*\off(%rdi), %ymm8
    vmovdqa 1280+32*\off(%rdi), %ymm9
    vmovdqa 1536+32*\off(%rdi), %ymm10
    vmovdqa 1792+32*\off(%rdi), %ymm11

    /* m=1 (stride 256) : M[1] */
    vpbroadcastd zfwd_l+(1)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(1)*4(%rip), %ymm2
    butterfly_fwd 4,8
    butterfly_fwd 5,9
    butterfly_fwd 6,10
    butterfly_fwd 7,11

    /* m=2 (stride 128) : M[2],M[3] */
    vpbroadcastd zfwd_l+(2)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(2)*4(%rip), %ymm2
    butterfly_fwd 4,6
    butterfly_fwd 5,7
    vpbroadcastd zfwd_l+(3)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(3)*4(%rip), %ymm2
    butterfly_fwd 8,10
    butterfly_fwd 9,11

    /* m=4 (stride 64) : M[4..7] */
    vpbroadcastd zfwd_l+(4)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(4)*4(%rip), %ymm2
    butterfly_fwd 4,5
    vpbroadcastd zfwd_l+(5)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(5)*4(%rip), %ymm2
    butterfly_fwd 6,7
    vpbroadcastd zfwd_l+(6)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(6)*4(%rip), %ymm2
    butterfly_fwd 8,9
    vpbroadcastd zfwd_l+(7)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(7)*4(%rip), %ymm2
    butterfly_fwd 10,11

    vmovdqa %ymm4,    0+32*\off(%rdi)
    vmovdqa %ymm5,  256+32*\off(%rdi)
    vmovdqa %ymm6,  512+32*\off(%rdi)
    vmovdqa %ymm7,  768+32*\off(%rdi)
    vmovdqa %ymm8, 1024+32*\off(%rdi)
    vmovdqa %ymm9, 1280+32*\off(%rdi)
    vmovdqa %ymm10,1536+32*\off(%rdi)
    vmovdqa %ymm11,1792+32*\off(%rdi)
.endm

/* Phase B: within 64-coeff block, levels m=8,16,32 (cross-register)
   and m=64,128 (intra-register via shuffle8 / shuffle4 involutions).
   Registers ymm4..11 hold S0..S7 ; Si lane j = a[64*off + 8*i + j]. */
.macro fwd_phaseB off
    vmovdqa  256*\off+  0(%rdi), %ymm4
    vmovdqa  256*\off+ 32(%rdi), %ymm5
    vmovdqa  256*\off+ 64(%rdi), %ymm6
    vmovdqa  256*\off+ 96(%rdi), %ymm7
    vmovdqa  256*\off+128(%rdi), %ymm8
    vmovdqa  256*\off+160(%rdi), %ymm9
    vmovdqa  256*\off+192(%rdi), %ymm10
    vmovdqa  256*\off+224(%rdi), %ymm11

    /* m=8 (stride 32) broadcast M[8+off] : (4,8),(5,9),(6,10),(7,11) */
    vpbroadcastd zfwd_l+(8+\off)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(8+\off)*4(%rip), %ymm2
    butterfly_fwd 4,8
    butterfly_fwd 5,9
    butterfly_fwd 6,10
    butterfly_fwd 7,11

    /* m=16 (stride 16) : (4,6),(5,7)->M[16+2off] ; (8,10),(9,11)->M[17+2off] */
    vpbroadcastd zfwd_l+(16+2*\off)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(16+2*\off)*4(%rip), %ymm2
    butterfly_fwd 4,6
    butterfly_fwd 5,7
    vpbroadcastd zfwd_l+(17+2*\off)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(17+2*\off)*4(%rip), %ymm2
    butterfly_fwd 8,10
    butterfly_fwd 9,11

    /* m=32 (stride 8) : (4,5),(6,7),(8,9),(10,11)->M[32+4off+g] */
    vpbroadcastd zfwd_l+(32+4*\off)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(32+4*\off)*4(%rip), %ymm2
    butterfly_fwd 4,5
    vpbroadcastd zfwd_l+(33+4*\off)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(33+4*\off)*4(%rip), %ymm2
    butterfly_fwd 6,7
    vpbroadcastd zfwd_l+(34+4*\off)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(34+4*\off)*4(%rip), %ymm2
    butterfly_fwd 8,9
    vpbroadcastd zfwd_l+(35+4*\off)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(35+4*\off)*4(%rip), %ymm2
    butterfly_fwd 10,11

    /* m=64 (stride 4, intra) via shuffle8 ; vector base 64+32off+8g */
    shuffle8 4,5,3,15
    vmovdqa zfwd_l+(64+32*\off+0)*4(%rip), %ymm1
    vmovdqa zfwd_h+(64+32*\off+0)*4(%rip), %ymm2
    butterfly_fwd 3,15
    shuffle8 3,15,4,5
    shuffle8 6,7,3,15
    vmovdqa zfwd_l+(64+32*\off+8)*4(%rip), %ymm1
    vmovdqa zfwd_h+(64+32*\off+8)*4(%rip), %ymm2
    butterfly_fwd 3,15
    shuffle8 3,15,6,7
    shuffle8 8,9,3,15
    vmovdqa zfwd_l+(64+32*\off+16)*4(%rip), %ymm1
    vmovdqa zfwd_h+(64+32*\off+16)*4(%rip), %ymm2
    butterfly_fwd 3,15
    shuffle8 3,15,8,9
    shuffle8 10,11,3,15
    vmovdqa zfwd_l+(64+32*\off+24)*4(%rip), %ymm1
    vmovdqa zfwd_h+(64+32*\off+24)*4(%rip), %ymm2
    butterfly_fwd 3,15
    shuffle8 3,15,10,11

    /* m=128 (stride 2, intra) via shuffle4 ; vector base 320+32off+8g */
    shuffle4 4,5,3,15
    vmovdqa zfwd_l+(320+32*\off+0)*4(%rip), %ymm1
    vmovdqa zfwd_h+(320+32*\off+0)*4(%rip), %ymm2
    butterfly_fwd 3,15
    shuffle4 3,15,4,5
    shuffle4 6,7,3,15
    vmovdqa zfwd_l+(320+32*\off+8)*4(%rip), %ymm1
    vmovdqa zfwd_h+(320+32*\off+8)*4(%rip), %ymm2
    butterfly_fwd 3,15
    shuffle4 3,15,6,7
    shuffle4 8,9,3,15
    vmovdqa zfwd_l+(320+32*\off+16)*4(%rip), %ymm1
    vmovdqa zfwd_h+(320+32*\off+16)*4(%rip), %ymm2
    butterfly_fwd 3,15
    shuffle4 3,15,8,9
    shuffle4 10,11,3,15
    vmovdqa zfwd_l+(320+32*\off+24)*4(%rip), %ymm1
    vmovdqa zfwd_h+(320+32*\off+24)*4(%rip), %ymm2
    butterfly_fwd 3,15
    shuffle4 3,15,10,11

    vmovdqa %ymm4,  256*\off+  0(%rdi)
    vmovdqa %ymm5,  256*\off+ 32(%rdi)
    vmovdqa %ymm6,  256*\off+ 64(%rdi)
    vmovdqa %ymm7,  256*\off+ 96(%rdi)
    vmovdqa %ymm8,  256*\off+128(%rdi)
    vmovdqa %ymm9,  256*\off+160(%rdi)
    vmovdqa %ymm10, 256*\off+192(%rdi)
    vmovdqa %ymm11, 256*\off+224(%rdi)
.endm

.text
.global ntt_fwd_asm
.global ntt_avx_forward
ntt_avx_forward:
ntt_fwd_asm:
    vmovdqa q(%rip), %ymm0

    fwd_phaseA 0
    fwd_phaseA 1
    fwd_phaseA 2
    fwd_phaseA 3
    fwd_phaseA 4
    fwd_phaseA 5
    fwd_phaseA 6
    fwd_phaseA 7

    fwd_phaseB 0
    fwd_phaseB 1
    fwd_phaseB 2
    fwd_phaseB 3
    fwd_phaseB 4
    fwd_phaseB 5
    fwd_phaseB 6
    fwd_phaseB 7

    ret

.section .note.GNU-stack,"",@progbits
