/* AVX2 forward NTT for DKE-512 (n=512, q=7681) — hand-scheduled, register-blocked.
 * GAS / AT&T syntax (source ... dest).  Mirrors dke_ntt512_rb (verified KAT==ref).
 *
 * Decomposition: layer len=256 couples the two 256-coeff halves; layer len=128
 * splits each half into two 128-coeff blocks. The remaining layers (len<=64) stay
 * inside a 128-coeff block, so the transform becomes FOUR independent 128-coeff
 * blocks (8 YMM vectors each: 8 data + Q/QINV + temps <= 16 YMM) that are fully
 * register-resident — the win the compiler throws away by spilling in intrinsics.
 *
 * void dke_ntt512_gas(int16_t *r, const int16_t *qdata512, const int16_t *zetas);
 *   SysV: rdi=r (int16[512]), rsi=qdata512 ([0..15]=Q,[16..31]=QINV), rdx=zetas[256].
 * q=7681 Montgomery butterflies. ymm15=Q, ymm14=QINV throughout.
 *
 * Canonical zeta order (== generic NTT, so KAT==ref):
 *   len256: z[1]   len128: z[2],z[3]   block b: len64 z[4+b], len32 z[8+2b..],
 *   len16 z[16+4b..], len8 z[32+8b+i], len4 z[64+16b+2i..], len2 z[128+32b+4i..]. */

.section .rodata
.p2align 5
z2idx:  /* expand 4 zetas (broadcast to both 128-lanes) to [z0x4,z1x4|z2x4,z3x4] */
.byte 0,1,0,1,0,1,0,1, 2,3,2,3,2,3,2,3
.byte 4,5,4,5,4,5,4,5, 6,7,6,7,6,7,6,7
z2idxr: /* reversed expand: loaded [z0,z1,z2,z3] -> [z3x4,z2x4|z1x4,z0x4] (inverse) */
.byte 6,7,6,7,6,7,6,7, 4,5,4,5,4,5,4,5
.byte 2,3,2,3,2,3,2,3, 0,1,0,1,0,1,0,1
.p2align 4
zshlo:  .byte 0,1,0,1,0,1,0,1, 2,3,2,3,2,3,2,3   /* basemul zeta expand, low 128 */
zshhi:  .byte 4,5,4,5,4,5,4,5, 6,7,6,7,6,7,6,7   /* basemul zeta expand, high 128 */
.p2align 5
zsign:  .word 1,1,-1,-1, 1,1,-1,-1, 1,1,-1,-1, 1,1,-1,-1  /* +z,+z,-z,-z per pair */

.text

/* Cooley-Tukey butterfly: t=montmul(zeta,b); b=a-t; a=a+t.  Q=ymm15, QINV=ymm14. */
.macro CTBF a, b, zeta, t0, t1
    vpmullw  \b, \zeta, \t0
    vpmulhw  \b, \zeta, \t1
    vpmullw  %ymm14, \t0, \t0
    vpmulhw  %ymm15, \t0, \t0
    vpsubw   \t0, \t1, \t0
    vpsubw   \t0, \a, \b
    vpaddw   \t0, \a, \a
.endm

/* within-vector layers len=8,4,2 for vector ymm<n> (n = local vector index 0..7).
 * zeta pointers: r10=&z[len8base], r11=&z[len4base], rcx=&z[len2base]; ymm12=z2idx.
 * temps ymm8..ymm13 (ymm12 preserved). */
.macro WV n
    /* len=8: low128 <-> high128 */
    vpbroadcastw (\n*2)(%r10), %ymm8
    vextracti128 $1, %ymm\n, %xmm9              /* b = high128 */
    vpmullw  %xmm9, %xmm8, %xmm10
    vpmulhw  %xmm9, %xmm8, %xmm11
    vpmullw  %xmm14, %xmm10, %xmm10
    vpmulhw  %xmm15, %xmm10, %xmm10
    vpsubw   %xmm10, %xmm11, %xmm10             /* t = hi - . */
    vpaddw   %xmm10, %xmm\n, %xmm11             /* new_low = alo + t */
    vpsubw   %xmm10, %xmm\n, %xmm9              /* new_high = alo - t */
    vinserti128 $1, %xmm9, %ymm11, %ymm\n
    /* len=4: adjacent 64-bit lanes; z4=[za x8 | zb x8] */
    vpbroadcastw (\n*4)(%r11), %ymm8
    vpbroadcastw (\n*4+2)(%r11), %ymm9
    vpblendd $0xF0, %ymm9, %ymm8, %ymm8         /* low128=za, high128=zb */
    vpunpcklqdq %ymm\n, %ymm\n, %ymm10          /* a */
    vpunpckhqdq %ymm\n, %ymm\n, %ymm11          /* b */
    vpmullw  %ymm11, %ymm8, %ymm9
    vpmulhw  %ymm11, %ymm8, %ymm13
    vpmullw  %ymm14, %ymm9, %ymm9
    vpmulhw  %ymm15, %ymm9, %ymm9
    vpsubw   %ymm9, %ymm13, %ymm9               /* t */
    vpaddw   %ymm9, %ymm10, %ymm13              /* a+t */
    vpsubw   %ymm9, %ymm10, %ymm10              /* a-t */
    vpunpcklqdq %ymm10, %ymm13, %ymm\n          /* unpacklo64(a+t, a-t) */
    /* len=2: adjacent 32-bit lanes; z2=expand(4 zetas) */
    vmovq    (\n*8)(%rcx), %xmm8
    vpbroadcastq %xmm8, %ymm8
    vpshufb  %ymm12, %ymm8, %ymm8               /* z2 */
    vpshufd  $0xA0, %ymm\n, %ymm10              /* a */
    vpshufd  $0xF5, %ymm\n, %ymm11              /* b */
    vpmullw  %ymm11, %ymm8, %ymm9
    vpmulhw  %ymm11, %ymm8, %ymm13
    vpmullw  %ymm14, %ymm9, %ymm9
    vpmulhw  %ymm15, %ymm9, %ymm9
    vpsubw   %ymm9, %ymm13, %ymm9               /* t */
    vpaddw   %ymm9, %ymm10, %ymm13              /* a+t */
    vpsubw   %ymm9, %ymm10, %ymm10              /* a-t */
    vpblendd $0xAA, %ymm10, %ymm13, %ymm\n      /* blend(a+t, a-t, 0xAA) */
.endm

.global dke_ntt512_gas
.hidden dke_ntt512_gas
dke_ntt512_gas:
    vmovdqa  (%rsi), %ymm15                     /* Q */
    vmovdqa  32(%rsi), %ymm14                   /* QINV */

    /* ---- layer len=256: r[16m] <-> r[256+16m], zeta=z[1] ---- */
    vpbroadcastw 2(%rdx), %ymm13
    xor      %eax, %eax
1:
    vmovdqa  (%rdi,%rax), %ymm0
    vmovdqa  512(%rdi,%rax), %ymm1
    CTBF     %ymm0, %ymm1, %ymm13, %ymm2, %ymm3
    vmovdqa  %ymm0, (%rdi,%rax)
    vmovdqa  %ymm1, 512(%rdi,%rax)
    add      $32, %rax
    cmp      $512, %rax
    jb       1b

    /* ---- layer len=128: group0 z[2] (bytes 0..255 <-> +256),
     *                     group1 z[3] (bytes 512..767 <-> +256) ---- */
    vpbroadcastw 4(%rdx), %ymm13                /* z[2] */
    xor      %eax, %eax
2:
    vmovdqa  (%rdi,%rax), %ymm0
    vmovdqa  256(%rdi,%rax), %ymm1
    CTBF     %ymm0, %ymm1, %ymm13, %ymm2, %ymm3
    vmovdqa  %ymm0, (%rdi,%rax)
    vmovdqa  %ymm1, 256(%rdi,%rax)
    add      $32, %rax
    cmp      $256, %rax
    jb       2b
    vpbroadcastw 6(%rdx), %ymm13                /* z[3] */
    mov      $512, %eax
3:
    vmovdqa  (%rdi,%rax), %ymm0
    vmovdqa  256(%rdi,%rax), %ymm1
    CTBF     %ymm0, %ymm1, %ymm13, %ymm2, %ymm3
    vmovdqa  %ymm0, (%rdi,%rax)
    vmovdqa  %ymm1, 256(%rdi,%rax)
    add      $32, %rax
    cmp      $768, %rax
    jb       3b

    /* ---- four register-resident 128-coeff blocks ---- */
    xor      %r8d, %r8d
.block_loop:
    mov      %r8, %r9
    shl      $8, %r9                            /* block base = rdi + b*256 */
    add      %rdi, %r9
    vmovdqa  (%r9),    %ymm0
    vmovdqa  32(%r9),  %ymm1
    vmovdqa  64(%r9),  %ymm2
    vmovdqa  96(%r9),  %ymm3
    vmovdqa  128(%r9), %ymm4
    vmovdqa  160(%r9), %ymm5
    vmovdqa  192(%r9), %ymm6
    vmovdqa  224(%r9), %ymm7

    /* len=64: z[4+b]; v[m]<->v[m+4] */
    lea      4(%r8), %r10
    lea      (%rdx,%r10,2), %r10
    vpbroadcastw (%r10), %ymm13
    CTBF     %ymm0, %ymm4, %ymm13, %ymm8, %ymm9
    CTBF     %ymm1, %ymm5, %ymm13, %ymm8, %ymm9
    CTBF     %ymm2, %ymm6, %ymm13, %ymm8, %ymm9
    CTBF     %ymm3, %ymm7, %ymm13, %ymm8, %ymm9

    /* len=32: z[8+2b],z[8+2b+1]; (0<->2,1<->3),(4<->6,5<->7) */
    lea      (%r8,%r8), %r10
    add      $8, %r10
    lea      (%rdx,%r10,2), %r10
    vpbroadcastw (%r10), %ymm13
    CTBF     %ymm0, %ymm2, %ymm13, %ymm8, %ymm9
    CTBF     %ymm1, %ymm3, %ymm13, %ymm8, %ymm9
    vpbroadcastw 2(%r10), %ymm13
    CTBF     %ymm4, %ymm6, %ymm13, %ymm8, %ymm9
    CTBF     %ymm5, %ymm7, %ymm13, %ymm8, %ymm9

    /* len=16: z[16+4b..+3]; (0<->1),(2<->3),(4<->5),(6<->7) */
    mov      %r8, %r10
    shl      $2, %r10
    add      $16, %r10
    lea      (%rdx,%r10,2), %r10
    vpbroadcastw (%r10), %ymm13
    CTBF     %ymm0, %ymm1, %ymm13, %ymm8, %ymm9
    vpbroadcastw 2(%r10), %ymm13
    CTBF     %ymm2, %ymm3, %ymm13, %ymm8, %ymm9
    vpbroadcastw 4(%r10), %ymm13
    CTBF     %ymm4, %ymm5, %ymm13, %ymm8, %ymm9
    vpbroadcastw 6(%r10), %ymm13
    CTBF     %ymm6, %ymm7, %ymm13, %ymm8, %ymm9

    /* within-vector layers — set up zeta base pointers */
    mov      %r8, %r10
    shl      $3, %r10
    add      $32, %r10
    lea      (%rdx,%r10,2), %r10                /* &z[32+8b] */
    mov      %r8, %r11
    shl      $4, %r11
    add      $64, %r11
    lea      (%rdx,%r11,2), %r11                /* &z[64+16b] */
    mov      %r8, %rcx
    shl      $5, %rcx
    add      $128, %rcx
    lea      (%rdx,%rcx,2), %rcx                /* &z[128+32b] */
    vmovdqa  z2idx(%rip), %ymm12

    WV 0
    WV 1
    WV 2
    WV 3
    WV 4
    WV 5
    WV 6
    WV 7

    vmovdqa  %ymm0, (%r9)
    vmovdqa  %ymm1, 32(%r9)
    vmovdqa  %ymm2, 64(%r9)
    vmovdqa  %ymm3, 96(%r9)
    vmovdqa  %ymm4, 128(%r9)
    vmovdqa  %ymm5, 160(%r9)
    vmovdqa  %ymm6, 192(%r9)
    vmovdqa  %ymm7, 224(%r9)

    inc      %r8
    cmp      $4, %r8
    jb       .block_loop

    vzeroupper
    ret

/* ============================================================================
 * Inverse NTT (Gentleman-Sande) for n=512, q=7681.
 * void dke_invntt512_gas(int16_t *r, const int16_t *qdata512, const int16_t *zetas);
 * Constants read as memory operands from (rsi): Q@0, QINV@32, V@64, 1<<9 @96, f@128.
 * Mirrors the verified DKE_AVX2_NTT_INTRINSIC inverse: per-vector len=2,4,8 then
 * cross-vector len=16,32,64 inside each 128-block, then len=128,256, then xf.
 * zeta order (descending): len2 z[255-32b-4i..], len4 z[127-16b-2i..], len8 z[63-8b-i],
 * len16 z[31-4b-j], len32 z[15-2b-j], len64 z[7-b], len128 z[3],z[2], len256 z[1]. */

/* GS butterfly (256-bit): t=b-a; a'=barrett(a+b); b'=montmul(zeta, t).  rsi=qdata. */
.macro GSBF a, b, zeta, t0, t1
    vpsubw   \a, \b, \t0
    vpaddw   \b, \a, \a
    vpmulhw  64(%rsi), \a, \t1
    vpaddw   96(%rsi), \t1, \t1
    vpsraw   $10, \t1, \t1
    vpmullw  (%rsi), \t1, \t1
    vpsubw   \t1, \a, \a
    vpmullw  \t0, \zeta, \t1
    vpmulhw  \t0, \zeta, \t0
    vpmullw  32(%rsi), \t1, \t1
    vpmulhw  (%rsi), \t1, \t1
    vpsubw   \t1, \t0, \b
.endm

/* within-vector inverse layers len=2,4,8 for vector ymm<n>.
 * r10=&z[252-32b], r11=&z[126-16b], rcx=&z[63-8b]; ymm12=z2idxr. temps ymm8..11,13. */
.macro WVI n
    /* len=2 GS */
    vmovq    (-8*\n)(%r10), %xmm8
    vpbroadcastq %xmm8, %ymm8
    vpshufb  %ymm12, %ymm8, %ymm8
    vpshufd  $0xA0, %ymm\n, %ymm9        /* a */
    vpshufd  $0xF5, %ymm\n, %ymm10       /* b */
    vpsubw   %ymm9, %ymm10, %ymm11       /* b - a */
    vpaddw   %ymm10, %ymm9, %ymm9        /* a + b */
    vpmulhw  64(%rsi), %ymm9, %ymm13
    vpaddw   96(%rsi), %ymm13, %ymm13
    vpsraw   $10, %ymm13, %ymm13
    vpmullw  (%rsi), %ymm13, %ymm13
    vpsubw   %ymm13, %ymm9, %ymm9        /* add' = barrett(a+b) */
    vpmullw  %ymm11, %ymm8, %ymm13
    vpmulhw  %ymm11, %ymm8, %ymm11
    vpmullw  32(%rsi), %ymm13, %ymm13
    vpmulhw  (%rsi), %ymm13, %ymm13
    vpsubw   %ymm13, %ymm11, %ymm10      /* sub' = montmul(z2, b-a) */
    vpblendd $0xAA, %ymm10, %ymm9, %ymm\n
    /* len=4 GS: za=z[K4] (high addr), zb=z[K4-1] */
    vpbroadcastw (2-4*\n)(%r11), %ymm8   /* za */
    vpbroadcastw (-4*\n)(%r11), %ymm9    /* zb */
    vpblendd $0xF0, %ymm9, %ymm8, %ymm8  /* low=za, high=zb */
    vpunpcklqdq %ymm\n, %ymm\n, %ymm9    /* a */
    vpunpckhqdq %ymm\n, %ymm\n, %ymm10   /* b */
    vpsubw   %ymm9, %ymm10, %ymm11
    vpaddw   %ymm10, %ymm9, %ymm9
    vpmulhw  64(%rsi), %ymm9, %ymm13
    vpaddw   96(%rsi), %ymm13, %ymm13
    vpsraw   $10, %ymm13, %ymm13
    vpmullw  (%rsi), %ymm13, %ymm13
    vpsubw   %ymm13, %ymm9, %ymm9
    vpmullw  %ymm11, %ymm8, %ymm13
    vpmulhw  %ymm11, %ymm8, %ymm11
    vpmullw  32(%rsi), %ymm13, %ymm13
    vpmulhw  (%rsi), %ymm13, %ymm13
    vpsubw   %ymm13, %ymm11, %ymm10
    vpunpcklqdq %ymm10, %ymm9, %ymm\n
    /* len=8 GS: low128<->high128, z8=z[63-8b-n] (128-bit ops) */
    vpbroadcastw (-2*\n)(%rcx), %ymm8
    vextracti128 $1, %ymm\n, %xmm10      /* b = high128 */
    vpsubw   %xmm\n, %xmm10, %xmm11      /* b - a */
    vpaddw   %xmm10, %xmm\n, %xmm9       /* a + b */
    vpmulhw  64(%rsi), %xmm9, %xmm13
    vpaddw   96(%rsi), %xmm13, %xmm13
    vpsraw   $10, %xmm13, %xmm13
    vpmullw  (%rsi), %xmm13, %xmm13
    vpsubw   %xmm13, %xmm9, %xmm9        /* add' (low128) */
    vpmullw  %xmm11, %xmm8, %xmm13
    vpmulhw  %xmm11, %xmm8, %xmm11
    vpmullw  32(%rsi), %xmm13, %xmm13
    vpmulhw  (%rsi), %xmm13, %xmm13
    vpsubw   %xmm13, %xmm11, %xmm10      /* sub' (low128) */
    vinserti128 $1, %xmm10, %ymm9, %ymm\n
.endm

.global dke_invntt512_gas
.hidden dke_invntt512_gas
dke_invntt512_gas:
    vmovdqa  z2idxr(%rip), %ymm12
    xor      %r8d, %r8d
.iblock_loop:
    mov      %r8, %r9
    shl      $8, %r9
    add      %rdi, %r9
    vmovdqa  (%r9),    %ymm0
    vmovdqa  32(%r9),  %ymm1
    vmovdqa  64(%r9),  %ymm2
    vmovdqa  96(%r9),  %ymm3
    vmovdqa  128(%r9), %ymm4
    vmovdqa  160(%r9), %ymm5
    vmovdqa  192(%r9), %ymm6
    vmovdqa  224(%r9), %ymm7

    /* within-vector zeta base pointers */
    mov      $252, %r10d
    mov      %r8, %rax
    shl      $5, %rax
    sub      %rax, %r10
    lea      (%rdx,%r10,2), %r10        /* &z[252-32b] */
    mov      $126, %r11d
    mov      %r8, %rax
    shl      $4, %rax
    sub      %rax, %r11
    lea      (%rdx,%r11,2), %r11        /* &z[126-16b] */
    mov      $63, %ecx
    mov      %r8, %rax
    shl      $3, %rax
    sub      %rax, %rcx
    lea      (%rdx,%rcx,2), %rcx        /* &z[63-8b] */

    WVI 0
    WVI 1
    WVI 2
    WVI 3
    WVI 4
    WVI 5
    WVI 6
    WVI 7

    /* len=16: (0,1)z[31-4b] (2,3)-1 (4,5)-2 (6,7)-3 */
    mov      $31, %r10d
    mov      %r8, %rax
    shl      $2, %rax
    sub      %rax, %r10
    lea      (%rdx,%r10,2), %r10
    vpbroadcastw (%r10), %ymm13
    GSBF     %ymm0, %ymm1, %ymm13, %ymm8, %ymm9
    vpbroadcastw -2(%r10), %ymm13
    GSBF     %ymm2, %ymm3, %ymm13, %ymm8, %ymm9
    vpbroadcastw -4(%r10), %ymm13
    GSBF     %ymm4, %ymm5, %ymm13, %ymm8, %ymm9
    vpbroadcastw -6(%r10), %ymm13
    GSBF     %ymm6, %ymm7, %ymm13, %ymm8, %ymm9

    /* len=32: (0,2)(1,3) z[15-2b]; (4,6)(5,7) z[15-2b-1] */
    mov      $15, %r10d
    mov      %r8, %rax
    shl      $1, %rax
    sub      %rax, %r10
    lea      (%rdx,%r10,2), %r10
    vpbroadcastw (%r10), %ymm13
    GSBF     %ymm0, %ymm2, %ymm13, %ymm8, %ymm9
    GSBF     %ymm1, %ymm3, %ymm13, %ymm8, %ymm9
    vpbroadcastw -2(%r10), %ymm13
    GSBF     %ymm4, %ymm6, %ymm13, %ymm8, %ymm9
    GSBF     %ymm5, %ymm7, %ymm13, %ymm8, %ymm9

    /* len=64: (0,4)(1,5)(2,6)(3,7) z[7-b] */
    mov      $7, %r10d
    sub      %r8, %r10
    lea      (%rdx,%r10,2), %r10
    vpbroadcastw (%r10), %ymm13
    GSBF     %ymm0, %ymm4, %ymm13, %ymm8, %ymm9
    GSBF     %ymm1, %ymm5, %ymm13, %ymm8, %ymm9
    GSBF     %ymm2, %ymm6, %ymm13, %ymm8, %ymm9
    GSBF     %ymm3, %ymm7, %ymm13, %ymm8, %ymm9

    vmovdqa  %ymm0, (%r9)
    vmovdqa  %ymm1, 32(%r9)
    vmovdqa  %ymm2, 64(%r9)
    vmovdqa  %ymm3, 96(%r9)
    vmovdqa  %ymm4, 128(%r9)
    vmovdqa  %ymm5, 160(%r9)
    vmovdqa  %ymm6, 192(%r9)
    vmovdqa  %ymm7, 224(%r9)

    inc      %r8
    cmp      $4, %r8
    jb       .iblock_loop

    /* len=128: group0 z[3] (bytes 0..255 <-> +256), group1 z[2] (512..767 <-> +256) */
    vpbroadcastw 6(%rdx), %ymm13
    xor      %eax, %eax
4:
    vmovdqa  (%rdi,%rax), %ymm0
    vmovdqa  256(%rdi,%rax), %ymm1
    GSBF     %ymm0, %ymm1, %ymm13, %ymm2, %ymm3
    vmovdqa  %ymm0, (%rdi,%rax)
    vmovdqa  %ymm1, 256(%rdi,%rax)
    add      $32, %rax
    cmp      $256, %rax
    jb       4b
    vpbroadcastw 4(%rdx), %ymm13
    mov      $512, %eax
5:
    vmovdqa  (%rdi,%rax), %ymm0
    vmovdqa  256(%rdi,%rax), %ymm1
    GSBF     %ymm0, %ymm1, %ymm13, %ymm2, %ymm3
    vmovdqa  %ymm0, (%rdi,%rax)
    vmovdqa  %ymm1, 256(%rdi,%rax)
    add      $32, %rax
    cmp      $768, %rax
    jb       5b

    /* len=256: z[1] (bytes 0..511 <-> +512) */
    vpbroadcastw 2(%rdx), %ymm13
    xor      %eax, %eax
6:
    vmovdqa  (%rdi,%rax), %ymm0
    vmovdqa  512(%rdi,%rax), %ymm1
    GSBF     %ymm0, %ymm1, %ymm13, %ymm2, %ymm3
    vmovdqa  %ymm0, (%rdi,%rax)
    vmovdqa  %ymm1, 512(%rdi,%rax)
    add      $32, %rax
    cmp      $512, %rax
    jb       6b

    /* final scaling: r[j] = montmul(r[j], f), f@128(%rsi) */
    xor      %eax, %eax
7:
    vmovdqa  (%rdi,%rax), %ymm0
    vpmullw  128(%rsi), %ymm0, %ymm1
    vpmulhw  128(%rsi), %ymm0, %ymm2
    vpmullw  32(%rsi), %ymm1, %ymm1
    vpmulhw  (%rsi), %ymm1, %ymm1
    vpsubw   %ymm1, %ymm2, %ymm0
    vmovdqa  %ymm0, (%rdi,%rax)
    add      $32, %rax
    cmp      $1024, %rax
    jb       7b

    vzeroupper
    ret

/* ============================================================================
 * Pointwise basemul in NTT domain for n=512, q=7681 (256 degree-2 products).
 * void dke_basemul512_gas(int16_t *r, const int16_t *a, const int16_t *b,
 *                         const int16_t *qdata512, const int16_t *zp);
 *   SysV: rdi=r, rsi=a, rdx=b, rcx=qdata512 (Q@0,QINV@32), r8=zp (=DKE_zetas+128).
 * Per 16-coeff vector (8 pairs): r0 = a0*b0 + zeta*(a1*b1), r1 = a0*b1 + a1*b0,
 * adjacent pairs share +-zeta. Mirrors the intrinsic fallback (KAT==ref). */

/* Montgomery multiply out = montmul(x,y); hi computed first so out may alias x. */
.macro FQMUL out, x, y, t
    vpmulhw  \y, \x, \t
    vpmullw  \y, \x, \out
    vpmullw  32(%rcx), \out, \out
    vpmulhw  (%rcx), \out, \out
    vpsubw   \out, \t, \out
.endm

.global dke_basemul512_gas
.hidden dke_basemul512_gas
dke_basemul512_gas:
    xor      %eax, %eax              /* data byte offset 0..992 step 32 */
    mov      %r8, %r9               /* zp pointer, +8 per vector */
.bm_loop:
    vmovdqa  (%rsi,%rax), %ymm0      /* av */
    vmovdqa  (%rdx,%rax), %ymm1      /* bv */
    vpshuflw $0xA0, %ymm0, %ymm2
    vpshufhw $0xA0, %ymm2, %ymm2     /* a0 (even, duplicated) */
    vpshuflw $0xF5, %ymm0, %ymm3
    vpshufhw $0xF5, %ymm3, %ymm3     /* a1 (odd) */
    vpshuflw $0xA0, %ymm1, %ymm4
    vpshufhw $0xA0, %ymm4, %ymm4     /* b0 */
    vpshuflw $0xF5, %ymm1, %ymm5
    vpshufhw $0xF5, %ymm5, %ymm5     /* b1 */
    vmovq    (%r9), %xmm7
    vpshufb  zshlo(%rip), %xmm7, %xmm8
    vpshufb  zshhi(%rip), %xmm7, %xmm9
    vinserti128 $1, %xmm9, %ymm8, %ymm6
    vpsignw  zsign(%rip), %ymm6, %ymm6   /* zv = +-zeta per pair */
    FQMUL    %ymm10, %ymm3, %ymm5, %ymm11   /* a1*b1 */
    FQMUL    %ymm10, %ymm10, %ymm6, %ymm11  /* *zeta */
    FQMUL    %ymm12, %ymm2, %ymm4, %ymm11   /* a0*b0 */
    vpaddw   %ymm10, %ymm12, %ymm10         /* r0 */
    FQMUL    %ymm12, %ymm2, %ymm5, %ymm11   /* a0*b1 */
    FQMUL    %ymm13, %ymm3, %ymm4, %ymm11   /* a1*b0 */
    vpaddw   %ymm12, %ymm13, %ymm12         /* r1 */
    vpblendw $0xAA, %ymm12, %ymm10, %ymm10  /* interleave r0(even)/r1(odd) */
    vmovdqa  %ymm10, (%rdi,%rax)
    add      $8, %r9
    add      $32, %rax
    cmp      $1024, %rax
    jb       .bm_loop
    vzeroupper
    ret

.section .note.GNU-stack,"",@progbits
