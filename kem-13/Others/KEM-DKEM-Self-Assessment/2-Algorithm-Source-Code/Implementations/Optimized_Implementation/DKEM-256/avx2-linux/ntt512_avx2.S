/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 NTT for N=512, Q=7681 (DKE-512).
 * Layers 1-2: dual-butterfly interleaved to hide latency.
 * Layers 3+4+5: fused in-register per 8-block group with interleaving.
 *
 * void dke_ntt512_avx2_asm(int16_t r[512], const int16_t zetas[256]);
 * Returns k in eax.
 */

.text

/* Single CT butterfly: zeta in ymm8, Q in ymm14, QINV in ymm15.
 * Result: r0 = r0 + fqmul(zeta, r1), r1 = r0_old - fqmul(zeta, r1).
 * Scratch: ymm9, ymm10, ymm11. */
.macro CT r0, r1
    vpmullw \r1, %ymm8, %ymm9
    vpmulhw \r1, %ymm8, %ymm10
    vpmullw %ymm15, %ymm9, %ymm11
    vpmulhw %ymm14, %ymm11, %ymm11
    vpsubw  %ymm11, %ymm10, %ymm10
    vpsubw  %ymm10, \r0, \r1
    vpaddw  %ymm10, \r0, \r0
.endm

/* Dual CT butterfly: process two pairs simultaneously.
 * Interleaves instructions from pair A (ra0,ra1) and pair B (rb0,rb1).
 * Both use same zeta in ymm8. Scratch: ymm9-ymm11 for A, ymm12-ymm13+stack for B.
 * Actually we only have ymm0-7 for data + ymm8 zeta + ymm14 Q + ymm15 QINV = 11 used.
 * Scratch available: ymm9,10,11,12,13 = 5 registers. Enough for 2 interleaved. */
.macro CT2 ra0, ra1, rb0, rb1
    /* Start both multiplies */
    vpmullw \ra1, %ymm8, %ymm9         /* A: lo */
    vpmullw \rb1, %ymm8, %ymm12        /* B: lo */
    vpmulhw \ra1, %ymm8, %ymm10        /* A: hi */
    vpmulhw \rb1, %ymm8, %ymm13        /* B: hi */
    /* Montgomery reduce both */
    vpmullw %ymm15, %ymm9, %ymm11      /* A: t = lo * QINV */
    vpmullw %ymm15, %ymm12, %ymm9      /* B: t = lo * QINV (reuse ymm9) */
    vpmulhw %ymm14, %ymm11, %ymm11     /* A: mulhi(t, Q) */
    vpmulhw %ymm14, %ymm9, %ymm9       /* B: mulhi(t, Q) */
    vpsubw  %ymm11, %ymm10, %ymm10     /* A: fqmul result */
    vpsubw  %ymm9, %ymm13, %ymm13      /* B: fqmul result */
    /* Apply butterflies */
    vpsubw  %ymm10, \ra0, \ra1
    vpaddw  %ymm10, \ra0, \ra0
    vpsubw  %ymm13, \rb0, \rb1
    vpaddw  %ymm13, \rb0, \rb0
.endm

.macro LOAD_ZETA_IDX zr, idx
    movswl  (\idx * 2)(\zr), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
.endm

.macro LOAD_Z zr, kr
    movswl  (\zr, \kr, 2), %eax
    inc     \kr
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
.endm

.globl dke_ntt512_avx2_asm
.type dke_ntt512_avx2_asm, @function
.align 32
dke_ntt512_avx2_asm:
    push    %rbx
    push    %r12
    push    %r13

    mov     %rdi, %r12
    mov     %rsi, %r13

    mov     $7681, %eax
    vmovd   %eax, %xmm14
    vpbroadcastw %xmm14, %ymm14
    mov     $-7679, %eax
    vmovd   %eax, %xmm15
    vpbroadcastw %xmm15, %ymm15

    mov     $1, %ebx

    /* ===== Layer 1: len=256, 1 zeta, 16 pairs — dual interleaved ===== */
    LOAD_Z %r13, %rbx
    .irp I, 0,2,4,6,8,10,12,14
    vmovdqa (\I * 32)(%r12), %ymm0
    vmovdqa ((\I + 16) * 32)(%r12), %ymm1
    vmovdqa ((\I + 1) * 32)(%r12), %ymm2
    vmovdqa ((\I + 17) * 32)(%r12), %ymm3
    CT2 %ymm0, %ymm1, %ymm2, %ymm3
    vmovdqa %ymm0, (\I * 32)(%r12)
    vmovdqa %ymm1, ((\I + 16) * 32)(%r12)
    vmovdqa %ymm2, ((\I + 1) * 32)(%r12)
    vmovdqa %ymm3, ((\I + 17) * 32)(%r12)
    .endr

    /* ===== Layer 2: len=128, 2 zetas, 8 pairs each — dual interleaved ===== */
    LOAD_Z %r13, %rbx
    .irp I, 0,2,4,6
    vmovdqa (\I * 32)(%r12), %ymm0
    vmovdqa ((\I + 8) * 32)(%r12), %ymm1
    vmovdqa ((\I + 1) * 32)(%r12), %ymm2
    vmovdqa ((\I + 9) * 32)(%r12), %ymm3
    CT2 %ymm0, %ymm1, %ymm2, %ymm3
    vmovdqa %ymm0, (\I * 32)(%r12)
    vmovdqa %ymm1, ((\I + 8) * 32)(%r12)
    vmovdqa %ymm2, ((\I + 1) * 32)(%r12)
    vmovdqa %ymm3, ((\I + 9) * 32)(%r12)
    .endr
    LOAD_Z %r13, %rbx
    .irp I, 16,18,20,22
    vmovdqa (\I * 32)(%r12), %ymm0
    vmovdqa ((\I + 8) * 32)(%r12), %ymm1
    vmovdqa ((\I + 1) * 32)(%r12), %ymm2
    vmovdqa ((\I + 9) * 32)(%r12), %ymm3
    CT2 %ymm0, %ymm1, %ymm2, %ymm3
    vmovdqa %ymm0, (\I * 32)(%r12)
    vmovdqa %ymm1, ((\I + 8) * 32)(%r12)
    vmovdqa %ymm2, ((\I + 1) * 32)(%r12)
    vmovdqa %ymm3, ((\I + 9) * 32)(%r12)
    .endr

    /* ===== Layers 3+4+5 FUSED in-register with interleaving ===== */
    .irp G, 0,1,2,3
    .set BASE, \G * 8

    vmovdqa ((BASE + 0) * 32)(%r12), %ymm0
    vmovdqa ((BASE + 1) * 32)(%r12), %ymm1
    vmovdqa ((BASE + 2) * 32)(%r12), %ymm2
    vmovdqa ((BASE + 3) * 32)(%r12), %ymm3
    vmovdqa ((BASE + 4) * 32)(%r12), %ymm4
    vmovdqa ((BASE + 5) * 32)(%r12), %ymm5
    vmovdqa ((BASE + 6) * 32)(%r12), %ymm6
    vmovdqa ((BASE + 7) * 32)(%r12), %ymm7

    /* Layer 3: zetas[4+G] */
    LOAD_ZETA_IDX %r13, (4 + \G)
    CT2 %ymm0, %ymm4, %ymm1, %ymm5
    CT2 %ymm2, %ymm6, %ymm3, %ymm7

    /* Layer 4: zetas[8+G*2] and zetas[9+G*2] */
    LOAD_ZETA_IDX %r13, (8 + \G * 2)
    CT2 %ymm0, %ymm2, %ymm1, %ymm3
    LOAD_ZETA_IDX %r13, (9 + \G * 2)
    CT2 %ymm4, %ymm6, %ymm5, %ymm7

    /* Layer 5: 4 zetas */
    LOAD_ZETA_IDX %r13, (16 + \G * 4)
    CT %ymm0, %ymm1
    LOAD_ZETA_IDX %r13, (17 + \G * 4)
    CT %ymm2, %ymm3
    LOAD_ZETA_IDX %r13, (18 + \G * 4)
    CT %ymm4, %ymm5
    LOAD_ZETA_IDX %r13, (19 + \G * 4)
    CT %ymm6, %ymm7

    vmovdqa %ymm0, ((BASE + 0) * 32)(%r12)
    vmovdqa %ymm1, ((BASE + 1) * 32)(%r12)
    vmovdqa %ymm2, ((BASE + 2) * 32)(%r12)
    vmovdqa %ymm3, ((BASE + 3) * 32)(%r12)
    vmovdqa %ymm4, ((BASE + 4) * 32)(%r12)
    vmovdqa %ymm5, ((BASE + 5) * 32)(%r12)
    vmovdqa %ymm6, ((BASE + 6) * 32)(%r12)
    vmovdqa %ymm7, ((BASE + 7) * 32)(%r12)
    .endr

    mov     $32, %eax
    vzeroupper
    pop     %r13
    pop     %r12
    pop     %rbx
    ret
.size dke_ntt512_avx2_asm, .-dke_ntt512_avx2_asm

.section .note.GNU-stack,"",@progbits
