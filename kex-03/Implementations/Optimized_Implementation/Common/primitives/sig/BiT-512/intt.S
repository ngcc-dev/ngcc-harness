/* ============================================================
   BiT-512  inverse NTT  (complete, n=1024, q=520193)  — AVX2 asm
   Gentleman-Sande, signed Montgomery, register-blocked; then scale
   by N^-1 and emit signed canonical output in [-q/2, q/2).

   phaseB : intra t=1,2,4 (shuffle2/4/8) + cross t=8,16,32
   phaseA : cross t=64,128,256
   phase0 : t=512 ; then N^-1 scale + canonicalize

   ymm0=q  ymm1=zl  ymm2=zh  ymm3,15=transpose tmp  ymm4..11=data  ymm12,13,14=scratch
   ============================================================ */

#include "shuffle.inc"

    .text

/* signed Montgomery GS butterfly:
   l := l + h ,  h := zeta*(l - h)   (montgomery). zl=ymm1 zh=ymm2 q=ymm0 ; scratch 12,13,14 */
.macro butterfly_inv l,h
    vpaddd    %ymm\h, %ymm\l, %ymm13
    vpsubd    %ymm\h, %ymm\l, %ymm14
    vpmuldq   %ymm2, %ymm14, %ymm12
    vmovshdup %ymm14, %ymm\l
    vpmuldq   %ymm2, %ymm\l, %ymm\h
    vpmuldq   %ymm1, %ymm14, %ymm14
    vpmuldq   %ymm0, %ymm14, %ymm14
    vpmuldq   %ymm1, %ymm\l, %ymm\l
    vpmuldq   %ymm0, %ymm\l, %ymm\l
    vmovshdup %ymm12, %ymm12
    vmovshdup %ymm14, %ymm14
    vmovshdup %ymm\h, %ymm\h
    vpblendd  $0xAA, %ymm\h, %ymm12, %ymm12
    vpblendd  $0xAA, %ymm\l, %ymm14, %ymm14
    vpsubd    %ymm14, %ymm12, %ymm\h
    vmovdqa   %ymm13, %ymm\l
.endm

/* broadcast zl/zh for inverse zeta index \zi */
.macro lzi zi
    vpbroadcastd bit512_zinv_l+(\zi)*4(%rip), %ymm1
    vpbroadcastd bit512_zinv_h+(\zi)*4(%rip), %ymm2
.endm

/* intra GS butterfly (t=2,4) on pair (\p,\q) via transpose \shuf, vector zeta from iit */
.macro intra_inv p, q, shuf, off
    \shuf \p,\q,3,15
    vmovdqa   bit512_iit_l+(\off)(%rip), %ymm1
    vmovdqa   bit512_iit_h+(\off)(%rip), %ymm2
    butterfly_inv 3,15
    \shuf 3,15,\p,\q
.endm

/* intra GS t=1 (stride 1) in-place, per-lane zeta:
   even lanes = U+V, odd lanes = montred((U-V)*zeta).
   qinv(+) in ymm1, zeta(zh) vector from bit512_iit1_h at \off ; scratch 5,6,7,12..15 */
.macro butterfly_inv_t1 A, off
    vmovdqa   bit512_iit1_h+(\off)(%rip), %ymm2
    vpshufd   $0xA0, %ymm\A, %ymm3          /* U = even dup */
    vpshufd   $0xF5, %ymm\A, %ymm12         /* V = odd dup  */
    vpaddd    %ymm12, %ymm3,  %ymm15        /* sum = U+V (keep) */
    vpsubd    %ymm12, %ymm3,  %ymm3         /* diff = U-V */
    vpmuldq   %ymm2,  %ymm3,  %ymm13        /* pe = zh_e*diff_e */
    vmovshdup %ymm3,  %ymm14                /* diff_odd */
    vmovshdup %ymm2,  %ymm12                /* zh_odd */
    vpmuldq   %ymm12, %ymm14, %ymm14        /* po */
    vpmuldq   %ymm1,  %ymm13, %ymm12        /* m_e */
    vpmuldq   %ymm1,  %ymm14, %ymm3         /* m_o */
    vpmuldq   %ymm0,  %ymm12, %ymm12
    vpmuldq   %ymm0,  %ymm3,  %ymm3
    vmovshdup %ymm13, %ymm13
    vmovshdup %ymm14, %ymm14
    vpblendd  $0xAA,  %ymm14, %ymm13, %ymm13 /* prodhi */
    vmovshdup %ymm12, %ymm12
    vmovshdup %ymm3,  %ymm3
    vpblendd  $0xAA,  %ymm3,  %ymm12, %ymm12 /* mqhi */
    vpsubd    %ymm12, %ymm13, %ymm13        /* W = montred(zeta*diff) */
    vpblendd  $0xAA,  %ymm13, %ymm15, %ymm\A /* even<-sum odd<-W */
.endm

/* inverse phase B : block b */
.macro iphaseB b
    vmovdqa   256*\b+0(%rdi),   %ymm4
    vmovdqa   256*\b+32(%rdi),  %ymm5
    vmovdqa   256*\b+64(%rdi),  %ymm6
    vmovdqa   256*\b+96(%rdi),  %ymm7
    vmovdqa   256*\b+128(%rdi), %ymm8
    vmovdqa   256*\b+160(%rdi), %ymm9
    vmovdqa   256*\b+192(%rdi), %ymm10
    vmovdqa   256*\b+224(%rdi), %ymm11

    /* t=1 (in-place per-lane) ; iit1[b*8 + 0..7] */
    vmovdqa   bit512_qinv(%rip), %ymm1
    butterfly_inv_t1 4,  (\b*8+0)*32
    butterfly_inv_t1 5,  (\b*8+1)*32
    butterfly_inv_t1 6,  (\b*8+2)*32
    butterfly_inv_t1 7,  (\b*8+3)*32
    butterfly_inv_t1 8,  (\b*8+4)*32
    butterfly_inv_t1 9,  (\b*8+5)*32
    butterfly_inv_t1 10, (\b*8+6)*32
    butterfly_inv_t1 11, (\b*8+7)*32
    /* t=2 (shuffle4) ; iit[b*8 + 0..3] */
    intra_inv 4,5,   shuffle4, (\b*8+0)*32
    intra_inv 6,7,   shuffle4, (\b*8+1)*32
    intra_inv 8,9,   shuffle4, (\b*8+2)*32
    intra_inv 10,11, shuffle4, (\b*8+3)*32
    /* t=4 (shuffle8) ; iit[b*8 + 4..7] */
    intra_inv 4,5,   shuffle8, (\b*8+4)*32
    intra_inv 6,7,   shuffle8, (\b*8+5)*32
    intra_inv 8,9,   shuffle8, (\b*8+6)*32
    intra_inv 10,11, shuffle8, (\b*8+7)*32

    /* t=8 (dist 1) */
    lzi 64+4*\b
    butterfly_inv 4,5
    lzi 64+4*\b+1
    butterfly_inv 6,7
    lzi 64+4*\b+2
    butterfly_inv 8,9
    lzi 64+4*\b+3
    butterfly_inv 10,11
    /* t=16 (dist 2) */
    lzi 32+2*\b
    butterfly_inv 4,6
    butterfly_inv 5,7
    lzi 32+2*\b+1
    butterfly_inv 8,10
    butterfly_inv 9,11
    /* t=32 (dist 4) */
    lzi 16+\b
    butterfly_inv 4,8
    butterfly_inv 5,9
    butterfly_inv 6,10
    butterfly_inv 7,11

    vmovdqa   %ymm4,  256*\b+0(%rdi)
    vmovdqa   %ymm5,  256*\b+32(%rdi)
    vmovdqa   %ymm6,  256*\b+64(%rdi)
    vmovdqa   %ymm7,  256*\b+96(%rdi)
    vmovdqa   %ymm8,  256*\b+128(%rdi)
    vmovdqa   %ymm9,  256*\b+160(%rdi)
    vmovdqa   %ymm10, 256*\b+192(%rdi)
    vmovdqa   %ymm11, 256*\b+224(%rdi)
.endm

/* inverse phase A : half H, column off ; cross t=64,128,256 */
.macro iphaseA H, off
    vmovdqa   2048*\H+32*\off+0(%rdi),    %ymm4
    vmovdqa   2048*\H+32*\off+256(%rdi),  %ymm5
    vmovdqa   2048*\H+32*\off+512(%rdi),  %ymm6
    vmovdqa   2048*\H+32*\off+768(%rdi),  %ymm7
    vmovdqa   2048*\H+32*\off+1024(%rdi), %ymm8
    vmovdqa   2048*\H+32*\off+1280(%rdi), %ymm9
    vmovdqa   2048*\H+32*\off+1536(%rdi), %ymm10
    vmovdqa   2048*\H+32*\off+1792(%rdi), %ymm11

    lzi 8+4*\H
    butterfly_inv 4,5
    lzi 8+4*\H+1
    butterfly_inv 6,7
    lzi 8+4*\H+2
    butterfly_inv 8,9
    lzi 8+4*\H+3
    butterfly_inv 10,11
    lzi 4+2*\H
    butterfly_inv 4,6
    butterfly_inv 5,7
    lzi 4+2*\H+1
    butterfly_inv 8,10
    butterfly_inv 9,11
    lzi 2+\H
    butterfly_inv 4,8
    butterfly_inv 5,9
    butterfly_inv 6,10
    butterfly_inv 7,11

    vmovdqa   %ymm4,  2048*\H+32*\off+0(%rdi)
    vmovdqa   %ymm5,  2048*\H+32*\off+256(%rdi)
    vmovdqa   %ymm6,  2048*\H+32*\off+512(%rdi)
    vmovdqa   %ymm7,  2048*\H+32*\off+768(%rdi)
    vmovdqa   %ymm8,  2048*\H+32*\off+1024(%rdi)
    vmovdqa   %ymm9,  2048*\H+32*\off+1280(%rdi)
    vmovdqa   %ymm10, 2048*\H+32*\off+1536(%rdi)
    vmovdqa   %ymm11, 2048*\H+32*\off+1792(%rdi)
.endm

    .global ntt_avx_inverse
ntt_avx_inverse:
    vmovdqa   bit512_q(%rip), %ymm0

    iphaseB 0
    iphaseB 1
    iphaseB 2
    iphaseB 3
    iphaseB 4
    iphaseB 5
    iphaseB 6
    iphaseB 7
    iphaseB 8
    iphaseB 9
    iphaseB 10
    iphaseB 11
    iphaseB 12
    iphaseB 13
    iphaseB 14
    iphaseB 15

    iphaseA 0,0
    iphaseA 0,1
    iphaseA 0,2
    iphaseA 0,3
    iphaseA 0,4
    iphaseA 0,5
    iphaseA 0,6
    iphaseA 0,7
    iphaseA 1,0
    iphaseA 1,1
    iphaseA 1,2
    iphaseA 1,3
    iphaseA 1,4
    iphaseA 1,5
    iphaseA 1,6
    iphaseA 1,7

    /* phase 0 : t=512, zeta = Mn[1] */
    lzi 1
    mov       %rdi, %rsi
    mov       $64, %ecx
1:
    vmovdqa   (%rsi),     %ymm4
    vmovdqa   2048(%rsi), %ymm5
    butterfly_inv 4,5
    vmovdqa   %ymm4, (%rsi)
    vmovdqa   %ymm5, 2048(%rsi)
    add       $32, %rsi
    dec       %ecx
    jnz       1b

    /* final : x = montred(x * N^-1) then -> [-q/2, q/2) */
    vmovdqa   bit512_ninv_l(%rip), %ymm1
    vmovdqa   bit512_ninv_h(%rip), %ymm2
    mov       %rdi, %rsi
    mov       $128, %ecx
2:
    vmovdqa   (%rsi), %ymm4
    vpmuldq   %ymm1, %ymm4, %ymm13
    vmovshdup %ymm4, %ymm12
    vpmuldq   %ymm1, %ymm12, %ymm14
    vpmuldq   %ymm2, %ymm4, %ymm4
    vpmuldq   %ymm2, %ymm12, %ymm12
    vpmuldq   %ymm0, %ymm13, %ymm13
    vpmuldq   %ymm0, %ymm14, %ymm14
    vmovshdup %ymm4, %ymm4
    vpblendd  $0xAA, %ymm12, %ymm4, %ymm4
    vmovshdup %ymm13, %ymm13
    vpblendd  $0xAA, %ymm14, %ymm13, %ymm13
    vpsubd    %ymm13, %ymm4, %ymm4          /* montred in (-q,q) */
    vpsrad    $31, %ymm4, %ymm13            /* -> [0,q) */
    vpand     %ymm0, %ymm13, %ymm13
    vpaddd    %ymm13, %ymm4, %ymm4
    vpcmpgtd  bit512_qhalf(%rip), %ymm4, %ymm13  /* -> [-q/2,q/2) */
    vpand     %ymm0, %ymm13, %ymm13
    vpsubd    %ymm13, %ymm4, %ymm4
    vmovdqa   %ymm4, (%rsi)
    add       $32, %rsi
    dec       %ecx
    jnz       2b

    ret

    .section .note.GNU-stack,"",@progbits
