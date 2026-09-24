/*
 * x86-64 AVX2 assembly ported (GNU-as) from the pq-crystals ML-KEM (Kyber)
 * AVX2 reference assembly. Third-party building block (pq-crystals;
 * public domain / Apache-2.0). NOT an original contribution of this
 * submission and OUTSIDE the scope of the B.3 ownership statement;
 * the upstream authors retain copyright (full license texts: the upstream pq-crystals project).
 */
/*
 * DKE AVX2 NTT tobytes/frombytes for N=256, Q=3329
 * GAS format for Linux/WSL
 *
 * Direct port of ML-KEM's ntttobytes_avx / nttfrombytes_avx from shuffle.S.
 *
 * void dke_ntttobytes_avx2_linux(uint8_t *r, const int16_t *a, const int16_t *qdata);
 *   rdi = r (output, 384 bytes)
 *   rsi = a (input poly, 512 bytes, packed format)
 *   rdx = qdata
 *
 * void dke_nttfrombytes_avx2_linux(int16_t *r, const uint8_t *a, const int16_t *qdata);
 *   rdi = r (output poly, 512 bytes, packed format)
 *   rsi = a (input bytes, 384 bytes)
 *   rdx = qdata
 *
 * Linux ABI: no ymm saves needed.
 */

/* ---- shuffle macros (ML-KEM GAS convention) ---- */
.macro SHUFFLE8T r0, r1, r2, r3
    vperm2i128 $0x20, %ymm\r1, %ymm\r0, %ymm\r2
    vperm2i128 $0x31, %ymm\r1, %ymm\r0, %ymm\r3
.endm
.macro SHUFFLE4T r0, r1, r2, r3
    vpunpcklqdq %ymm\r1, %ymm\r0, %ymm\r2
    vpunpckhqdq %ymm\r1, %ymm\r0, %ymm\r3
.endm
.macro SHUFFLE2T r0, r1, r2, r3
    vmovsldup %ymm\r1, %ymm\r2
    vpblendd $0xAA, %ymm\r2, %ymm\r0, %ymm\r2
    vpsrlq $32, %ymm\r0, %ymm\r0
    vpblendd $0xAA, %ymm\r1, %ymm\r0, %ymm\r3
.endm
.macro SHUFFLE1T r0, r1, r2, r3
    vpslld $16, %ymm\r1, %ymm\r2
    vpblendw $0xAA, %ymm\r2, %ymm\r0, %ymm\r2
    vpsrld $16, %ymm\r0, %ymm\r0
    vpblendw $0xAA, %ymm\r1, %ymm\r0, %ymm\r3
.endm

/* csubq: map [0,2Q-1] -> [0,Q-1]. ymm0=Q, uses ymm_tmp as scratch */
.macro CSUBQ r, tmp
    vpsubw %ymm0, %ymm\r, %ymm\r
    vpsraw $15, %ymm\r, %ymm\tmp
    vpand  %ymm0, %ymm\tmp, %ymm\tmp
    vpaddw %ymm\tmp, %ymm\r, %ymm\r
.endm

/* ============================================================
 * ntttobytes128: serialize 128 packed coefficients -> 192 bytes
 * rdi = output (192 bytes), rsi = input (256 bytes), ymm0 = Q
 * ============================================================ */
.macro NTTTOBYTES128
    vmovdqa    (%rsi), %ymm5
    vmovdqa  32(%rsi), %ymm6
    vmovdqa  64(%rsi), %ymm7
    vmovdqa  96(%rsi), %ymm8
    vmovdqa 128(%rsi), %ymm9
    vmovdqa 160(%rsi), %ymm10
    vmovdqa 192(%rsi), %ymm11
    vmovdqa 224(%rsi), %ymm12

    CSUBQ 5,  13
    CSUBQ 6,  13
    CSUBQ 7,  13
    CSUBQ 8,  13
    CSUBQ 9,  13
    CSUBQ 10, 13
    CSUBQ 11, 13
    CSUBQ 12, 13

    /* bitpack */
    vpsllw $12, %ymm6, %ymm4
    vpor   %ymm5, %ymm4, %ymm4

    vpsrlw $4, %ymm6, %ymm5
    vpsllw $8, %ymm7, %ymm6
    vpor   %ymm5, %ymm6, %ymm5

    vpsrlw $8, %ymm7, %ymm6
    vpsllw $4, %ymm8, %ymm7
    vpor   %ymm6, %ymm7, %ymm6

    vpsllw $12, %ymm10, %ymm7
    vpor   %ymm9, %ymm7, %ymm7

    vpsrlw $4, %ymm10, %ymm8
    vpsllw $8, %ymm11, %ymm9
    vpor   %ymm8, %ymm9, %ymm8

    vpsrlw $8, %ymm11, %ymm9
    vpsllw $4, %ymm12, %ymm10
    vpor   %ymm9, %ymm10, %ymm9

    /* shuffle */
    SHUFFLE1T 4, 5, 3, 5
    SHUFFLE1T 6, 7, 4, 7
    SHUFFLE1T 8, 9, 6, 9

    SHUFFLE2T 3, 4, 8, 4
    SHUFFLE2T 6, 5, 3, 5
    SHUFFLE2T 7, 9, 6, 9

    SHUFFLE4T 8, 3, 7, 3
    SHUFFLE4T 6, 4, 8, 4
    SHUFFLE4T 5, 9, 6, 9

    SHUFFLE8T 7, 8, 5, 8
    SHUFFLE8T 6, 3, 7, 3
    SHUFFLE8T 4, 9, 6, 9

    vmovdqu %ymm5,    (%rdi)
    vmovdqu %ymm7,  32(%rdi)
    vmovdqu %ymm6,  64(%rdi)
    vmovdqu %ymm8,  96(%rdi)
    vmovdqu %ymm3, 128(%rdi)
    vmovdqu %ymm9, 160(%rdi)
.endm

/* ============================================================
 * nttfrombytes128: deserialize 192 bytes -> 128 packed coefficients
 * rdi = output (256 bytes), rsi = input (192 bytes), ymm0 = MASK(0xFFF)
 * ============================================================ */
.macro NTTFROMBYTES128
    vmovdqu    (%rsi), %ymm4
    vmovdqu  32(%rsi), %ymm5
    vmovdqu  64(%rsi), %ymm6
    vmovdqu  96(%rsi), %ymm7
    vmovdqu 128(%rsi), %ymm8
    vmovdqu 160(%rsi), %ymm9

    /* unshuffle */
    SHUFFLE8T 4, 7, 3, 7
    SHUFFLE8T 5, 8, 4, 8
    SHUFFLE8T 6, 9, 5, 9

    SHUFFLE4T 3, 8, 6, 8
    SHUFFLE4T 7, 5, 3, 5
    SHUFFLE4T 4, 9, 7, 9

    SHUFFLE2T 6, 5, 4, 5
    SHUFFLE2T 8, 7, 6, 7
    SHUFFLE2T 3, 9, 8, 9

    SHUFFLE1T 4, 7, 10, 7
    SHUFFLE1T 5, 8, 4,  8
    SHUFFLE1T 6, 9, 5,  9

    /* bitunpack */
    vpsrlw $12, %ymm10, %ymm11
    vpsllw $4,  %ymm7,  %ymm12
    vpor   %ymm11, %ymm12, %ymm11
    vpand  %ymm0, %ymm10, %ymm10
    vpand  %ymm0, %ymm11, %ymm11

    vpsrlw $8, %ymm7,  %ymm12
    vpsllw $8, %ymm4,  %ymm13
    vpor   %ymm12, %ymm13, %ymm12
    vpand  %ymm0, %ymm12, %ymm12

    vpsrlw $4, %ymm4, %ymm13
    vpand  %ymm0, %ymm13, %ymm13

    vpsrlw $12, %ymm8,  %ymm14
    vpsllw $4,  %ymm5,  %ymm15
    vpor   %ymm14, %ymm15, %ymm14
    vpand  %ymm0, %ymm8,  %ymm8
    vpand  %ymm0, %ymm14, %ymm14

    vpsrlw $8, %ymm5, %ymm15
    vpsllw $8, %ymm9, %ymm1
    vpor   %ymm15, %ymm1, %ymm15
    vpand  %ymm0, %ymm15, %ymm15

    vpsrlw $4, %ymm9, %ymm1
    vpand  %ymm0, %ymm1, %ymm1

    vmovdqa %ymm10,    (%rdi)
    vmovdqa %ymm11,  32(%rdi)
    vmovdqa %ymm12,  64(%rdi)
    vmovdqa %ymm13,  96(%rdi)
    vmovdqa %ymm8,  128(%rdi)
    vmovdqa %ymm14, 160(%rdi)
    vmovdqa %ymm15, 192(%rdi)
    vmovdqa %ymm1,  224(%rdi)
.endm

.text

/* ============================================================
 * dke_ntttobytes_avx2_linux
 * rdi=output(384B), rsi=input(512B), rdx=qdata
 * ============================================================ */
.globl dke_ntttobytes_avx2_linux
.type dke_ntttobytes_avx2_linux, @function
.align 32
dke_ntttobytes_avx2_linux:
    vmovdqa (%rdx), %ymm0       /* Q */

    NTTTOBYTES128

    add $256, %rsi
    add $192, %rdi

    NTTTOBYTES128

    vzeroupper
    ret
.size dke_ntttobytes_avx2_linux, .-dke_ntttobytes_avx2_linux

/* ============================================================
 * dke_nttfrombytes_avx2_linux
 * rdi=output(512B), rsi=input(384B), rdx=qdata
 * ============================================================ */
.globl dke_nttfrombytes_avx2_linux
.type dke_nttfrombytes_avx2_linux, @function
.align 32
dke_nttfrombytes_avx2_linux:
    /* MASK = 0x0FFF */
    vpcmpeqw %ymm0, %ymm0, %ymm0
    vpsrlw $4, %ymm0, %ymm0

    NTTFROMBYTES128

    add $192, %rsi
    add $256, %rdi

    NTTFROMBYTES128

    vzeroupper
    ret
.size dke_nttfrombytes_avx2_linux, .-dke_nttfrombytes_avx2_linux

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
