/*
 * x86-64 AVX2 assembly ported (GNU-as) from the pq-crystals ML-KEM (Kyber)
 * AVX2 reference assembly. Third-party building block (pq-crystals;
 * public domain / Apache-2.0). NOT an original contribution of this
 * submission and OUTSIDE the scope of the B.3 ownership statement;
 * the upstream authors retain copyright (full license texts: the upstream pq-crystals project).
 */
/*
 * DKE AVX2 NTT - packed-NTT full pipeline for N=256, Q=3329
 * GAS format for Linux/WSL
 *
 * void dke_ntt_dkek_avx2_unpack_linux(int16_t *r, const int16_t *qdata);
 *   rdi = r (512 bytes, 32-byte aligned)
 *   rsi = qdata (dke_qdata, 32-byte aligned)
 *
 * qdata layout: [0]=16xQ, [32]=16xQINV, [64]=16xV, [192]=_ZETAS_EXP
 *
 * Linux ABI: no ymm register saves needed.
 * ymm0 = Q, ymm2 = zeta_hi, ymm15 = zeta_lo, ymm12..15 = temps
 */

/* ---- shuffle macros ---- */
/* shuffle8: r2={r0[low],r1[low]}, r3={r0[high],r1[high]} */
.macro SHUFFLE8 r0, r1, r2, r3
    vperm2i128 $0x20, %ymm\r1, %ymm\r0, %ymm\r2
    vperm2i128 $0x31, %ymm\r1, %ymm\r0, %ymm\r3
.endm

/* shuffle4: interleave 64-bit halves */
.macro SHUFFLE4 r0, r1, r2, r3
    vpunpcklqdq %ymm\r1, %ymm\r0, %ymm\r2
    vpunpckhqdq %ymm\r1, %ymm\r0, %ymm\r3
.endm

/* shuffle2: interleave 32-bit halves */
.macro SHUFFLE2 r0, r1, r2, r3
    vmovsldup %ymm\r1, %ymm\r2
    vpblendd $0xAA, %ymm\r2, %ymm\r0, %ymm\r2
    vpsrlq $32, %ymm\r0, %ymm\r0
    vpblendd $0xAA, %ymm\r1, %ymm\r0, %ymm\r3
.endm

/* shuffle1: interleave 16-bit halves */
.macro SHUFFLE1 r0, r1, r2, r3
    vpslld $16, %ymm\r1, %ymm\r2
    vpblendw $0xAA, %ymm\r2, %ymm\r0, %ymm\r2
    vpsrld $16, %ymm\r0, %ymm\r0
    vpblendw $0xAA, %ymm\r1, %ymm\r0, %ymm\r3
.endm

/* ---- FQMUL4: Montgomery multiply 4 pairs ----
 * zl=ymm15 (default), zh=ymm2 (default)
 * rh0..rh3: input/output; ymm12..15 = temps */
.macro FQMUL4 rh0, rh1, rh2, rh3, zl0=15, zl1=15, zh0=2, zh1=2
    vpmullw %ymm\zl0, %ymm\rh0, %ymm12
    vpmullw %ymm\zl0, %ymm\rh1, %ymm13
    vpmullw %ymm\zl1, %ymm\rh2, %ymm14
    vpmullw %ymm\zl1, %ymm\rh3, %ymm15
    vpmulhw %ymm\zh0, %ymm\rh0, %ymm\rh0
    vpmulhw %ymm\zh0, %ymm\rh1, %ymm\rh1
    vpmulhw %ymm\zh1, %ymm\rh2, %ymm\rh2
    vpmulhw %ymm\zh1, %ymm\rh3, %ymm\rh3
.endm

/* ---- FQREDUCE: complete Montgomery reduction ---- */
.macro FQREDUCE
    vpmulhw %ymm0, %ymm12, %ymm12
    vpmulhw %ymm0, %ymm13, %ymm13
    vpmulhw %ymm0, %ymm14, %ymm14
    vpmulhw %ymm0, %ymm15, %ymm15
.endm

/* ---- CTUPDATE: CT butterfly on 8 registers ---- */
.macro CTUPDATE rln, rl0, rl1, rl2, rl3, rh0, rh1, rh2, rh3
    vpaddw %ymm\rh0, %ymm\rl0, %ymm\rln
    vpsubw %ymm\rh0, %ymm\rl0, %ymm\rh0
    vpaddw %ymm\rh1, %ymm\rl1, %ymm\rl0
    vpsubw %ymm\rh1, %ymm\rl1, %ymm\rh1
    vpaddw %ymm\rh2, %ymm\rl2, %ymm\rl1
    vpsubw %ymm\rh2, %ymm\rl2, %ymm\rh2
    vpaddw %ymm\rh3, %ymm\rl3, %ymm\rl2
    vpsubw %ymm\rh3, %ymm\rl3, %ymm\rh3
    vpsubw %ymm12, %ymm\rln, %ymm\rln
    vpaddw %ymm12, %ymm\rh0, %ymm\rh0
    vpsubw %ymm13, %ymm\rl0, %ymm\rl0
    vpaddw %ymm13, %ymm\rh1, %ymm\rh1
    vpsubw %ymm14, %ymm\rl1, %ymm\rl1
    vpaddw %ymm14, %ymm\rh2, %ymm\rh2
    vpsubw %ymm15, %ymm\rl2, %ymm\rl2
    vpaddw %ymm15, %ymm\rh3, %ymm\rh3
.endm

/* ---- NTT_LEVEL0: layer 0 (len=128) ----
 * off: 0 or 1 */
.macro NTT_LEVEL0 off
    vpbroadcastq 192(%rsi), %ymm15
    vmovdqa (64*\off+128)*2(%rdi), %ymm8
    vmovdqa (64*\off+144)*2(%rdi), %ymm9
    vmovdqa (64*\off+160)*2(%rdi), %ymm10
    vmovdqa (64*\off+176)*2(%rdi), %ymm11
    vpbroadcastq 200(%rsi), %ymm2

    FQMUL4 8, 9, 10, 11

    vmovdqa (64*\off+  0)*2(%rdi), %ymm4
    vmovdqa (64*\off+ 16)*2(%rdi), %ymm5
    vmovdqa (64*\off+ 32)*2(%rdi), %ymm6
    vmovdqa (64*\off+ 48)*2(%rdi), %ymm7

    FQREDUCE
    CTUPDATE 3, 4, 5, 6, 7, 8, 9, 10, 11

    vmovdqa %ymm3,  (64*\off+  0)*2(%rdi)
    vmovdqa %ymm4,  (64*\off+ 16)*2(%rdi)
    vmovdqa %ymm5,  (64*\off+ 32)*2(%rdi)
    vmovdqa %ymm6,  (64*\off+ 48)*2(%rdi)
    vmovdqa %ymm8,  (64*\off+128)*2(%rdi)
    vmovdqa %ymm9,  (64*\off+144)*2(%rdi)
    vmovdqa %ymm10, (64*\off+160)*2(%rdi)
    vmovdqa %ymm11, (64*\off+176)*2(%rdi)
.endm

/* ---- NTT_LEVELS1T6: layers 1-6 fused in-register ----
 * off: 0 or 1 */
.macro NTT_LEVELS1T6 off
    vmovdqa (128*\off+  0)(%rdi), %ymm4
    vmovdqa (128*\off+ 32)(%rdi), %ymm5
    vmovdqa (128*\off+ 64)(%rdi), %ymm6
    vmovdqa (128*\off+ 96)(%rdi), %ymm7
    vmovdqa (128*\off+128)(%rdi), %ymm8
    vmovdqa (128*\off+160)(%rdi), %ymm9
    vmovdqa (128*\off+192)(%rdi), %ymm10
    vmovdqa (128*\off+224)(%rdi), %ymm11

    /* Level 1 */
    vmovdqa (192 + 224*(1-\off)*2 + 16*2)(%rsi), %ymm15
    vmovdqa (192 + 224*(1-\off)*2 + 32*2)(%rsi), %ymm2
    FQMUL4 8, 9, 10, 11
    FQREDUCE
    CTUPDATE 3, 4, 5, 6, 7, 8, 9, 10, 11

    SHUFFLE8 5, 10, 7, 10
    SHUFFLE8 6, 11, 5, 11

    /* Level 2 */
    vmovdqa (192 + 224*(1-\off)*2 + 48*2)(%rsi), %ymm15
    vmovdqa (192 + 224*(1-\off)*2 + 64*2)(%rsi), %ymm2
    FQMUL4 7, 10, 5, 11

    SHUFFLE8 3, 8, 6, 8
    SHUFFLE8 4, 9, 3, 9

    FQREDUCE
    CTUPDATE 4, 6, 8, 3, 9, 7, 10, 5, 11

    SHUFFLE4 8, 5, 9, 5
    SHUFFLE4 3, 11, 8, 11

    /* Level 3 */
    vmovdqa (192 + 224*(1-\off)*2 + 80*2)(%rsi), %ymm15
    vmovdqa (192 + 224*(1-\off)*2 + 96*2)(%rsi), %ymm2
    FQMUL4 9, 5, 8, 11

    SHUFFLE4 4, 7, 3, 7
    SHUFFLE4 6, 10, 4, 10

    FQREDUCE
    CTUPDATE 6, 3, 7, 4, 10, 9, 5, 8, 11

    SHUFFLE2 7, 8, 10, 8
    SHUFFLE2 4, 11, 7, 11

    /* Level 4 */
    vmovdqa (192 + 224*(1-\off)*2 + 112*2)(%rsi), %ymm15
    vmovdqa (192 + 224*(1-\off)*2 + 128*2)(%rsi), %ymm2
    FQMUL4 10, 8, 7, 11

    SHUFFLE2 6, 9, 4, 9
    SHUFFLE2 3, 5, 6, 5

    FQREDUCE
    CTUPDATE 3, 4, 9, 6, 5, 10, 8, 7, 11

    SHUFFLE1 9, 7, 5, 7
    SHUFFLE1 6, 11, 9, 11

    /* Level 5 */
    vmovdqa (192 + 224*(1-\off)*2 + 144*2)(%rsi), %ymm15
    vmovdqa (192 + 224*(1-\off)*2 + 160*2)(%rsi), %ymm2
    FQMUL4 5, 7, 9, 11

    SHUFFLE1 3, 10, 6, 10
    SHUFFLE1 4, 8, 3, 8

    FQREDUCE
    CTUPDATE 4, 6, 10, 3, 8, 5, 7, 9, 11

    /* Level 6 */
    vmovdqa (192 + 224*(1-\off)*2 + 176*2)(%rsi), %ymm14
    vmovdqa (192 + 224*(1-\off)*2 + 208*2)(%rsi), %ymm15
    vmovdqa (192 + 224*(1-\off)*2 + 192*2)(%rsi), %ymm8
    vmovdqa (192 + 224*(1-\off)*2 + 224*2)(%rsi), %ymm2
    FQMUL4 10, 3, 9, 11, 14, 15, 8, 2

    FQREDUCE
    CTUPDATE 8, 4, 6, 5, 7, 10, 3, 9, 11

    vmovdqa %ymm8,  (128*\off+  0)(%rdi)
    vmovdqa %ymm4,  (128*\off+ 32)(%rdi)
    vmovdqa %ymm10, (128*\off+ 64)(%rdi)
    vmovdqa %ymm3,  (128*\off+ 96)(%rdi)
    vmovdqa %ymm6,  (128*\off+128)(%rdi)
    vmovdqa %ymm5,  (128*\off+160)(%rdi)
    vmovdqa %ymm9,  (128*\off+192)(%rdi)
    vmovdqa %ymm11, (128*\off+224)(%rdi)
.endm

/* ---- nttunpack128: unpack 128 coefficients ---- */
.macro UNPACK128 base
    vmovdqa \base+  0(%rdi), %ymm4
    vmovdqa \base+ 32(%rdi), %ymm5
    vmovdqa \base+ 64(%rdi), %ymm6
    vmovdqa \base+ 96(%rdi), %ymm7
    vmovdqa \base+128(%rdi), %ymm8
    vmovdqa \base+160(%rdi), %ymm9
    vmovdqa \base+192(%rdi), %ymm10
    vmovdqa \base+224(%rdi), %ymm11
    SHUFFLE8 4, 8,  3, 8
    SHUFFLE8 5, 9,  4, 9
    SHUFFLE8 6, 10, 5, 10
    SHUFFLE8 7, 11, 6, 11
    SHUFFLE4 3, 5,  7, 5
    SHUFFLE4 8, 10, 3, 10
    SHUFFLE4 4, 6,  8, 6
    SHUFFLE4 9, 11, 4, 11
    SHUFFLE2 7, 8,  9, 8
    SHUFFLE2 5, 6,  7, 6
    SHUFFLE2 3, 4,  5, 4
    SHUFFLE2 10, 11, 3, 11
    SHUFFLE1 9, 5,  10, 5
    SHUFFLE1 8, 4,  9,  4
    SHUFFLE1 7, 3,  8,  3
    SHUFFLE1 6, 11, 7,  11
    vmovdqa %ymm10, \base+  0(%rdi)
    vmovdqa %ymm5,  \base+ 32(%rdi)
    vmovdqa %ymm9,  \base+ 64(%rdi)
    vmovdqa %ymm4,  \base+ 96(%rdi)
    vmovdqa %ymm8,  \base+128(%rdi)
    vmovdqa %ymm3,  \base+160(%rdi)
    vmovdqa %ymm7,  \base+192(%rdi)
    vmovdqa %ymm11, \base+224(%rdi)
.endm

.text
.globl dke_ntt_dkek_avx2_unpack_linux
.type dke_ntt_dkek_avx2_unpack_linux, @function
.align 32
dke_ntt_dkek_avx2_unpack_linux:
    vmovdqa (%rsi), %ymm0       /* Q */

    NTT_LEVEL0 0
    NTT_LEVEL0 1

    NTT_LEVELS1T6 0
    NTT_LEVELS1T6 1

    UNPACK128 0
    UNPACK128 256

    vzeroupper
    ret
.size dke_ntt_dkek_avx2_unpack_linux, .-dke_ntt_dkek_avx2_unpack_linux

/* ============================================================
 * dke_ntt_dkek_avx2_linux: NTT only, outputs packed format
 * void dke_ntt_dkek_avx2_linux(int16_t *r, const int16_t *qdata);
 *   rdi = r, rsi = qdata
 * ============================================================ */
.globl dke_ntt_dkek_avx2_linux
.type dke_ntt_dkek_avx2_linux, @function
.align 32
dke_ntt_dkek_avx2_linux:
    vmovdqa (%rsi), %ymm0

    NTT_LEVEL0 0
    NTT_LEVEL0 1

    NTT_LEVELS1T6 0
    NTT_LEVELS1T6 1

    vzeroupper
    ret
.size dke_ntt_dkek_avx2_linux, .-dke_ntt_dkek_avx2_linux

/* ============================================================
 * dke_nttunpack_avx2_linux: convert packed format to standard order
 * void dke_nttunpack_avx2_linux(int16_t *r);
 *   rdi = r (512 bytes, 32-byte aligned)
 * ============================================================ */
.globl dke_nttunpack_avx2_linux
.type dke_nttunpack_avx2_linux, @function
.align 32
dke_nttunpack_avx2_linux:
    UNPACK128 0
    UNPACK128 256
    vzeroupper
    ret
.size dke_nttunpack_avx2_linux, .-dke_nttunpack_avx2_linux

.section .note.GNU-stack,"",@progbits
