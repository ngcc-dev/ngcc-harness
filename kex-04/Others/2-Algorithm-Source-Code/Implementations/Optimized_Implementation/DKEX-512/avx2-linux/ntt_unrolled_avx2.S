/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 NTT fully unrolled for N=256, Q=3329 (DKE-128/256)
 * GAS format for Linux/WSL
 *
 * void dke_ntt_unrolled_avx2_linux(int16_t *r, const int16_t *zetas, const int16_t *qdata);
 *   rdi=r, rsi=zetas+1, rdx=qdata
 */

/* CT butterfly macro: [rdi+off_a] vs [rdi+off_b] with zeta in ymm2 */
.macro BF off_a, off_b
    vmovdqa \off_a(%rdi), %ymm3
    vmovdqa \off_b(%rdi), %ymm4
    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5
    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7
    vmovdqa %ymm6, \off_a(%rdi)
    vmovdqa %ymm7, \off_b(%rdi)
.endm

.text
.global dke_ntt_unrolled_avx2_linux

dke_ntt_unrolled_avx2_linux:
    vmovdqa 32(%rdx), %ymm0         /* QINV */
    vmovdqa (%rdx), %ymm1           /* Q */

    /* Layer 0: len=128, zetas[1] */
    vpbroadcastw (%rsi), %ymm2
    BF 0,   256
    BF 32,  288
    BF 64,  320
    BF 96,  352
    BF 128, 384
    BF 160, 416
    BF 192, 448
    BF 224, 480

    /* Layer 1: len=64, zetas[2..3] */
    vpbroadcastw 2(%rsi), %ymm2
    BF 0,   128
    BF 32,  160
    BF 64,  192
    BF 96,  224

    vpbroadcastw 4(%rsi), %ymm2
    BF 256, 384
    BF 288, 416
    BF 320, 448
    BF 352, 480

    /* Layer 2: len=32, zetas[4..7] */
    vpbroadcastw 6(%rsi), %ymm2
    BF 0,   64
    BF 32,  96

    vpbroadcastw 8(%rsi), %ymm2
    BF 128, 192
    BF 160, 224

    vpbroadcastw 10(%rsi), %ymm2
    BF 256, 320
    BF 288, 352

    vpbroadcastw 12(%rsi), %ymm2
    BF 384, 448
    BF 416, 480

    /* Layer 3: len=16, zetas[8..15] */
    vpbroadcastw 14(%rsi), %ymm2
    BF 0,   32

    vpbroadcastw 16(%rsi), %ymm2
    BF 64,  96

    vpbroadcastw 18(%rsi), %ymm2
    BF 128, 160

    vpbroadcastw 20(%rsi), %ymm2
    BF 192, 224

    vpbroadcastw 22(%rsi), %ymm2
    BF 256, 288

    vpbroadcastw 24(%rsi), %ymm2
    BF 320, 352

    vpbroadcastw 26(%rsi), %ymm2
    BF 384, 416

    vpbroadcastw 28(%rsi), %ymm2
    BF 448, 480

    vzeroupper
    ret

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
