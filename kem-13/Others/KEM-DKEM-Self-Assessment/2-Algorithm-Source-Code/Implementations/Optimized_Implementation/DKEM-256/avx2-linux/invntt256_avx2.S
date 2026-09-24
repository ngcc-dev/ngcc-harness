/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 INVNTT for N=256, Q=3329 (DKE-128/256).
 * All 7 INVNTT layers + final multiply by F=1441.
 *
 * void dke_invntt256_full_avx2_asm(int16_t r[256], const int16_t zetas[128],
 *                              const int16_t qdata[96]);
 * qdata: [0..15]=Q, [16..31]=QINV, [32..47]=V, [48..63]=(1<<9),
 *        [64..79]=MONT_F, [80..95]=INVNTT_F(1441)
 *
 * GS butterfly: add = barrett_reduce(a+b), sub = fqmul(zeta, b-a)
 * Zeta consumption: zetas[127..1] (from end), then multiply by F.
 */

.text

/* Barrett reduce ymm_a in-place. Uses ymm12=V, ymm13=(1<<9), ymm14=Q */
.macro BARRETT a
    vpmulhw %ymm12, \a, %ymm9
    vpaddw  %ymm13, %ymm9, %ymm9
    vpsraw  $10, %ymm9, %ymm9
    vpmullw %ymm14, %ymm9, %ymm9
    vpsubw  %ymm9, \a, \a
.endm

/* fqmul: dst = montgomery_reduce(zeta * src). Uses ymm15=QINV, ymm14=Q */
.macro FQMUL dst, src, zeta
    vpmullw \src, \zeta, %ymm9
    vpmulhw \src, \zeta, \dst
    vpmullw %ymm15, %ymm9, %ymm9
    vpmulhw %ymm14, %ymm9, %ymm9
    vpsubw  %ymm9, \dst, \dst
.endm

/* GS butterfly on [r12+off_a] and [r12+off_b] with zeta in ymm8 */
.macro INVGS off_a, off_b
    vmovdqa \off_a(%r12), %ymm0
    vmovdqa \off_b(%r12), %ymm1
    vpsubw  %ymm0, %ymm1, %ymm2    /* b - a */
    vpaddw  %ymm1, %ymm0, %ymm0    /* a + b */
    BARRETT %ymm0
    FQMUL   %ymm1, %ymm2, %ymm8
    vmovdqa %ymm0, \off_a(%r12)
    vmovdqa %ymm1, \off_b(%r12)
.endm

.globl dke_invntt256_full_avx2_asm
.type dke_invntt256_full_avx2_asm, @function
.align 32
dke_invntt256_full_avx2_asm:
    push    %r12
    push    %r13

    mov     %rdi, %r12          /* r */
    mov     %rsi, %r13          /* zetas */

    /* Load constants */
    vpbroadcastw (%rdx), %ymm14         /* Q */
    vpbroadcastw 32(%rdx), %ymm15       /* QINV */
    vpbroadcastw 64(%rdx), %ymm12       /* V (Barrett) */
    vpbroadcastw 96(%rdx), %ymm13       /* (1<<9) */

    /* ===== Layer 7 (len=2): zetas[127..64], 4 per block ===== */
    .set ZOFF, 64
    .irp I, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    /* 4 zetas counting DOWN: zetas[127-4*I .. 127-4*I-3] */
    .set ZB, 127 - \I * 4
    movswl  (ZB * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %xmm8
    movswl  ((ZB - 1) * 2)(%r13), %eax
    vmovd   %eax, %xmm9
    vpbroadcastw %xmm9, %xmm9
    vpblendw $0xF0, %xmm9, %xmm8, %xmm8
    movswl  ((ZB - 2) * 2)(%r13), %eax
    vmovd   %eax, %xmm9
    vpbroadcastw %xmm9, %xmm9
    movswl  ((ZB - 3) * 2)(%r13), %eax
    vmovd   %eax, %xmm10
    vpbroadcastw %xmm10, %xmm10
    vpblendw $0xF0, %xmm10, %xmm9, %xmm9
    vinserti128 $0, %xmm8, %ymm8, %ymm8
    vinserti128 $1, %xmm9, %ymm8, %ymm8
    vmovdqa (\I * 32)(%r12), %ymm0
    vpshufd $0xA0, %ymm0, %ymm1
    vpshufd $0xF5, %ymm0, %ymm2
    vpsubw  %ymm1, %ymm2, %ymm3        /* b - a */
    vpaddw  %ymm2, %ymm1, %ymm1        /* a + b */
    BARRETT %ymm1
    FQMUL   %ymm2, %ymm3, %ymm8
    vpblendw $0xCC, %ymm2, %ymm1, %ymm0
    vmovdqa %ymm0, (\I * 32)(%r12)
    .endr

    /* ===== Layer 6 (len=4): zetas[63..32], 2 per block ===== */
    .irp I, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    .set ZB, 63 - \I * 2
    movswl  (ZB * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %xmm8
    movswl  ((ZB - 1) * 2)(%r13), %eax
    vmovd   %eax, %xmm9
    vpbroadcastw %xmm9, %xmm9
    vinserti128 $0, %xmm8, %ymm8, %ymm8
    vinserti128 $1, %xmm9, %ymm8, %ymm8
    vmovdqa (\I * 32)(%r12), %ymm0
    vpunpcklqdq %ymm0, %ymm0, %ymm1
    vpunpckhqdq %ymm0, %ymm0, %ymm2
    vpsubw  %ymm1, %ymm2, %ymm3
    vpaddw  %ymm2, %ymm1, %ymm1
    BARRETT %ymm1
    FQMUL   %ymm2, %ymm3, %ymm8
    vpunpcklqdq %ymm2, %ymm1, %ymm0
    vmovdqa %ymm0, (\I * 32)(%r12)
    .endr

    /* ===== Layer 5 (len=8): zetas[31..16], 1 per block ===== */
    .irp I, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    .set ZB, 31 - \I
    movswl  (ZB * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    vmovdqa (\I * 32)(%r12), %ymm0
    vperm2i128 $0x00, %ymm0, %ymm0, %ymm1  /* {a, a} */
    vperm2i128 $0x11, %ymm0, %ymm0, %ymm2  /* {b, b} */
    vpsubw  %ymm1, %ymm2, %ymm3
    vpaddw  %ymm2, %ymm1, %ymm1
    BARRETT %ymm1
    FQMUL   %ymm2, %ymm3, %ymm8
    /* recombine: {add[low], sub[low]} */
    vperm2i128 $0x20, %ymm2, %ymm1, %ymm1
    vmovdqa %ymm1, (\I * 32)(%r12)
    .endr

    /* ===== Layer 4 (len=16): zetas[15..8] ===== */
    movswl  (15 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 0, 32

    movswl  (14 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 64, 96

    movswl  (13 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 128, 160

    movswl  (12 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 192, 224

    movswl  (11 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 256, 288

    movswl  (10 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 320, 352

    movswl  (9 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 384, 416

    movswl  (8 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 448, 480

    /* ===== Layer 3 (len=32): zetas[7..4] ===== */
    movswl  (7 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 0, 64
    INVGS 32, 96

    movswl  (6 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 128, 192
    INVGS 160, 224

    movswl  (5 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 256, 320
    INVGS 288, 352

    movswl  (4 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 384, 448
    INVGS 416, 480

    /* ===== Layer 2 (len=64): zetas[3..2] ===== */
    movswl  (3 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 256, 384
    INVGS 288, 416
    INVGS 320, 448
    INVGS 352, 480

    movswl  (2 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 0, 128
    INVGS 32, 160
    INVGS 64, 192
    INVGS 96, 224

    /* ===== Layer 1 (len=128): zetas[1] ===== */
    movswl  (1 * 2)(%r13), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    INVGS 0, 256
    INVGS 32, 288
    INVGS 64, 320
    INVGS 96, 352
    INVGS 128, 384
    INVGS 160, 416
    INVGS 192, 448
    INVGS 224, 480

    /* ===== Final multiply by F = INVNTT_F = 1441 ===== */
    vpbroadcastw 160(%rdx), %ymm8       /* 16 x INVNTT_F (offset 80*2=160) */
    xor     %eax, %eax
.Linvntt_f_loop:
    vmovdqa (%r12,%rax), %ymm0
    FQMUL   %ymm1, %ymm0, %ymm8
    vmovdqa %ymm1, (%r12,%rax)
    add     $32, %eax
    cmp     $512, %eax
    jb      .Linvntt_f_loop

    vzeroupper
    pop     %r13
    pop     %r12
    ret
.size dke_invntt256_full_avx2_asm, .-dke_invntt256_full_avx2_asm

.section .note.GNU-stack,"",@progbits
