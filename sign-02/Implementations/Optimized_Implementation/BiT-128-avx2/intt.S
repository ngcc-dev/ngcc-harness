/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */

/*
 * BiT-128 AVX2 inverse NTT — hand-written assembly (4-way ILP, min-trick).
 * GS butterflies + N^-1 scale + ->signed.  Tables consts.c zinv_h/zinv_l.
 *   x in [0,2q) -> [0,q):  minu(x, x-q)     x in (-q,q) -> [0,q): minu(x, x+q)
 *   ntt_inv_asm(int16_t *a)
 */
#define Q      %ymm15
#define BIT_Q       26881
#define BIT_QHALF   13440

.section .rodata
.p2align 5
vQ:     .rept 16
        .short BIT_Q
        .endr
vQHALF: .rept 16
        .short BIT_QHALF
        .endr

.text
/* Intra GS core: u=U, v=shuffled V -> u=(U+V)[0,q), v=mont(U-V)[0,q). */
.macro PGS u, v, z, t
    vpaddw  \v, \u, \t          /* sum = U+V */
    vpsubw  \v, \u, \u          /* dif = U-V */
    vpmullw \z(%rdx), \u, \v
    vpmulhw \z(%rsi), \u, \u
    vpmulhw Q, \v, \v
    vpsubw  \v, \u, \v          /* mont(dif) (-q,q) */
    vpaddw  Q, \v, \u
    vpminuw \u, \v, \v          /* mont [0,q) */
    vpsubw  Q, \t, \u
    vpminuw \u, \t, \u          /* sum [0,q) */
.endm

.macro INTRA_BLOCK_INV o0,o1,o2,o3, z0, z1, z2
    vmovdqa \o0(%rdi), %ymm0
    vmovdqa \o1(%rdi), %ymm1
    vmovdqa \o2(%rdi), %ymm2
    vmovdqa \o3(%rdi), %ymm3
    vpshufd $0xF5, %ymm0, %ymm4
    vpshufd $0xF5, %ymm1, %ymm5
    vpshufd $0xF5, %ymm2, %ymm6
    vpshufd $0xF5, %ymm3, %ymm7
    PGS %ymm0, %ymm4, \z0,    %ymm8
    PGS %ymm1, %ymm5, \z0+32, %ymm9
    PGS %ymm2, %ymm6, \z0+64, %ymm10
    PGS %ymm3, %ymm7, \z0+96, %ymm11
    vpshufd $0xB1, %ymm4, %ymm4
    vpshufd $0xB1, %ymm5, %ymm5
    vpshufd $0xB1, %ymm6, %ymm6
    vpshufd $0xB1, %ymm7, %ymm7
    vpblendd $0xAA, %ymm4, %ymm0, %ymm0
    vpblendd $0xAA, %ymm5, %ymm1, %ymm1
    vpblendd $0xAA, %ymm6, %ymm2, %ymm2
    vpblendd $0xAA, %ymm7, %ymm3, %ymm3
    vpshufd $0xEE, %ymm0, %ymm4
    vpshufd $0xEE, %ymm1, %ymm5
    vpshufd $0xEE, %ymm2, %ymm6
    vpshufd $0xEE, %ymm3, %ymm7
    PGS %ymm0, %ymm4, \z1,    %ymm8
    PGS %ymm1, %ymm5, \z1+32, %ymm9
    PGS %ymm2, %ymm6, \z1+64, %ymm10
    PGS %ymm3, %ymm7, \z1+96, %ymm11
    vpunpcklqdq %ymm4, %ymm0, %ymm0
    vpunpcklqdq %ymm5, %ymm1, %ymm1
    vpunpcklqdq %ymm6, %ymm2, %ymm2
    vpunpcklqdq %ymm7, %ymm3, %ymm3
    vperm2i128 $0x11, %ymm0, %ymm0, %ymm4
    vperm2i128 $0x11, %ymm1, %ymm1, %ymm5
    vperm2i128 $0x11, %ymm2, %ymm2, %ymm6
    vperm2i128 $0x11, %ymm3, %ymm3, %ymm7
    PGS %ymm0, %ymm4, \z2,    %ymm8
    PGS %ymm1, %ymm5, \z2+32, %ymm9
    PGS %ymm2, %ymm6, \z2+64, %ymm10
    PGS %ymm3, %ymm7, \z2+96, %ymm11
    vperm2i128 $0x20, %ymm4, %ymm0, %ymm0
    vperm2i128 $0x20, %ymm5, %ymm1, %ymm1
    vperm2i128 $0x20, %ymm6, %ymm2, %ymm2
    vperm2i128 $0x20, %ymm7, %ymm3, %ymm3
    vmovdqa %ymm0, \o0(%rdi)
    vmovdqa %ymm1, \o1(%rdi)
    vmovdqa %ymm2, \o2(%rdi)
    vmovdqa %ymm3, \o3(%rdi)
.endm

/* Inter GS, 4 pairs.  u=(U+V)[0,q), v=mont(U-V)[0,q). */
.macro GS4 u0,u1,u2,u3, v0,v1,v2,v3, z0,z1,z2,z3
    vmovdqa \u0(%rdi), %ymm8
    vmovdqa \u1(%rdi), %ymm9
    vmovdqa \u2(%rdi), %ymm10
    vmovdqa \u3(%rdi), %ymm11
    vmovdqa \v0(%rdi), %ymm0
    vmovdqa \v1(%rdi), %ymm1
    vmovdqa \v2(%rdi), %ymm2
    vmovdqa \v3(%rdi), %ymm3
    vpsubw  %ymm0, %ymm8, %ymm4          /* dif = U-V */
    vpsubw  %ymm1, %ymm9, %ymm5
    vpsubw  %ymm2, %ymm10, %ymm6
    vpsubw  %ymm3, %ymm11, %ymm7
    vpaddw  %ymm0, %ymm8, %ymm8          /* sum = U+V [0,2q) */
    vpaddw  %ymm1, %ymm9, %ymm9
    vpaddw  %ymm2, %ymm10, %ymm10
    vpaddw  %ymm3, %ymm11, %ymm11
    vpsubw  Q, %ymm8, %ymm0
    vpsubw  Q, %ymm9, %ymm1
    vpsubw  Q, %ymm10, %ymm2
    vpsubw  Q, %ymm11, %ymm3
    vpminuw %ymm0, %ymm8, %ymm8          /* sum [0,q) */
    vpminuw %ymm1, %ymm9, %ymm9
    vpminuw %ymm2, %ymm10, %ymm10
    vpminuw %ymm3, %ymm11, %ymm11
    vpmullw \z0(%rdx), %ymm4, %ymm0
    vpmullw \z1(%rdx), %ymm5, %ymm1
    vpmullw \z2(%rdx), %ymm6, %ymm2
    vpmullw \z3(%rdx), %ymm7, %ymm3
    vpmulhw \z0(%rsi), %ymm4, %ymm4
    vpmulhw \z1(%rsi), %ymm5, %ymm5
    vpmulhw \z2(%rsi), %ymm6, %ymm6
    vpmulhw \z3(%rsi), %ymm7, %ymm7
    vpmulhw Q, %ymm0, %ymm0
    vpmulhw Q, %ymm1, %ymm1
    vpmulhw Q, %ymm2, %ymm2
    vpmulhw Q, %ymm3, %ymm3
    vpsubw  %ymm0, %ymm4, %ymm4          /* mont (-q,q) */
    vpsubw  %ymm1, %ymm5, %ymm5
    vpsubw  %ymm2, %ymm6, %ymm6
    vpsubw  %ymm3, %ymm7, %ymm7
    vpaddw  Q, %ymm4, %ymm0
    vpaddw  Q, %ymm5, %ymm1
    vpaddw  Q, %ymm6, %ymm2
    vpaddw  Q, %ymm7, %ymm3
    vpminuw %ymm0, %ymm4, %ymm4          /* mont [0,q) */
    vpminuw %ymm1, %ymm5, %ymm5
    vpminuw %ymm2, %ymm6, %ymm6
    vpminuw %ymm3, %ymm7, %ymm7
    vmovdqa %ymm8, \u0(%rdi)
    vmovdqa %ymm9, \u1(%rdi)
    vmovdqa %ymm10, \u2(%rdi)
    vmovdqa %ymm11, \u3(%rdi)
    vmovdqa %ymm4, \v0(%rdi)
    vmovdqa %ymm5, \v1(%rdi)
    vmovdqa %ymm6, \v2(%rdi)
    vmovdqa %ymm7, \v3(%rdi)
.endm

/* scale by N^-1 (=512) + ->signed.  ymm14 = Q/2. */
.macro SCALE o0,o1,o2,o3
    vmovdqa \o0(%rdi), %ymm0
    vmovdqa \o1(%rdi), %ymm1
    vmovdqa \o2(%rdi), %ymm2
    vmovdqa \o3(%rdi), %ymm3
    vpsllw  $9, %ymm0, %ymm4
    vpsllw  $9, %ymm1, %ymm5
    vpsllw  $9, %ymm2, %ymm6
    vpsllw  $9, %ymm3, %ymm7
    vpsraw  $7, %ymm0, %ymm0
    vpsraw  $7, %ymm1, %ymm1
    vpsraw  $7, %ymm2, %ymm2
    vpsraw  $7, %ymm3, %ymm3
    vpmulhw Q, %ymm4, %ymm4
    vpmulhw Q, %ymm5, %ymm5
    vpmulhw Q, %ymm6, %ymm6
    vpmulhw Q, %ymm7, %ymm7
    vpsubw  %ymm4, %ymm0, %ymm0          /* mont*512 (-q,q) */
    vpsubw  %ymm5, %ymm1, %ymm1
    vpsubw  %ymm6, %ymm2, %ymm2
    vpsubw  %ymm7, %ymm3, %ymm3
    vpaddw  Q, %ymm0, %ymm4
    vpaddw  Q, %ymm1, %ymm5
    vpaddw  Q, %ymm2, %ymm6
    vpaddw  Q, %ymm3, %ymm7
    vpminuw %ymm4, %ymm0, %ymm0          /* [0,q) */
    vpminuw %ymm5, %ymm1, %ymm1
    vpminuw %ymm6, %ymm2, %ymm2
    vpminuw %ymm7, %ymm3, %ymm3
    vpcmpgtw %ymm14, %ymm0, %ymm4        /* -> signed [-q/2,q/2) */
    vpcmpgtw %ymm14, %ymm1, %ymm5
    vpcmpgtw %ymm14, %ymm2, %ymm6
    vpcmpgtw %ymm14, %ymm3, %ymm7
    vpand   Q, %ymm4, %ymm4
    vpand   Q, %ymm5, %ymm5
    vpand   Q, %ymm6, %ymm6
    vpand   Q, %ymm7, %ymm7
    vpsubw  %ymm4, %ymm0, %ymm0
    vpsubw  %ymm5, %ymm1, %ymm1
    vpsubw  %ymm6, %ymm2, %ymm2
    vpsubw  %ymm7, %ymm3, %ymm3
    vmovdqa %ymm0, \o0(%rdi)
    vmovdqa %ymm1, \o1(%rdi)
    vmovdqa %ymm2, \o2(%rdi)
    vmovdqa %ymm3, \o3(%rdi)
.endm

.global ntt_inv_asm
#ifdef ASM_ALIASES
.global ntt_avx_inverse
ntt_avx_inverse:
#endif
ntt_inv_asm:
    vmovdqa vQ(%rip), Q
    leaq    zinv_h(%rip), %rsi
    leaq    zinv_l(%rip), %rdx
    INTRA_BLOCK_INV 0,32,64,96,    0,    512,  1024
    INTRA_BLOCK_INV 128,160,192,224, 128,  640,  1152
    INTRA_BLOCK_INV 256,288,320,352, 256,  768,  1280
    INTRA_BLOCK_INV 384,416,448,480, 384,  896,  1408
    GS4 0,64,128,192,   32,96,160,224,   1536,1568,1600,1632
    GS4 256,320,384,448, 288,352,416,480, 1664,1696,1728,1760
    GS4 0,32,128,160,   64,96,192,224,   1792,1792,1824,1824
    GS4 256,288,384,416, 320,352,448,480, 1856,1856,1888,1888
    GS4 0,32,64,96,   128,160,192,224, 1920,1920,1920,1920
    GS4 256,288,320,352, 384,416,448,480, 1952,1952,1952,1952
    GS4 0,32,64,96,   256,288,320,352, 1984,1984,1984,1984
    GS4 128,160,192,224, 384,416,448,480, 1984,1984,1984,1984
    vmovdqa vQHALF(%rip), %ymm14
    SCALE 0,32,64,96
    SCALE 128,160,192,224
    SCALE 256,288,320,352
    SCALE 384,416,448,480
    vzeroupper
    ret

.section .note.GNU-stack,"",@progbits
