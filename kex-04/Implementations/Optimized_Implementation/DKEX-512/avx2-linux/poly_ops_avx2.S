/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 poly_reduce and poly_tomont for N=256, Q=3329
 * GAS format for Linux/WSL
 *
 * void dke_reduce_avx2_linux(int16_t *r, const int16_t *qdata);
 *   rdi=r, rsi=qdata
 *
 * void dke_tomont_avx2_linux(int16_t *r, const int16_t *qdata);
 *   rdi=r, rsi=qdata
 *
 * qdata layout (bytes):
 *   [0..31]   = 16 x Q
 *   [32..63]  = 16 x QINV
 *   [64..95]  = 16 x V (Barrett)
 *   [96..127] = 16 x (1<<9)
 *   [128..159]= 16 x F (Montgomery)
 */

.text

/* ============================================================ */
.global dke_reduce_avx2_linux
dke_reduce_avx2_linux:
    vmovdqa (%rsi), %ymm0           /* Q */
    vmovdqa 64(%rsi), %ymm1         /* V (Barrett) */
    vmovdqa 96(%rsi), %ymm2         /* (1<<9) */

    xor     %eax, %eax
.Lreduce_loop:
    vmovdqa (%rdi,%rax), %ymm3

    vpmulhw %ymm1, %ymm3, %ymm4    /* mulhi(a, v) */
    vpaddw  %ymm2, %ymm4, %ymm4    /* + (1<<9) */
    vpsraw  $10, %ymm4, %ymm4      /* >> 10 */
    vpmullw %ymm0, %ymm4, %ymm4    /* * Q */
    vpsubw  %ymm4, %ymm3, %ymm3    /* a - t */

    vmovdqa %ymm3, (%rdi,%rax)
    add     $32, %eax
    cmp     $512, %eax
    jb      .Lreduce_loop

    vzeroupper
    ret

/* ============================================================ */
.global dke_tomont_avx2_linux
dke_tomont_avx2_linux:
    vmovdqa 32(%rsi), %ymm0         /* QINV */
    vmovdqa (%rsi), %ymm1           /* Q */
    vmovdqa 128(%rsi), %ymm2        /* F = (2^32) mod Q */

    xor     %eax, %eax
.Ltomont_loop:
    vmovdqa (%rdi,%rax), %ymm3

    vpmullw %ymm2, %ymm3, %ymm4    /* lo = mullo(a, f) */
    vpmulhw %ymm2, %ymm3, %ymm5    /* hi = mulhi(a, f) */
    vpmullw %ymm0, %ymm4, %ymm4    /* lo * QINV */
    vpmulhw %ymm1, %ymm4, %ymm4    /* mulhi(lo*QINV, Q) */
    vpsubw  %ymm4, %ymm5, %ymm3    /* hi - t */

    vmovdqa %ymm3, (%rdi,%rax)
    add     $32, %eax
    cmp     $512, %eax
    jb      .Ltomont_loop

    vzeroupper
    ret


#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
