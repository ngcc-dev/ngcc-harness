/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */

/*
 * BiT-128 AVX2 forward NTT — hand-written assembly.
 * int16 Montgomery, same tables (consts.c zfwd_h/zfwd_l) and natural order
 * as ntt_avx.c, but canonicalisation uses the min-trick
 *   x in [0,2q) -> [0,q):  x' = minu(x, x-q)
 *   x in (-q,q) -> [0,q):  x' = minu(x, x+q)
 * (2 ops vs the 4-op sub/sra/and/add the intrinsics use).
 *   ntt_fwd_asm(int16_t *a)
 */
#define Q      %ymm15
#define BIT_Q       26881

.section .rodata
.p2align 5
vQ:     .rept 16
        .short BIT_Q
        .endr

.text

/* CT butterfly, 4 independent pairs, per-lane zetas, min-trick reductions. */
.macro BFLY4 u0,u1,u2,u3, v0,v1,v2,v3, z0,z1,z2,z3
    vmovdqa \v0(%rdi), %ymm0
    vmovdqa \v1(%rdi), %ymm1
    vmovdqa \v2(%rdi), %ymm2
    vmovdqa \v3(%rdi), %ymm3
    vpmullw \z0(%rdx), %ymm0, %ymm4
    vpmullw \z1(%rdx), %ymm1, %ymm5
    vpmullw \z2(%rdx), %ymm2, %ymm6
    vpmullw \z3(%rdx), %ymm3, %ymm7
    vpmulhw \z0(%rsi), %ymm0, %ymm0
    vpmulhw \z1(%rsi), %ymm1, %ymm1
    vpmulhw \z2(%rsi), %ymm2, %ymm2
    vpmulhw \z3(%rsi), %ymm3, %ymm3
    vpmulhw Q, %ymm4, %ymm4
    vpmulhw Q, %ymm5, %ymm5
    vpmulhw Q, %ymm6, %ymm6
    vpmulhw Q, %ymm7, %ymm7
    vpsubw  %ymm4, %ymm0, %ymm0          /* Vm = h-tt in (-q,q) */
    vpsubw  %ymm5, %ymm1, %ymm1
    vpsubw  %ymm6, %ymm2, %ymm2
    vpsubw  %ymm7, %ymm3, %ymm3
    vpaddw  Q, %ymm0, %ymm4
    vpaddw  Q, %ymm1, %ymm5
    vpaddw  Q, %ymm2, %ymm6
    vpaddw  Q, %ymm3, %ymm7
    vpminuw %ymm4, %ymm0, %ymm0          /* Vm -> [0,q) */
    vpminuw %ymm5, %ymm1, %ymm1
    vpminuw %ymm6, %ymm2, %ymm2
    vpminuw %ymm7, %ymm3, %ymm3
    vmovdqa \u0(%rdi), %ymm8
    vmovdqa \u1(%rdi), %ymm9
    vmovdqa \u2(%rdi), %ymm10
    vmovdqa \u3(%rdi), %ymm11
    vpaddw  %ymm0, %ymm8, %ymm4          /* X = U+Vm in [0,2q) */
    vpaddw  %ymm1, %ymm9, %ymm5
    vpaddw  %ymm2, %ymm10, %ymm6
    vpaddw  %ymm3, %ymm11, %ymm7
    vpsubw  %ymm0, %ymm8, %ymm8          /* Y = U-Vm in (-q,q) */
    vpsubw  %ymm1, %ymm9, %ymm9
    vpsubw  %ymm2, %ymm10, %ymm10
    vpsubw  %ymm3, %ymm11, %ymm11
    vpsubw  Q, %ymm4, %ymm0
    vpsubw  Q, %ymm5, %ymm1
    vpsubw  Q, %ymm6, %ymm2
    vpsubw  Q, %ymm7, %ymm3
    vpminuw %ymm0, %ymm4, %ymm4          /* X -> [0,q) */
    vpminuw %ymm1, %ymm5, %ymm5
    vpminuw %ymm2, %ymm6, %ymm6
    vpminuw %ymm3, %ymm7, %ymm7
    vpaddw  Q, %ymm8, %ymm0
    vpaddw  Q, %ymm9, %ymm1
    vpaddw  Q, %ymm10, %ymm2
    vpaddw  Q, %ymm11, %ymm3
    vpminuw %ymm0, %ymm8, %ymm8          /* Y -> [0,q) */
    vpminuw %ymm1, %ymm9, %ymm9
    vpminuw %ymm2, %ymm10, %ymm10
    vpminuw %ymm3, %ymm11, %ymm11
    vmovdqa %ymm4, \u0(%rdi)
    vmovdqa %ymm5, \u1(%rdi)
    vmovdqa %ymm6, \u2(%rdi)
    vmovdqa %ymm7, \u3(%rdi)
    vmovdqa %ymm8, \v0(%rdi)
    vmovdqa %ymm9, \v1(%rdi)
    vmovdqa %ymm10, \v2(%rdi)
    vmovdqa %ymm11, \v3(%rdi)
.endm

/* signed -> unsigned, 4 regs (min-trick: x + (x>>15 & q) == minu(x,x+q) for (-q,0)) */
.macro S2U o0,o1,o2,o3
    vmovdqa \o0(%rdi), %ymm0
    vmovdqa \o1(%rdi), %ymm1
    vmovdqa \o2(%rdi), %ymm2
    vmovdqa \o3(%rdi), %ymm3
    vpaddw  Q, %ymm0, %ymm4
    vpaddw  Q, %ymm1, %ymm5
    vpaddw  Q, %ymm2, %ymm6
    vpaddw  Q, %ymm3, %ymm7
    vpminuw %ymm4, %ymm0, %ymm0
    vpminuw %ymm5, %ymm1, %ymm1
    vpminuw %ymm6, %ymm2, %ymm2
    vpminuw %ymm7, %ymm3, %ymm3
    vmovdqa %ymm0, \o0(%rdi)
    vmovdqa %ymm1, \o1(%rdi)
    vmovdqa %ymm2, \o2(%rdi)
    vmovdqa %ymm3, \o3(%rdi)
.endm

/* Intra-register CT core, min-trick.  u=U, v=shuffled V -> u=X[0,q), v=Y[0,q). */
.macro PINTRA u, v, z, t
    vpmullw \z(%rdx), \v, \t
    vpmulhw \z(%rsi), \v, \v
    vpmulhw Q, \t, \t
    vpsubw  \t, \v, \v          /* Vm (-q,q) */
    vpaddw  Q, \v, \t
    vpminuw \t, \v, \v          /* Vm [0,q) */
    vpsubw  \v, \u, \t          /* Y = U-V */
    vpaddw  \v, \u, \u          /* X = U+V */
    vpsubw  Q, \u, \v
    vpminuw \v, \u, \u          /* X [0,q) */
    vpaddw  Q, \t, \v
    vpminuw \v, \t, \v          /* Y [0,q) */
.endm

.macro INTRA_BLOCK o0,o1,o2,o3, z5, z6, z7
    vmovdqa \o0(%rdi), %ymm0
    vmovdqa \o1(%rdi), %ymm1
    vmovdqa \o2(%rdi), %ymm2
    vmovdqa \o3(%rdi), %ymm3
    vperm2i128 $0x11, %ymm0, %ymm0, %ymm4
    vperm2i128 $0x11, %ymm1, %ymm1, %ymm5
    vperm2i128 $0x11, %ymm2, %ymm2, %ymm6
    vperm2i128 $0x11, %ymm3, %ymm3, %ymm7
    PINTRA %ymm0, %ymm4, \z5,    %ymm8
    PINTRA %ymm1, %ymm5, \z5+32, %ymm9
    PINTRA %ymm2, %ymm6, \z5+64, %ymm10
    PINTRA %ymm3, %ymm7, \z5+96, %ymm11
    vperm2i128 $0x20, %ymm4, %ymm0, %ymm0
    vperm2i128 $0x20, %ymm5, %ymm1, %ymm1
    vperm2i128 $0x20, %ymm6, %ymm2, %ymm2
    vperm2i128 $0x20, %ymm7, %ymm3, %ymm3
    vpshufd $0xEE, %ymm0, %ymm4
    vpshufd $0xEE, %ymm1, %ymm5
    vpshufd $0xEE, %ymm2, %ymm6
    vpshufd $0xEE, %ymm3, %ymm7
    PINTRA %ymm0, %ymm4, \z6,    %ymm8
    PINTRA %ymm1, %ymm5, \z6+32, %ymm9
    PINTRA %ymm2, %ymm6, \z6+64, %ymm10
    PINTRA %ymm3, %ymm7, \z6+96, %ymm11
    vpunpcklqdq %ymm4, %ymm0, %ymm0
    vpunpcklqdq %ymm5, %ymm1, %ymm1
    vpunpcklqdq %ymm6, %ymm2, %ymm2
    vpunpcklqdq %ymm7, %ymm3, %ymm3
    vpshufd $0xF5, %ymm0, %ymm4
    vpshufd $0xF5, %ymm1, %ymm5
    vpshufd $0xF5, %ymm2, %ymm6
    vpshufd $0xF5, %ymm3, %ymm7
    PINTRA %ymm0, %ymm4, \z7,    %ymm8
    PINTRA %ymm1, %ymm5, \z7+32, %ymm9
    PINTRA %ymm2, %ymm6, \z7+64, %ymm10
    PINTRA %ymm3, %ymm7, \z7+96, %ymm11
    vpshufd $0xB1, %ymm4, %ymm4
    vpshufd $0xB1, %ymm5, %ymm5
    vpshufd $0xB1, %ymm6, %ymm6
    vpshufd $0xB1, %ymm7, %ymm7
    vpblendd $0xAA, %ymm4, %ymm0, %ymm0
    vpblendd $0xAA, %ymm5, %ymm1, %ymm1
    vpblendd $0xAA, %ymm6, %ymm2, %ymm2
    vpblendd $0xAA, %ymm7, %ymm3, %ymm3
    vmovdqa %ymm0, \o0(%rdi)
    vmovdqa %ymm1, \o1(%rdi)
    vmovdqa %ymm2, \o2(%rdi)
    vmovdqa %ymm3, \o3(%rdi)
.endm

.global ntt_fwd_asm
#ifdef ASM_ALIASES
.global ntt_avx_forward
.global ntt_avx_forward_stages
ntt_avx_forward:
ntt_avx_forward_stages:
#endif
ntt_fwd_asm:
    vmovdqa vQ(%rip), Q
    leaq    zfwd_h(%rip), %rsi
    leaq    zfwd_l(%rip), %rdx
    S2U 0,32,64,96
    S2U 128,160,192,224
    S2U 256,288,320,352
    S2U 384,416,448,480
    /* L0 t=128 */
    BFLY4 0,32,64,96,   256,288,320,352,  0,0,0,0
    BFLY4 128,160,192,224, 384,416,448,480, 0,0,0,0
    /* L1 t=64 */
    BFLY4 0,32,64,96,   128,160,192,224, 32,32,32,32
    BFLY4 256,288,320,352, 384,416,448,480, 64,64,64,64
    /* L2 t=32 */
    BFLY4 0,32,128,160,   64,96,192,224,   96,96,128,128
    BFLY4 256,288,384,416, 320,352,448,480, 160,160,192,192
    /* L3 t=16 */
    BFLY4 0,64,128,192,   32,96,160,224,   224,256,288,320
    BFLY4 256,320,384,448, 288,352,416,480, 352,384,416,448
    /* L5/L6/L7 intra-register */
    INTRA_BLOCK 0,32,64,96,    480, 992, 1504
    INTRA_BLOCK 128,160,192,224, 608, 1120, 1632
    INTRA_BLOCK 256,288,320,352, 736, 1248, 1760
    INTRA_BLOCK 384,416,448,480, 864, 1376, 1888
    vzeroupper
    ret

.section .note.GNU-stack,"",@progbits
