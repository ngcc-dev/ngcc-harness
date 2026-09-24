/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 NTT large layers for N=512, Q=7681 (DKE-512)
 * GAS format for Linux/WSL
 *
 * void dke_ntt_large512_avx2_linux(int16_t *r, const int16_t *zetas, const int16_t *qdata512);
 *   rdi=r (1024 bytes), rsi=zetas+1, rdx=qdata512
 */

.macro BF512 off_a, off_b
    vmovdqa \off_a(%r11), %ymm3
    vmovdqa \off_b(%r11), %ymm4
    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5
    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7
    vmovdqa %ymm6, \off_a(%r11)
    vmovdqa %ymm7, \off_b(%r11)
.endm

.text
.global dke_ntt_large512_avx2_linux

dke_ntt_large512_avx2_linux:
    vmovdqa 32(%rdx), %ymm0         /* QINV */
    vmovdqa (%rdx), %ymm1           /* Q */

    /* Layer 0: len=256, zetas[1] */
    vpbroadcastw (%rsi), %ymm2
    xor     %eax, %eax
.Ll0_512:
    vmovdqa (%rdi,%rax), %ymm3
    vmovdqa 512(%rdi,%rax), %ymm4
    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5
    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7
    vmovdqa %ymm6, (%rdi,%rax)
    vmovdqa %ymm7, 512(%rdi,%rax)
    add     $32, %eax
    cmp     $512, %eax
    jb      .Ll0_512

    /* Layer 1: len=128, zetas[2..3] */
    vpbroadcastw 2(%rsi), %ymm2
    xor     %eax, %eax
.Ll1_b0:
    vmovdqa (%rdi,%rax), %ymm3
    vmovdqa 256(%rdi,%rax), %ymm4
    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5
    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7
    vmovdqa %ymm6, (%rdi,%rax)
    vmovdqa %ymm7, 256(%rdi,%rax)
    add     $32, %eax
    cmp     $256, %eax
    jb      .Ll1_b0

    vpbroadcastw 4(%rsi), %ymm2
    xor     %eax, %eax
.Ll1_b1:
    vmovdqa 512(%rdi,%rax), %ymm3
    vmovdqa 768(%rdi,%rax), %ymm4
    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5
    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7
    vmovdqa %ymm6, 512(%rdi,%rax)
    vmovdqa %ymm7, 768(%rdi,%rax)
    add     $32, %eax
    cmp     $256, %eax
    jb      .Ll1_b1

    /* Layer 2: len=64, zetas[4..7] */
    lea     6(%rsi), %r9
    xor     %r10d, %r10d
.Ll2_512:
    vpbroadcastw (%r9), %ymm2
    lea     (%rdi,%r10), %r11
    BF512 0,   128
    BF512 32,  160
    BF512 64,  192
    BF512 96,  224
    add     $2, %r9
    add     $256, %r10
    cmp     $1024, %r10d
    jb      .Ll2_512

    /* Layer 3: len=32, zetas[8..15] */
    xor     %r10d, %r10d
.Ll3_512:
    vpbroadcastw (%r9), %ymm2
    lea     (%rdi,%r10), %r11
    BF512 0,  64
    BF512 32, 96
    add     $2, %r9
    add     $128, %r10
    cmp     $1024, %r10d
    jb      .Ll3_512

    /* Layer 4: len=16, zetas[16..31] */
    xor     %r10d, %r10d
.Ll4_512:
    vpbroadcastw (%r9), %ymm2
    lea     (%rdi,%r10), %r11
    BF512 0, 32
    add     $2, %r9
    add     $64, %r10
    cmp     $1024, %r10d
    jb      .Ll4_512

    vzeroupper
    ret

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
