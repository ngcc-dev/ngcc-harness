/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 basemul for N=512, Q=7681 (DKE-512)
 * GAS format for Linux/WSL
 *
 * void dke_basemul512_avx2_linux(int16_t *r, const int16_t *a, const int16_t *b,
 *                                 const int16_t *qdata512, const int16_t *zetas);
 * rdi=r, rsi=a, rdx=b, rcx=qdata512, r8=zetas
 * 16 iterations x 32 coefficients = 512 total.
 */

.text
.global dke_basemul512_avx2_linux

dke_basemul512_avx2_linux:
    mov     %r8, %r10
    xor     %eax, %eax

    vmovdqa (%rcx), %ymm1           /* Q */
    vmovdqa 32(%rcx), %ymm0         /* QINV */

.Lbm512_loop:
    vpmovzxwd (%r10), %ymm2
    vpslld    $16, %ymm2, %ymm3
    vpsubw    %ymm3, %ymm2, %ymm2

    vmovdqa (%rsi), %ymm4
    vmovdqa 32(%rsi), %ymm5
    vmovdqa (%rdx), %ymm6
    vmovdqa 32(%rdx), %ymm7

    vpsrld  $16, %ymm6, %ymm14
    vpsrld  $16, %ymm7, %ymm13
    vpslld  $16, %ymm6, %ymm12
    vpslld  $16, %ymm7, %ymm11
    vpblendw $0x55, %ymm14, %ymm12, %ymm14
    vpblendw $0x55, %ymm13, %ymm11, %ymm13

    vpmulhw %ymm14, %ymm4, %ymm8
    vpmulhw %ymm13, %ymm5, %ymm9
    vpmullw %ymm14, %ymm4, %ymm10
    vpmullw %ymm13, %ymm5, %ymm11
    vpmullw %ymm0, %ymm10, %ymm10
    vpmullw %ymm0, %ymm11, %ymm11
    vpmulhw %ymm1, %ymm10, %ymm10
    vpmulhw %ymm1, %ymm11, %ymm11
    vpsubw  %ymm10, %ymm8, %ymm8
    vpsubw  %ymm11, %ymm9, %ymm9

    vphaddw %ymm9, %ymm8, %ymm15
    vpermq  $0xD8, %ymm15, %ymm15

    vpmulhw %ymm6, %ymm4, %ymm8
    vpmulhw %ymm7, %ymm5, %ymm9
    vpmullw %ymm6, %ymm4, %ymm13
    vpmullw %ymm7, %ymm5, %ymm14
    vpmullw %ymm0, %ymm13, %ymm10
    vpmullw %ymm0, %ymm14, %ymm11
    vpmulhw %ymm1, %ymm10, %ymm10
    vpmulhw %ymm1, %ymm11, %ymm11
    vpsubw  %ymm10, %ymm8, %ymm8
    vpsubw  %ymm11, %ymm9, %ymm9

    vpshuflw $0xD8, %ymm8, %ymm8
    vpshuflw $0xD8, %ymm9, %ymm9
    vpshufhw $0xD8, %ymm8, %ymm8
    vpshufhw $0xD8, %ymm9, %ymm9
    vpshufd  $0xD8, %ymm8, %ymm8
    vpshufd  $0xD8, %ymm9, %ymm9
    vpermq   $0xD8, %ymm8, %ymm8
    vpermq   $0xD8, %ymm9, %ymm9

    vperm2f128 $0x20, %ymm9, %ymm8, %ymm10
    vperm2f128 $0x31, %ymm9, %ymm8, %ymm11

    vpmulhw %ymm2, %ymm11, %ymm8
    vpmullw %ymm2, %ymm11, %ymm9
    vpmullw %ymm0, %ymm9, %ymm9
    vpmulhw %ymm1, %ymm9, %ymm9
    vpsubw  %ymm9, %ymm8, %ymm8
    vpaddw  %ymm10, %ymm8, %ymm8

    vperm2i128 $0x20, %ymm15, %ymm8, %ymm9
    vperm2i128 $0x31, %ymm15, %ymm8, %ymm10
    vpermq  $0xD8, %ymm9, %ymm9
    vpermq  $0xD8, %ymm10, %ymm10
    vpshufd $0xD8, %ymm9, %ymm9
    vpshufd $0xD8, %ymm10, %ymm10
    vpshuflw $0xD8, %ymm9, %ymm9
    vpshuflw $0xD8, %ymm10, %ymm10
    vpshufhw $0xD8, %ymm9, %ymm9
    vpshufhw $0xD8, %ymm10, %ymm10

    vmovdqa %ymm9, (%rdi)
    vmovdqa %ymm10, 32(%rdi)

    add     $64, %rdi
    add     $64, %rsi
    add     $64, %rdx
    add     $16, %r10
    add     $1, %eax
    cmp     $16, %eax
    jb      .Lbm512_loop

    vzeroupper
    ret

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
