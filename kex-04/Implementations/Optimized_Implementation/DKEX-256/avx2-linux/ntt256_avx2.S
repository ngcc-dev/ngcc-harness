/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 NTT for N=256, Q=3329 (DKE-128/256).
 * Layer 1 (len=128): cross-half load/compute/store.
 * Layers 2+3+4: fused in-register per half (8 blocks).
 *
 * Zeta layout for NTT (k increasing from 1):
 *   Layer 1 (len=128): zetas[1]
 *   Layer 2 (len=64):  zetas[2], zetas[3]
 *   Layer 3 (len=32):  zetas[4..7]
 *   Layer 4 (len=16):  zetas[8..15]
 *
 * For fused half H (H=0: blocks 0-7, H=1: blocks 8-15):
 *   Layer 2: zetas[2 + H]
 *   Layer 3: zetas[4 + H*2], zetas[5 + H*2]
 *   Layer 4: zetas[8 + H*4 + {0,1,2,3}]
 *
 * void dke_ntt256_avx2_asm(int16_t r[256], const int16_t zetas[128]);
 * Returns k in eax.
 */

.text

.macro CT r0, r1
    vpmullw \r1, %ymm8, %ymm9
    vpmulhw \r1, %ymm8, %ymm10
    vpmullw %ymm15, %ymm9, %ymm11
    vpmulhw %ymm14, %ymm11, %ymm11
    vpsubw  %ymm11, %ymm10, %ymm10
    vpsubw  %ymm10, \r0, \r1
    vpaddw  %ymm10, \r0, \r0
.endm

.macro LOAD_ZETA_IDX zr, idx
    movswl  (\idx * 2)(\zr), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
.endm

.globl dke_ntt256_avx2_asm
.type dke_ntt256_avx2_asm, @function
.align 32
dke_ntt256_avx2_asm:
    push    %r12

    mov     %rdi, %r12          /* r */

    mov     $3329, %eax
    vmovd   %eax, %xmm14
    vpbroadcastw %xmm14, %ymm14
    mov     $-3327, %eax
    vmovd   %eax, %xmm15
    vpbroadcastw %xmm15, %ymm15

    /* ===== Layer 1: len=128, zetas[1] ===== */
    LOAD_ZETA_IDX %rsi, 1

    .irp I, 0,1,2,3,4,5,6,7
    vmovdqa (\I * 32)(%r12), %ymm0
    vmovdqa ((\I + 8) * 32)(%r12), %ymm1
    CT %ymm0, %ymm1
    vmovdqa %ymm0, (\I * 32)(%r12)
    vmovdqa %ymm1, ((\I + 8) * 32)(%r12)
    .endr

    /* ===== Layers 2+3+4 FUSED per half ===== */
    .irp H, 0,1
    .set BASE, \H * 8

    vmovdqa ((BASE + 0) * 32)(%r12), %ymm0
    vmovdqa ((BASE + 1) * 32)(%r12), %ymm1
    vmovdqa ((BASE + 2) * 32)(%r12), %ymm2
    vmovdqa ((BASE + 3) * 32)(%r12), %ymm3
    vmovdqa ((BASE + 4) * 32)(%r12), %ymm4
    vmovdqa ((BASE + 5) * 32)(%r12), %ymm5
    vmovdqa ((BASE + 6) * 32)(%r12), %ymm6
    vmovdqa ((BASE + 7) * 32)(%r12), %ymm7

    /* Layer 2 (len=64): zetas[2+H], pairs (0,4)(1,5)(2,6)(3,7) */
    LOAD_ZETA_IDX %rsi, (2 + \H)
    CT %ymm0, %ymm4
    CT %ymm1, %ymm5
    CT %ymm2, %ymm6
    CT %ymm3, %ymm7

    /* Layer 3 (len=32): zetas[4+H*2], zetas[5+H*2] */
    LOAD_ZETA_IDX %rsi, (4 + \H * 2)
    CT %ymm0, %ymm2
    CT %ymm1, %ymm3
    LOAD_ZETA_IDX %rsi, (5 + \H * 2)
    CT %ymm4, %ymm6
    CT %ymm5, %ymm7

    /* Layer 4 (len=16): zetas[8+H*4+{0,1,2,3}] */
    LOAD_ZETA_IDX %rsi, (8 + \H * 4)
    CT %ymm0, %ymm1
    LOAD_ZETA_IDX %rsi, (9 + \H * 4)
    CT %ymm2, %ymm3
    LOAD_ZETA_IDX %rsi, (10 + \H * 4)
    CT %ymm4, %ymm5
    LOAD_ZETA_IDX %rsi, (11 + \H * 4)
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

    mov     $16, %eax           /* k = 16 */
    vzeroupper
    pop     %r12
    ret
.size dke_ntt256_avx2_asm, .-dke_ntt256_avx2_asm

.section .note.GNU-stack,"",@progbits

.text
/*
 * Extended NTT: all 7 layers including small layers (len=8,4,2).
 * void dke_ntt256_full_avx2_asm(int16_t r[256], const int16_t zetas[128],
 *                                const int16_t qdata[96]);
 * Handles layers 1-7 (len=128..2), returns void.
 * qdata layout: [0..15]=Q, [16..31]=QINV, [32..47]=V, [48..63]=(1<<9)
 */

/* GS Barrett reduce macro: reduce ymm_a in-place using qdata in ymm12(V),ymm13((1<<9)),ymm14(Q) */
.macro BARRETT a
    vpmulhw %ymm12, \a, %ymm9
    vpaddw  %ymm13, %ymm9, %ymm9
    vpsraw  $10, %ymm9, %ymm9
    vpmullw %ymm14, %ymm9, %ymm9
    vpsubw  %ymm9, \a, \a
.endm

/* CT butterfly: a += fqmul(z, b); b = a_orig - fqmul(z, b) */
/* Uses ymm15=QINV, ymm14=Q, ymm8=zeta */
.macro CT2 a, b
    vpmullw \b, %ymm8, %ymm9
    vpmulhw \b, %ymm8, %ymm10
    vpmullw %ymm15, %ymm9, %ymm9
    vpmulhw %ymm14, %ymm9, %ymm9
    vpsubw  %ymm9, %ymm10, %ymm10
    vpsubw  %ymm10, \a, \b
    vpaddw  %ymm10, \a, \a
.endm

.globl dke_ntt256_full_avx2_asm
.type dke_ntt256_full_avx2_asm, @function
.align 32
dke_ntt256_full_avx2_asm:
    push    %r12
    push    %r13

    mov     %rdi, %r12          /* r */
    mov     %rsi, %r13          /* zetas */

    /* Load constants from qdata (rdx) */
    vpbroadcastw (%rdx), %ymm14         /* Q */
    vpbroadcastw 32(%rdx), %ymm15       /* QINV */
    vpbroadcastw 64(%rdx), %ymm12       /* V (Barrett) */
    vpbroadcastw 96(%rdx), %ymm13       /* (1<<9) */

    /* ===== Layer 1: len=128, zetas[1] ===== */
    movswl  2(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8

    .irp I, 0,1,2,3,4,5,6,7
    vmovdqa (\I * 32)(%r12), %ymm0
    vmovdqa ((\I + 8) * 32)(%r12), %ymm1
    CT2 %ymm0, %ymm1
    vmovdqa %ymm0, (\I * 32)(%r12)
    vmovdqa %ymm1, ((\I + 8) * 32)(%r12)
    .endr

    /* ===== Layers 2+3+4 FUSED per half ===== */
    .irp H, 0,1
    .set BASE, \H * 8

    vmovdqa ((BASE + 0) * 32)(%r12), %ymm0
    vmovdqa ((BASE + 1) * 32)(%r12), %ymm1
    vmovdqa ((BASE + 2) * 32)(%r12), %ymm2
    vmovdqa ((BASE + 3) * 32)(%r12), %ymm3
    vmovdqa ((BASE + 4) * 32)(%r12), %ymm4
    vmovdqa ((BASE + 5) * 32)(%r12), %ymm5
    vmovdqa ((BASE + 6) * 32)(%r12), %ymm6
    vmovdqa ((BASE + 7) * 32)(%r12), %ymm7

    movswl  ((2 + \H) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    CT2 %ymm0, %ymm4
    CT2 %ymm1, %ymm5
    CT2 %ymm2, %ymm6
    CT2 %ymm3, %ymm7

    movswl  ((4 + \H * 2) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    CT2 %ymm0, %ymm2
    CT2 %ymm1, %ymm3
    movswl  ((5 + \H * 2) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    CT2 %ymm4, %ymm6
    CT2 %ymm5, %ymm7

    movswl  ((8 + \H * 4) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    CT2 %ymm0, %ymm1
    movswl  ((9 + \H * 4) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    CT2 %ymm2, %ymm3
    movswl  ((10 + \H * 4) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    CT2 %ymm4, %ymm5
    movswl  ((11 + \H * 4) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    CT2 %ymm6, %ymm7

    vmovdqa %ymm0, ((BASE + 0) * 32)(%r12)
    vmovdqa %ymm1, ((BASE + 1) * 32)(%r12)
    vmovdqa %ymm2, ((BASE + 2) * 32)(%r12)
    vmovdqa %ymm3, ((BASE + 3) * 32)(%r12)
    vmovdqa %ymm4, ((BASE + 4) * 32)(%r12)
    vmovdqa %ymm5, ((BASE + 5) * 32)(%r12)
    vmovdqa %ymm6, ((BASE + 6) * 32)(%r12)
    vmovdqa %ymm7, ((BASE + 7) * 32)(%r12)
    .endr

    /* ===== Layer 5 (len=8): zetas[16..31], 1 per 32-byte block ===== */
    /* 16 blocks, each 32 bytes = 1 ymm. Split low/high 128-bit halves. */
    /* zetas[16+i] for block i (i=0..15), offset = (16+i)*2 from zetas base */
    .set ZOFF, 16
    .irp I, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    movswl  ((\I + ZOFF) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    vmovdqa (\I * 32)(%r12), %ymm0
    vperm2i128 $0x11, %ymm0, %ymm0, %ymm1  /* ymm1 = {b, b} */
    vperm2i128 $0x00, %ymm0, %ymm0, %ymm0  /* ymm0 = {a, a} */
    CT2 %ymm0, %ymm1
    /* recombine: {ymm0[low], ymm1[low]} = {a+t, a-t} */
    vperm2i128 $0x20, %ymm1, %ymm0, %ymm0
    vmovdqa %ymm0, (\I * 32)(%r12)
    .endr

    /* ===== Layer 6 (len=4): zetas[32..63], 2 per 32-byte block ===== */
    /* a = unpacklo_epi64(v,v), b = unpackhi_epi64(v,v) */
    .set ZOFF, 32
    .irp I, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    /* za = zetas[32 + 2*I], zb = zetas[32 + 2*I + 1] */
    movswl  ((\I * 2 + ZOFF) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %xmm8
    movswl  ((\I * 2 + ZOFF + 1) * 2)(%r13), %eax
    vmovd   %eax, %xmm9
    vpbroadcastw %xmm9, %xmm9
    vinserti128 $0, %xmm8, %ymm8, %ymm8
    vinserti128 $1, %xmm9, %ymm8, %ymm8    /* ymm8 = {za x8, zb x8} */
    vmovdqa (\I * 32)(%r12), %ymm0
    vpunpcklqdq %ymm0, %ymm0, %ymm1        /* a = {c0..3,c0..3, c8..11,c8..11} */
    vpunpckhqdq %ymm0, %ymm0, %ymm2        /* b = {c4..7,c4..7, c12..15,c12..15} */
    /* CT: t = fqmul(z, b); a+t, a-t */
    vpmullw %ymm2, %ymm8, %ymm9
    vpmulhw %ymm2, %ymm8, %ymm10
    vpmullw %ymm15, %ymm9, %ymm9
    vpmulhw %ymm14, %ymm9, %ymm9
    vpsubw  %ymm9, %ymm10, %ymm10
    vpaddw  %ymm10, %ymm1, %ymm3           /* add = a + t */
    vpsubw  %ymm10, %ymm1, %ymm4           /* sub = a - t */
    vpunpcklqdq %ymm4, %ymm3, %ymm0        /* {add_lo, sub_lo, add_hi, sub_hi} */
    vmovdqa %ymm0, (\I * 32)(%r12)
    .endr

    /* ===== Layer 7 (len=2): zetas[64..127], 4 per 32-byte block ===== */
    /* a = vpshufd(v, 0xA0), b = vpshufd(v, 0xF5) */
    .set ZOFF, 64
    .irp I, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    /* 4 zetas: za=zetas[64+4*I], zb=..+1, zc=..+2, zd=..+3 */
    movswl  ((\I * 4 + ZOFF) * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %xmm8
    movswl  ((\I * 4 + ZOFF + 1) * 2)(%r13), %eax
    vmovd   %eax, %xmm9
    vpbroadcastw %xmm9, %xmm9
    vpblendw $0xF0, %xmm9, %xmm8, %xmm8   /* {za x4, zb x4} */
    movswl  ((\I * 4 + ZOFF + 2) * 2)(%r13), %eax
    vmovd   %eax, %xmm9
    vpbroadcastw %xmm9, %xmm9
    movswl  ((\I * 4 + ZOFF + 3) * 2)(%r13), %eax
    vmovd   %eax, %xmm10
    vpbroadcastw %xmm10, %xmm10
    vpblendw $0xF0, %xmm10, %xmm9, %xmm9  /* {zc x4, zd x4} */
    vinserti128 $0, %xmm8, %ymm8, %ymm8
    vinserti128 $1, %xmm9, %ymm8, %ymm8   /* ymm8 = {za4,zb4, zc4,zd4} */
    vmovdqa (\I * 32)(%r12), %ymm0
    vpshufd $0xA0, %ymm0, %ymm1            /* a = even 32-bit words dup */
    vpshufd $0xF5, %ymm0, %ymm2            /* b = odd  32-bit words dup */
    vpmullw %ymm2, %ymm8, %ymm9
    vpmulhw %ymm2, %ymm8, %ymm10
    vpmullw %ymm15, %ymm9, %ymm9
    vpmulhw %ymm14, %ymm9, %ymm9
    vpsubw  %ymm9, %ymm10, %ymm10
    vpaddw  %ymm10, %ymm1, %ymm3           /* add */
    vpsubw  %ymm10, %ymm1, %ymm4           /* sub */
    vpblendw $0xCC, %ymm4, %ymm3, %ymm0   /* even 32-bit from add, odd from sub */
    vmovdqa %ymm0, (\I * 32)(%r12)
    .endr

    vzeroupper
    pop     %r13
    pop     %r12
    ret
.size dke_ntt256_full_avx2_asm, .-dke_ntt256_full_avx2_asm
