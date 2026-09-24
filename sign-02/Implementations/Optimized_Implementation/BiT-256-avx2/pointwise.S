/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
/* ==================================================================
   BIT degree-2 CRT pointwise multiply – AVX2, q=119297, n=512.

   For each group of 4 coefficients (root = M[128+group]):
     r0 = a0*b0 + (a1*b1)*root
     r1 = a0*b1 + a1*b0
     r2 = a2*b2 + (a3*b3)*(-root)
     r3 = a2*b3 + a3*b2
   (all products are Montgomery multiplications, result in [0,q)).

   Per iteration 8 coefficients (2 groups) are processed; the root
   vector bmul[8*it] holds, in even lanes, [r0, q-r0, r1, q-r1].
   ================================================================== */

#include "consts.h"

.text

/* Montgomery multiply of even 32-bit lanes:  d = montred(x*y), even lanes,
   odd lanes cleared.  Uses ymm0=qinv, ymm1=q, scratch ymm8. */
.macro mr_even d,x,y
    vpmuldq  %ymm\x, %ymm\y, %ymm\d
    vpmuldq  %ymm0, %ymm\d, %ymm8
    vpmuldq  %ymm1, %ymm8, %ymm8
    vpsubq   %ymm8, %ymm\d, %ymm\d
    vpsrlq   $32, %ymm\d, %ymm\d
.endm

/* correct signed montred result (-q,q) -> [0,q) ; scratch ymm13 */
.macro cmod v
    vpsrad   $31, %ymm\v, %ymm13
    vpand    %ymm1, %ymm13, %ymm13
    vpaddd   %ymm13, %ymm\v, %ymm\v
.endm

/* reduce a in [0,2q) -> [0,q) ; scratch ymm13 */
.macro rmodq v
    vpsubd   %ymm1, %ymm\v, %ymm\v
    vpsrad   $31, %ymm\v, %ymm13
    vpand    %ymm1, %ymm13, %ymm13
    vpaddd   %ymm13, %ymm\v, %ymm\v
.endm

/* compute one 8-coefficient degree-2 product; result in ymm7 (natural order).
   ymm2=A, ymm3=B loaded by caller ; ymm4=RV loaded by caller. */
.macro pw_core
    vmovshdup %ymm2, %ymm5            /* a_odd: a1,a3,a5,a7 in even lanes */
    vmovshdup %ymm3, %ymm6            /* b_odd: b1,b3,b5,b7 in even lanes */

    /* b1z = montred(b_odd * RV) = b1·ζ·R⁻¹   (-q,q) in even lanes */
    mr_even 7,6,4

    /* even out = montred(a0·b0 + a1·b1z)
       (fused: sum the two 64-bit products, then ONE Montgomery reduction —
        algebraically identical to montred(a0b0)+montred(montred(a1b1)·ζ)) */
    vpmuldq  %ymm2, %ymm3, %ymm8      /* a0·b0  (64-bit, even qwords) */
    vpmuldq  %ymm5, %ymm7, %ymm9      /* a1·b1z (64-bit) */
    vpaddq   %ymm9, %ymm8, %ymm8      /* esum */
    vpmuldq  %ymm0, %ymm8, %ymm9      /* m = esum_lo · qinv */
    vpmuldq  %ymm1, %ymm9, %ymm9      /* m·q */
    vpsubq   %ymm9, %ymm8, %ymm8      /* esum − m·q */
    vpsrlq   $32, %ymm8, %ymm8        /* E ∈ (-q,q) in even lanes */

    /* odd out = montred(a0·b1 + a1·b0)  (fused) */
    vpmuldq  %ymm2, %ymm6, %ymm10     /* a0·b1 */
    vpmuldq  %ymm5, %ymm3, %ymm11     /* a1·b0 */
    vpaddq   %ymm11, %ymm10, %ymm10   /* osum */
    vpmuldq  %ymm0, %ymm10, %ymm11    /* m */
    vpmuldq  %ymm1, %ymm11, %ymm11    /* m·q */
    vpsubq   %ymm11, %ymm10, %ymm10
    vpsrlq   $32, %ymm10, %ymm10      /* O ∈ (-q,q) in even lanes */

    /* interleave: even lanes from E (ymm8), odd lanes from O (ymm10) */
    vpsllq   $32, %ymm10, %ymm10
    vpblendd $0xAA, %ymm10, %ymm8, %ymm7
.endm

/* ------------------------------------------------------------------
   ntt_basemul_asm(r, a, b) :  r = degree2 basemul(a,b)
   rdi=r, rsi=a, rdx=b
   ------------------------------------------------------------------ */
.global ntt_basemul_asm
.global ntt_basemul_raw
ntt_basemul_raw:
ntt_basemul_asm:
    vmovdqa  qinv(%rip), %ymm0
    vmovdqa  q(%rip), %ymm1
    leaq     bmul(%rip), %r8
    xor      %eax, %eax
1:
    vmovdqa  (%rsi), %ymm2
    vmovdqa  (%rdx), %ymm3
    vmovdqa  (%r8), %ymm4
    pw_core
    vmovdqa  %ymm7, (%rdi)
    add      $32, %rsi
    add      $32, %rdx
    add      $32, %rdi
    add      $32, %r8
    inc      %eax
    cmp      $64, %eax
    jb       1b
    ret

/* ------------------------------------------------------------------
   ntt_basemul_acc_asm(r, a, b) : r += degree2 basemul(a,b)
   rdi=r (in/out), rsi=a, rdx=b
   ------------------------------------------------------------------ */
.global ntt_basemul_acc_asm
.global ntt_basemul_acc_raw
ntt_basemul_acc_raw:
ntt_basemul_acc_asm:
    vmovdqa  qinv(%rip), %ymm0
    vmovdqa  q(%rip), %ymm1
    leaq     bmul(%rip), %r8
    xor      %eax, %eax
1:
    vmovdqa  (%rsi), %ymm2
    vmovdqa  (%rdx), %ymm3
    vmovdqa  (%r8), %ymm4
    pw_core
    vmovdqa  (%rdi), %ymm14           /* r_old (lazily reduced) */
    vpaddd   %ymm14, %ymm7, %ymm7     /* accumulate; stays well within int32,
                                         downstream reduces mod q */
    vmovdqa  %ymm7, (%rdi)
    add      $32, %rsi
    add      $32, %rdx
    add      $32, %rdi
    add      $32, %r8
    inc      %eax
    cmp      $64, %eax
    jb       1b
    ret

.section .note.GNU-stack,"",@progbits
