/*
 * x86-64 AVX2 assembly ported (GNU-as) from the pq-crystals ML-KEM (Kyber)
 * AVX2 reference assembly. Third-party building block (pq-crystals;
 * public domain / Apache-2.0). NOT an original contribution of this
 * submission and OUTSIDE the scope of the B.3 ownership statement;
 * the upstream authors retain copyright (full license texts: the upstream pq-crystals project).
 */
/*
 * DKE AVX2 basemul for N=256, Q=3329 - ML-KEM packed format
 * GAS format for Linux/WSL
 *
 * Direct port of ML-KEM's basemul.S.
 *
 * void dke_basemul_dkek_avx2_linux(int16_t *r, const int16_t *a,
 *                                    const int16_t *b, const int16_t *qdata);
 *   rdi = r   (output, 512 bytes, 32-byte aligned)
 *   rsi = a   (input,  512 bytes, 32-byte aligned, packed NTT format)
 *   rdx = b   (input,  512 bytes, 32-byte aligned, packed NTT format)
 *   rcx = qdata (dke_qdata, 32-byte aligned)
 *
 * Linux ABI: no ymm register saves needed.
 *
 * Basemul zeta byte offsets from qdata start:
 *   schoolbook 0: qdata + 192 + 176*2 = qdata + 544
 *   schoolbook 1: qdata + 192 + 208*2 = qdata + 608
 *   schoolbook 2: qdata + 192 + 400*2 = qdata + 992
 *   schoolbook 3: qdata + 192 + 432*2 = qdata + 1056
 *
 * GAS operand order: vpmullw src2, src1, dst => dst = src1 * src2
 *                    vpsubw  src2, src1, dst => dst = src1 - src2
 */

/* ---- SCHOOLBOOK macro: port of ML-KEM's schoolbook macro ----
 * off: 0 or 1 (which 64-coefficient block)
 * rdi=output, rsi=first poly, rdx=second poly, rcx=qdata, r9=zeta ptr
 * [rsp-32]: 32-byte temp (red zone, Linux ABI allows 128-byte red zone)
 */
.macro SCHOOLBOOK off
    vmovdqa 32(%rcx), %ymm0                         /* QINV */
    vmovdqa (64*\off+ 0)*2(%rsi), %ymm1             /* a0 */
    vmovdqa (64*\off+16)*2(%rsi), %ymm2             /* b0 */
    vmovdqa (64*\off+32)*2(%rsi), %ymm3             /* a1 */
    vmovdqa (64*\off+48)*2(%rsi), %ymm4             /* b1 */

    vpmullw %ymm1, %ymm0, %ymm9                     /* a0.lo = a0 * QINV */
    vpmullw %ymm2, %ymm0, %ymm10                    /* b0.lo */
    vpmullw %ymm3, %ymm0, %ymm11                    /* a1.lo */
    vpmullw %ymm4, %ymm0, %ymm12                    /* b1.lo */

    vmovdqa (64*\off+ 0)*2(%rdx), %ymm5             /* c0 */
    vmovdqa (64*\off+16)*2(%rdx), %ymm6             /* d0 */

    vpmulhw %ymm1, %ymm5, %ymm13                    /* a0c0.hi */
    vpmulhw %ymm1, %ymm6, %ymm1                     /* a0d0.hi [ymm1 reused] */
    vpmulhw %ymm2, %ymm5, %ymm14                    /* b0c0.hi */
    vpmulhw %ymm2, %ymm6, %ymm2                     /* b0d0.hi [ymm2 reused] */

    vmovdqa (64*\off+32)*2(%rdx), %ymm7             /* c1 */
    vmovdqa (64*\off+48)*2(%rdx), %ymm8             /* d1 */

    vpmulhw %ymm3, %ymm7, %ymm15                    /* a1c1.hi */
    vpmulhw %ymm3, %ymm8, %ymm3                     /* a1d1.hi [ymm3 reused] */
    vpmulhw %ymm4, %ymm7, %ymm0                     /* b1c1.hi [ymm0 reused] */
    vpmulhw %ymm4, %ymm8, %ymm4                     /* b1d1.hi [ymm4 reused] */

    /* Save a0c0.hi to red zone */
    vmovdqu %ymm13, -32(%rsp)

    /* Compute lo parts */
    vpmullw %ymm9,  %ymm5, %ymm13                   /* a0c0.lo */
    vpmullw %ymm9,  %ymm6, %ymm9                    /* a0d0.lo */
    vpmullw %ymm10, %ymm5, %ymm5                    /* b0c0.lo */
    vpmullw %ymm10, %ymm6, %ymm10                   /* b0d0.lo */
    vpmullw %ymm11, %ymm7, %ymm6                    /* a1c1.lo */
    vpmullw %ymm11, %ymm8, %ymm11                   /* a1d1.lo */
    vpmullw %ymm12, %ymm7, %ymm7                    /* b1c1.lo */
    vpmullw %ymm12, %ymm8, %ymm12                   /* b1d1.lo */

    /* Load Q */
    vmovdqa (%rcx), %ymm8

    /* Montgomery reduce: mulhi(lo, Q) */
    vpmulhw %ymm13, %ymm8, %ymm13
    vpmulhw %ymm9,  %ymm8, %ymm9
    vpmulhw %ymm5,  %ymm8, %ymm5
    vpmulhw %ymm10, %ymm8, %ymm10
    vpmulhw %ymm6,  %ymm8, %ymm6
    vpmulhw %ymm11, %ymm8, %ymm11
    vpmulhw %ymm7,  %ymm8, %ymm7
    vpmulhw %ymm12, %ymm8, %ymm12

    /* -Montgomery(x) = mulhi(lo,Q) - hi */
    vpsubw  -32(%rsp), %ymm13, %ymm13               /* -a0c0 */
    vpsubw  %ymm1,  %ymm9,  %ymm9                   /* -a0d0 */
    vpsubw  %ymm14, %ymm5,  %ymm5                   /* -b0c0 */
    vpsubw  %ymm2,  %ymm10, %ymm10                  /* -b0d0 */
    vpsubw  %ymm15, %ymm6,  %ymm6                   /* -a1c1 */
    vpsubw  %ymm3,  %ymm11, %ymm11                  /* -a1d1 */
    vpsubw  %ymm0,  %ymm7,  %ymm7                   /* -b1c1 */
    vpsubw  %ymm4,  %ymm12, %ymm12                  /* -b1d1 */

    /* Load zeta: zl at r9, zh at r9+32 */
    vmovdqa (%r9),    %ymm0                          /* zl */
    vmovdqa 32(%r9),  %ymm1                          /* zh */

    /* Multiply -b0d0 and -b1d1 by zeta (Montgomery) */
    vpmullw %ymm10, %ymm0, %ymm2
    vpmullw %ymm12, %ymm0, %ymm3
    vpmulhw %ymm10, %ymm1, %ymm10
    vpmulhw %ymm12, %ymm1, %ymm12
    vpmulhw %ymm2,  %ymm8, %ymm2
    vpmulhw %ymm3,  %ymm8, %ymm3
    vpsubw  %ymm2,  %ymm10, %ymm10                  /* -rb0d0 */
    vpsubw  %ymm3,  %ymm12, %ymm12                  /* -rb1d1 */

    /* Accumulate */
    vpaddw  %ymm9,  %ymm5,  %ymm9                   /* -(a0d0+b0c0) */
    vpaddw  %ymm11, %ymm7,  %ymm11                  /* -(a1d1+b1c1) */
    vpsubw  %ymm13, %ymm10, %ymm13                  /* a0c0 - rb0d0 */
    vpsubw  %ymm6,  %ymm12, %ymm6                   /* a1c1 - rb1d1 */

    /* Store */
    vmovdqa %ymm13, (64*\off+ 0)*2(%rdi)
    vmovdqa %ymm9,  (64*\off+16)*2(%rdi)
    vmovdqa %ymm6,  (64*\off+32)*2(%rdi)
    vmovdqa %ymm11, (64*\off+48)*2(%rdi)
.endm

.text
.globl dke_basemul_dkek_avx2_linux
.type dke_basemul_dkek_avx2_linux, @function
.align 32
dke_basemul_dkek_avx2_linux:
    /* r9 = qdata + 544 = _ZETAS_EXP + 176*2 */
    lea     544(%rcx), %r9

    /* schoolbook 0: a[0..63] x b[0..63] */
    SCHOOLBOOK 0

    add     $64, %r9

    /* schoolbook 1: a[64..127] x b[64..127] */
    SCHOOLBOOK 1

    /* advance to second half */
    add     $256, %rsi
    add     $256, %rdx
    add     $256, %rdi

    /* r9: current=qdata+608, need qdata+992, delta=384 */
    add     $384, %r9

    /* schoolbook 2: a[128..191] x b[128..191] */
    SCHOOLBOOK 0

    add     $64, %r9

    /* schoolbook 3: a[192..255] x b[192..255] */
    SCHOOLBOOK 1

    vzeroupper
    ret
.size dke_basemul_dkek_avx2_linux, .-dke_basemul_dkek_avx2_linux

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
