/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
/*
 * BiT-128 AVX2 basemul — hand-written assembly.
 * Degree-2 pointwise multiply, mirrors basemul_avx.c exactly.
 * Uses bmul_h_odd[] / bmul_l_odd[] from consts.c.
 *
 *   basemul_asm(int16_t *r, const int16_t *a, const int16_t *b)
 *   basemul_acc_asm(int16_t *r, const int16_t *a, const int16_t *b)
 */

#define Q      %ymm15
#define QINV   %ymm14
#define SWAP   %ymm13
#define BIT_Q       26881
#define BIT_QINV   -26879

.section .rodata
.p2align 5
vQ:     .rept 16
        .short BIT_Q
        .endr
vQINV:  .rept 16
        .short BIT_QINV
        .endr
vSWAP:  .byte 2,3,0,1,6,7,4,5,10,11,8,9,14,15,12,13
        .byte 2,3,0,1,6,7,4,5,10,11,8,9,14,15,12,13

.text

/* Degree-2 Montgomery basemul, result (degree-2 product) into %ymm0.
   rsi=a, rdx=b, rcx=bmul_h_odd, r8=bmul_l_odd.  Clobbers ymm0-4.

   The b1*zeta twiddle uses a Kyber-style fqmul (precomputed zeta + zeta*qinv in
   the odd lane), replacing the old madd + 32->16 reduction + MONT_FIX, and keeps
   b1z signed in (-q,q) — bit-identical to the previous output because the final
   per-coefficient Montgomery reduction collapses the a1*q difference mod q.
     C0 = a0*b0 + (a1)*montred(b1*zeta)   (constant, even lane)
     C1 = a0*b1 + a1*b0                    (X coeff, odd lane) */
.macro BMUL off
    vmovdqa  \off(%rsi), %ymm0          /* A   = (a0,a1)            */
    vmovdqa  \off(%rdx), %ymm1          /* B   = (b0,b1)            */
    vmovdqa  \off(%rcx), %ymm2          /* zeta in odd lane         */
    vmovdqa  \off(%r8),  %ymm3          /* zeta*qinv in odd lane    */
    vpmullw  %ymm3, %ymm1, %ymm3        /* m  = B * zeta_qinv          */
    vpmulhw  %ymm2, %ymm1, %ymm2        /* hi = mulhi_s(B, zeta)       */
    vpmulhuw Q, %ymm3, %ymm3            /* t  = mulhi_u(m, q)          */
    vpsubw   %ymm3, %ymm2, %ymm2        /* b1z = hi - t  (signed)      */
    vpaddw   Q, %ymm2, %ymm4
    vpminuw  %ymm4, %ymm2, %ymm2        /* b1z -> [0,q)  (MONT_FIX)    */
    vpblendw $0xAA, %ymm2, %ymm1, %ymm2 /* B' = (b0, b1z)              */
    vpshufb  SWAP, %ymm1, %ymm3         /* B_swap = (b1, b0)        */
    vpmaddwd %ymm2, %ymm0, %ymm2        /* C0 = a0*b0 + a1*b1z      */
    vpmaddwd %ymm3, %ymm0, %ymm0        /* C1 = a0*b1 + a1*b0       */
    vpmullw  QINV, %ymm2, %ymm4         /* C0 reduce */
    vpsrad   $16, %ymm2, %ymm2
    vpmulhuw Q, %ymm4, %ymm4
    vpsubw   %ymm4, %ymm2, %ymm2        /* r0 (even lane) */
    vpmullw  QINV, %ymm0, %ymm4         /* C1 reduce */
    vpsrad   $16, %ymm0, %ymm0
    vpmulhuw Q, %ymm4, %ymm4
    vpsubw   %ymm4, %ymm0, %ymm0        /* r1 */
    vpslld   $16, %ymm0, %ymm0          /* r1 -> odd lane */
    vpblendw $0xAA, %ymm0, %ymm2, %ymm0 /* out = (r0, r1) */
    vpaddw   Q, %ymm0, %ymm4
    vpminuw  %ymm4, %ymm0, %ymm0        /* MONT_FIX -> [0,q) */
.endm

.global ntt_basemul_raw
#ifdef ASM_ALIASES
.global basemul_asm
basemul_asm:
#endif
ntt_basemul_raw:
    vmovdqa vQ(%rip), Q
    vmovdqa vQINV(%rip), QINV
    vmovdqa vSWAP(%rip), SWAP
    leaq    bmul_h_odd(%rip), %rcx
    leaq    bmul_l_odd(%rip), %r8
    .set    o, 0
    .rept   16
    BMUL    o
    vmovdqa %ymm0, o(%rdi)
    .set    o, o+32
    .endr
    vzeroupper
    ret

.global ntt_basemul_acc_raw
#ifdef ASM_ALIASES
.global basemul_acc_asm
basemul_acc_asm:
#endif
ntt_basemul_acc_raw:
    vmovdqa vQ(%rip), Q
    vmovdqa vQINV(%rip), QINV
    vmovdqa vSWAP(%rip), SWAP
    leaq    bmul_h_odd(%rip), %rcx
    leaq    bmul_l_odd(%rip), %r8
    .set    o, 0
    .rept   16
    BMUL    o
    vmovdqa o(%rdi), %ymm1             /* R */
    vpaddw  %ymm0, %ymm1, %ymm0
    vpsubw  Q, %ymm0, %ymm1
    vpminuw %ymm1, %ymm0, %ymm0
    vmovdqa %ymm0, o(%rdi)
    .set    o, o+32
    .endr
    vzeroupper
    ret

.section .note.GNU-stack,"",@progbits
