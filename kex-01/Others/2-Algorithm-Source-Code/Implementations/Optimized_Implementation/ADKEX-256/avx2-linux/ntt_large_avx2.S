/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 NTT large layers (len=128,64,32,16) for N=256, Q=3329
 * GAS format for Linux/WSL
 *
 * void dke_ntt_large_avx2_linux(int16_t *r, const int16_t *zetas, const int16_t *qdata);
 *   rdi=r, rsi=zetas (starting at zetas[1]), rdx=qdata
 *
 * qdata: [0..31]=Q, [32..63]=QINV
 * Processes layers len=128,64,32,16 (zetas[1..15]).
 */

.text
.global dke_ntt_large_avx2_linux

dke_ntt_large_avx2_linux:
    /* Load constants */
    vmovdqa 32(%rdx), %ymm0         /* QINV */
    vmovdqa (%rdx), %ymm1           /* Q */

    /* ============ Layer 0: len=128, zetas[1] ============ */
    vpbroadcastw (%rsi), %ymm2

    xor     %eax, %eax
.Llayer0:
    vmovdqa (%rdi,%rax), %ymm3          /* a = r[j] */
    vmovdqa 256(%rdi,%rax), %ymm4       /* b = r[j+128] */

    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5         /* t */

    vpaddw  %ymm5, %ymm3, %ymm6         /* a + t */
    vpsubw  %ymm5, %ymm3, %ymm7         /* a - t */

    vmovdqa %ymm6, (%rdi,%rax)
    vmovdqa %ymm7, 256(%rdi,%rax)

    add     $32, %eax
    cmp     $256, %eax
    jb      .Llayer0

    /* ============ Layer 1: len=64, zetas[2..3] ============ */
    /* Block 0: r[0..63] vs r[64..127] */
    vpbroadcastw 2(%rsi), %ymm2

    xor     %eax, %eax
.Llayer1_b0:
    vmovdqa (%rdi,%rax), %ymm3
    vmovdqa 128(%rdi,%rax), %ymm4

    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5

    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7

    vmovdqa %ymm6, (%rdi,%rax)
    vmovdqa %ymm7, 128(%rdi,%rax)

    add     $32, %eax
    cmp     $128, %eax
    jb      .Llayer1_b0

    /* Block 1: r[128..191] vs r[192..255] */
    vpbroadcastw 4(%rsi), %ymm2

    xor     %eax, %eax
.Llayer1_b1:
    vmovdqa 256(%rdi,%rax), %ymm3
    vmovdqa 384(%rdi,%rax), %ymm4

    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5

    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7

    vmovdqa %ymm6, 256(%rdi,%rax)
    vmovdqa %ymm7, 384(%rdi,%rax)

    add     $32, %eax
    cmp     $128, %eax
    jb      .Llayer1_b1

    /* ============ Layer 2: len=32, zetas[4..7] ============ */
    lea     6(%rsi), %r9             /* r9 = &zetas[4] */
    xor     %r10d, %r10d
.Llayer2:
    vpbroadcastw (%r9), %ymm2

    /* 2 AVX2 iterations per block */
    vmovdqa (%rdi,%r10), %ymm3
    vmovdqa 64(%rdi,%r10), %ymm4

    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5
    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7
    vmovdqa %ymm6, (%rdi,%r10)
    vmovdqa %ymm7, 64(%rdi,%r10)

    vmovdqa 32(%rdi,%r10), %ymm3
    vmovdqa 96(%rdi,%r10), %ymm4

    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5
    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7
    vmovdqa %ymm6, 32(%rdi,%r10)
    vmovdqa %ymm7, 96(%rdi,%r10)

    add     $2, %r9
    add     $128, %r10
    cmp     $512, %r10d
    jb      .Llayer2

    /* ============ Layer 3: len=16, zetas[8..15] ============ */
    /* r9 now = &zetas[8] */
    xor     %r10d, %r10d
.Llayer3:
    vpbroadcastw (%r9), %ymm2

    vmovdqa (%rdi,%r10), %ymm3
    vmovdqa 32(%rdi,%r10), %ymm4

    vpmullw %ymm4, %ymm2, %ymm5
    vpmulhw %ymm4, %ymm2, %ymm6
    vpmullw %ymm0, %ymm5, %ymm5
    vpmulhw %ymm1, %ymm5, %ymm5
    vpsubw  %ymm5, %ymm6, %ymm5
    vpaddw  %ymm5, %ymm3, %ymm6
    vpsubw  %ymm5, %ymm3, %ymm7
    vmovdqa %ymm6, (%rdi,%r10)
    vmovdqa %ymm7, 32(%rdi,%r10)

    add     $2, %r9
    add     $64, %r10
    cmp     $512, %r10d
    jb      .Llayer3

    vzeroupper
    ret


#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
