/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */

/* ============================================================
   BiT-512  forward NTT  (complete, n=1024, q=520193)  — AVX2 asm
   Cooley-Tukey, signed Montgomery (Dilithium butterfly), register-blocked.
   Output: lazy signed (|c| < 2^21); only consumer is the pointwise
   Montgomery multiply, which reduces mod q.  KAT bit-identical.

   phase0 : t=512                (M[1])
   phaseA : t=256,128,64         (per half H, column off)
   phaseB : t=32,16,8 (cross)  + t=4,2,1 (intra via shuffle8/4/2)

   ymm0=q  ymm1=zl(zeta*qinv)  ymm2=zh(zeta)  ymm3,15=transpose tmp
   ymm4..11=data  ymm12,13,14=scratch
   ============================================================ */

#include "shuffle.inc"

    .text

/* signed Montgomery CT butterfly (Dilithium):
   l := l + zeta*h ,  h := l - zeta*h   (montgomery, lazy in (-q,q) growth)
   zl in ymm1, zh in ymm2, q in ymm0 ; scratch 12,13,14 */
.macro butterfly_fwd l,h
    vpmuldq    %ymm1, %ymm\h, %ymm13
    vmovshdup  %ymm\h, %ymm12
    vpmuldq    %ymm1, %ymm12, %ymm14
    vpmuldq    %ymm2, %ymm\h, %ymm\h
    vpmuldq    %ymm2, %ymm12, %ymm12
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

/* broadcast zl/zh for forward zeta index \zi into ymm1/ymm2 */
.macro lzf zi
    vpbroadcastd zfwd_l+(\zi)*4(%rip), %ymm1
    vpbroadcastd zfwd_h+(\zi)*4(%rip), %ymm2
.endm

/* intra butterfly (t=4,2) on register pair (\p,\q) via transpose \shuf and
   vector zeta from bit512_fit_{l,h} at byte offset \off.
   (shuffle8/shuffle4 keep lane-pairs sharing a zeta, so butterfly_fwd works.) */
.macro intra_fwd p, q, shuf, off
    \shuf \p,\q,3,15
    vmovdqa   fit_l+(\off)(%rip), %ymm1
    vmovdqa   fit_h+(\off)(%rip), %ymm2
    butterfly_fwd 3,15
    \shuf 3,15,\p,\q
.endm

/* intra t=1 (stride 1) in-place, per-lane zeta — adjacent lanes need
   distinct zetas so we use a full per-lane Montgomery multiply.
   qinv(+) in ymm1, zeta(zh) vector from fit1_h at \off ; scratch 5,6,12..15 */
.macro butterfly_fwd_t1 A, off
    vmovdqa   fit1_h+(\off)(%rip), %ymm2
    vpshufd   $0xA0, %ymm\A, %ymm3          /* Ud = even-lane dup (keep) */
    vpshufd   $0xF5, %ymm\A, %ymm12         /* Vd = odd-lane dup  */
    vpmuldq   %ymm2,  %ymm12, %ymm13        /* pe = zh_e*Vd_e */
    vmovshdup %ymm12, %ymm14                /* Vd_odd */
    vmovshdup %ymm2,  %ymm15                /* zh_odd */
    vpmuldq   %ymm15, %ymm14, %ymm14        /* po */
    vpmuldq   %ymm1,  %ymm13, %ymm15        /* m_e = lo(pe)*qinv */
    vpmuldq   %ymm1,  %ymm14, %ymm12        /* m_o */
    vpmuldq   %ymm0,  %ymm15, %ymm15        /* m_e*q */
    vpmuldq   %ymm0,  %ymm12, %ymm12        /* m_o*q */
    vmovshdup %ymm13, %ymm13
    vmovshdup %ymm14, %ymm14
    vpblendd  $0xAA,  %ymm14, %ymm13, %ymm13 /* prodhi */
    vmovshdup %ymm15, %ymm15
    vmovshdup %ymm12, %ymm12
    vpblendd  $0xAA,  %ymm12, %ymm15, %ymm15 /* mqhi */
    vpsubd    %ymm15, %ymm13, %ymm13        /* W = montred(zeta*Vd) */
    vpaddd    %ymm13, %ymm3,  %ymm14        /* Up = Ud+W */
    vpsubd    %ymm13, %ymm3,  %ymm3         /* Um = Ud-W */
    vpblendd  $0xAA,  %ymm3,  %ymm14, %ymm\A /* even<-Up odd<-Um */
.endm

/* phase A : half H, column off ; t=256,128,64 */
.macro phaseA H, off
    vmovdqa   2048*\H+32*\off+0(%rdi),    %ymm4
    vmovdqa   2048*\H+32*\off+256(%rdi),  %ymm5
    vmovdqa   2048*\H+32*\off+512(%rdi),  %ymm6
    vmovdqa   2048*\H+32*\off+768(%rdi),  %ymm7
    vmovdqa   2048*\H+32*\off+1024(%rdi), %ymm8
    vmovdqa   2048*\H+32*\off+1280(%rdi), %ymm9
    vmovdqa   2048*\H+32*\off+1536(%rdi), %ymm10
    vmovdqa   2048*\H+32*\off+1792(%rdi), %ymm11

    lzf 2+\H
    butterfly_fwd 4,8
    butterfly_fwd 5,9
    butterfly_fwd 6,10
    butterfly_fwd 7,11
    lzf 4+2*\H
    butterfly_fwd 4,6
    butterfly_fwd 5,7
    lzf 4+2*\H+1
    butterfly_fwd 8,10
    butterfly_fwd 9,11
    lzf 8+4*\H
    butterfly_fwd 4,5
    lzf 8+4*\H+1
    butterfly_fwd 6,7
    lzf 8+4*\H+2
    butterfly_fwd 8,9
    lzf 8+4*\H+3
    butterfly_fwd 10,11

    vmovdqa   %ymm4,  2048*\H+32*\off+0(%rdi)
    vmovdqa   %ymm5,  2048*\H+32*\off+256(%rdi)
    vmovdqa   %ymm6,  2048*\H+32*\off+512(%rdi)
    vmovdqa   %ymm7,  2048*\H+32*\off+768(%rdi)
    vmovdqa   %ymm8,  2048*\H+32*\off+1024(%rdi)
    vmovdqa   %ymm9,  2048*\H+32*\off+1280(%rdi)
    vmovdqa   %ymm10, 2048*\H+32*\off+1536(%rdi)
    vmovdqa   %ymm11, 2048*\H+32*\off+1792(%rdi)
.endm

/* phase B : block b ; cross t=32,16,8 then intra t=4,2,1 */
.macro phaseB b
    vmovdqa   256*\b+0(%rdi),   %ymm4
    vmovdqa   256*\b+32(%rdi),  %ymm5
    vmovdqa   256*\b+64(%rdi),  %ymm6
    vmovdqa   256*\b+96(%rdi),  %ymm7
    vmovdqa   256*\b+128(%rdi), %ymm8
    vmovdqa   256*\b+160(%rdi), %ymm9
    vmovdqa   256*\b+192(%rdi), %ymm10
    vmovdqa   256*\b+224(%rdi), %ymm11

    lzf 16+\b
    butterfly_fwd 4,8
    butterfly_fwd 5,9
    butterfly_fwd 6,10
    butterfly_fwd 7,11
    lzf 32+2*\b
    butterfly_fwd 4,6
    butterfly_fwd 5,7
    lzf 32+2*\b+1
    butterfly_fwd 8,10
    butterfly_fwd 9,11
    lzf 64+4*\b
    butterfly_fwd 4,5
    lzf 64+4*\b+1
    butterfly_fwd 6,7
    lzf 64+4*\b+2
    butterfly_fwd 8,9
    lzf 64+4*\b+3
    butterfly_fwd 10,11

    /* t=4 (shuffle8) ; fit[b*8 + 0..3] */
    intra_fwd 4,5,   shuffle8, (\b*8+0)*32
    intra_fwd 6,7,   shuffle8, (\b*8+1)*32
    intra_fwd 8,9,   shuffle8, (\b*8+2)*32
    intra_fwd 10,11, shuffle8, (\b*8+3)*32
    /* t=2 (shuffle4) ; fit[b*8 + 4..7] */
    intra_fwd 4,5,   shuffle4, (\b*8+4)*32
    intra_fwd 6,7,   shuffle4, (\b*8+5)*32
    intra_fwd 8,9,   shuffle4, (\b*8+6)*32
    intra_fwd 10,11, shuffle4, (\b*8+7)*32
    /* t=1 (in-place per-lane) ; fit1[b*8 + 0..7] */
    vmovdqa   qinv(%rip), %ymm1
    butterfly_fwd_t1 4,  (\b*8+0)*32
    butterfly_fwd_t1 5,  (\b*8+1)*32
    butterfly_fwd_t1 6,  (\b*8+2)*32
    butterfly_fwd_t1 7,  (\b*8+3)*32
    butterfly_fwd_t1 8,  (\b*8+4)*32
    butterfly_fwd_t1 9,  (\b*8+5)*32
    butterfly_fwd_t1 10, (\b*8+6)*32
    butterfly_fwd_t1 11, (\b*8+7)*32

    vmovdqa   %ymm4,  256*\b+0(%rdi)
    vmovdqa   %ymm5,  256*\b+32(%rdi)
    vmovdqa   %ymm6,  256*\b+64(%rdi)
    vmovdqa   %ymm7,  256*\b+96(%rdi)
    vmovdqa   %ymm8,  256*\b+128(%rdi)
    vmovdqa   %ymm9,  256*\b+160(%rdi)
    vmovdqa   %ymm10, 256*\b+192(%rdi)
    vmovdqa   %ymm11, 256*\b+224(%rdi)
.endm

    .global ntt_avx_forward
ntt_avx_forward:
    vmovdqa   q(%rip), %ymm0

    /* phase 0 : t=512, zeta = M[1] */
    lzf 1
    mov       %rdi, %rsi
    mov       $64, %ecx
1:
    vmovdqa   (%rsi),     %ymm4
    vmovdqa   2048(%rsi), %ymm5
    butterfly_fwd 4,5
    vmovdqa   %ymm4, (%rsi)
    vmovdqa   %ymm5, 2048(%rsi)
    add       $32, %rsi
    dec       %ecx
    jnz       1b

    phaseA 0,0
    phaseA 0,1
    phaseA 0,2
    phaseA 0,3
    phaseA 0,4
    phaseA 0,5
    phaseA 0,6
    phaseA 0,7
    phaseA 1,0
    phaseA 1,1
    phaseA 1,2
    phaseA 1,3
    phaseA 1,4
    phaseA 1,5
    phaseA 1,6
    phaseA 1,7

    phaseB 0
    phaseB 1
    phaseB 2
    phaseB 3
    phaseB 4
    phaseB 5
    phaseB 6
    phaseB 7
    phaseB 8
    phaseB 9
    phaseB 10
    phaseB 11
    phaseB 12
    phaseB 13
    phaseB 14
    phaseB 15

    ret

    .section .note.GNU-stack,"",@progbits
