/*
 * x86-64 AVX2 assembly ported (GNU-as) from the pq-crystals ML-KEM (Kyber)
 * AVX2 reference assembly. Third-party building block (pq-crystals;
 * public domain / Apache-2.0). NOT an original contribution of this
 * submission and OUTSIDE the scope of the B.3 ownership statement;
 * the upstream authors retain copyright (full license texts: the upstream pq-crystals project).
 */
/*
 * DKE INVNTT — ML-KEM-style full 7-layer unrolled, operates on packed NTT format.
 * Direct port of ML-KEM's invntt.S with DKE qdata offset adjustment.
 *
 * ML-KEM qdata layout: _ZETAS_EXP at offset 160 (index 80)
 * DKE   qdata layout: _ZETAS_EXP at offset 192 (index 96)
 * Offset delta: +32 bytes (+16 int16)
 *
 * void dke_invntt_dkek_avx2_linux(int16_t *r, const int16_t *qdata);
 *   rdi = r (256 int16, packed NTT format)
 *   rsi = qdata
 *
 * After this call, r is in standard coefficient order (unpacked).
 */

/* DKE qdata offsets (in bytes) */
#define _16XQ           0
#define _16XQINV       32
#define _16XV          64
/* offset 96 = (1<<9), offset 128 = MONT_F */
#define _16XINVNTTF   160    /* 16 x INVNTT_F (1441) = FHI */
#define _ZETAS_EXP    192    /* preexpanded zeta table */

/*
 * FLO = QINV * INVNTT_F mod 2^16 = -3327 * 1441 mod 2^16 = -10079
 * FHI = INVNTT_F = 1441
 * We broadcast these at function entry instead of loading from qdata.
 */
#define FLO_VAL  -10079
#define FHI_VAL   1441

/* ---- Macros (same as ML-KEM) ---- */

.macro shuffle8 r0,r1,r2,r3
    vperm2i128  $0x20,%ymm\r1,%ymm\r0,%ymm\r2
    vperm2i128  $0x31,%ymm\r1,%ymm\r0,%ymm\r3
.endm

.macro shuffle4 r0,r1,r2,r3
    vpunpcklqdq %ymm\r1,%ymm\r0,%ymm\r2
    vpunpckhqdq %ymm\r1,%ymm\r0,%ymm\r3
.endm

.macro shuffle2 r0,r1,r2,r3
    vmovsldup   %ymm\r1,%ymm\r2
    vpblendd    $0xAA,%ymm\r2,%ymm\r0,%ymm\r2
    vpsrlq      $32,%ymm\r0,%ymm\r0
    vpblendd    $0xAA,%ymm\r1,%ymm\r0,%ymm\r3
.endm

.macro shuffle1 r0,r1,r2,r3
    vpslld      $16,%ymm\r1,%ymm\r2
    vpblendw    $0xAA,%ymm\r2,%ymm\r0,%ymm\r2
    vpsrld      $16,%ymm\r0,%ymm\r0
    vpblendw    $0xAA,%ymm\r1,%ymm\r0,%ymm\r3
.endm

.macro red16 r,rs=0,x=12
    vpmulhw     %ymm1,%ymm\r,%ymm\x
.if \rs
    vpmulhrsw   %ymm\rs,%ymm\x,%ymm\x
.else
    vpsraw      $10,%ymm\x,%ymm\x
.endif
    vpmullw     %ymm0,%ymm\x,%ymm\x
    vpsubw      %ymm\x,%ymm\r,%ymm\r
.endm

.macro fqmulprecomp al,ah,b,x=12
    vpmullw     %ymm\al,%ymm\b,%ymm\x
    vpmulhw     %ymm\ah,%ymm\b,%ymm\b
    vpmulhw     %ymm0,%ymm\x,%ymm\x
    vpsubw      %ymm\x,%ymm\b,%ymm\b
.endm

.macro butterfly rl0,rl1,rl2,rl3,rh0,rh1,rh2,rh3,zl0=2,zl1=2,zh0=3,zh1=3
    vpsubw      %ymm\rl0,%ymm\rh0,%ymm12
    vpaddw      %ymm\rh0,%ymm\rl0,%ymm\rl0
    vpsubw      %ymm\rl1,%ymm\rh1,%ymm13
    vpmullw     %ymm\zl0,%ymm12,%ymm\rh0
    vpaddw      %ymm\rh1,%ymm\rl1,%ymm\rl1
    vpsubw      %ymm\rl2,%ymm\rh2,%ymm14
    vpmullw     %ymm\zl0,%ymm13,%ymm\rh1
    vpaddw      %ymm\rh2,%ymm\rl2,%ymm\rl2
    vpsubw      %ymm\rl3,%ymm\rh3,%ymm15
    vpmullw     %ymm\zl1,%ymm14,%ymm\rh2
    vpaddw      %ymm\rh3,%ymm\rl3,%ymm\rl3
    vpmullw     %ymm\zl1,%ymm15,%ymm\rh3
    vpmulhw     %ymm\zh0,%ymm12,%ymm12
    vpmulhw     %ymm\zh0,%ymm13,%ymm13
    vpmulhw     %ymm\zh1,%ymm14,%ymm14
    vpmulhw     %ymm\zh1,%ymm15,%ymm15
    vpmulhw     %ymm0,%ymm\rh0,%ymm\rh0
    vpmulhw     %ymm0,%ymm\rh1,%ymm\rh1
    vpmulhw     %ymm0,%ymm\rh2,%ymm\rh2
    vpmulhw     %ymm0,%ymm\rh3,%ymm\rh3
    vpsubw      %ymm\rh0,%ymm12,%ymm\rh0
    vpsubw      %ymm\rh1,%ymm13,%ymm\rh1
    vpsubw      %ymm\rh2,%ymm14,%ymm\rh2
    vpsubw      %ymm\rh3,%ymm15,%ymm\rh3
.endm

/* ---- INVNTT levels 0-5 (one half of 256 coefficients) ---- */
.macro intt_levels0t5 off
    /* level 0: multiply by f (INVNTT scaling factor)
     * FLO and FHI are broadcast into ymm2/ymm3 at function entry */

    vmovdqa     (128*\off+  0)*2(%rdi),%ymm4
    vmovdqa     (128*\off+ 32)*2(%rdi),%ymm6
    vmovdqa     (128*\off+ 16)*2(%rdi),%ymm5
    vmovdqa     (128*\off+ 48)*2(%rdi),%ymm7

    fqmulprecomp 2,3,4
    fqmulprecomp 2,3,6
    fqmulprecomp 2,3,5
    fqmulprecomp 2,3,7

    vmovdqa     (128*\off+ 64)*2(%rdi),%ymm8
    vmovdqa     (128*\off+ 96)*2(%rdi),%ymm10
    vmovdqa     (128*\off+ 80)*2(%rdi),%ymm9
    vmovdqa     (128*\off+112)*2(%rdi),%ymm11

    fqmulprecomp 2,3,8
    fqmulprecomp 2,3,10
    fqmulprecomp 2,3,9
    fqmulprecomp 2,3,11

    /* level 0 butterfly with zetas */
    vpermq      $0x4E,(_ZETAS_EXP+(1-\off)*224+208)*2(%rsi),%ymm15
    vpermq      $0x4E,(_ZETAS_EXP+(1-\off)*224+176)*2(%rsi),%ymm1
    vpermq      $0x4E,(_ZETAS_EXP+(1-\off)*224+224)*2(%rsi),%ymm2
    vpermq      $0x4E,(_ZETAS_EXP+(1-\off)*224+192)*2(%rsi),%ymm3

    /* byte-reverse the zeta vectors */
    /* Use inline byte-reverse: swap bytes within each 16-bit word */
    /* ML-KEM uses _REVIDXB from qdata; we inline it */
    vmovdqa     .Lrevidxb(%rip),%ymm12
    vpshufb     %ymm12,%ymm15,%ymm15
    vpshufb     %ymm12,%ymm1,%ymm1
    vpshufb     %ymm12,%ymm2,%ymm2
    vpshufb     %ymm12,%ymm3,%ymm3

    butterfly   4,5,8,9,6,7,10,11,15,1,2,3

    /* level 1 */
    vpermq      $0x4E,(_ZETAS_EXP+(1-\off)*224+144)*2(%rsi),%ymm2
    vpermq      $0x4E,(_ZETAS_EXP+(1-\off)*224+160)*2(%rsi),%ymm3
    vmovdqa     .Lrevidxb(%rip),%ymm1
    vpshufb     %ymm1,%ymm2,%ymm2
    vpshufb     %ymm1,%ymm3,%ymm3

    butterfly   4,5,6,7,8,9,10,11,2,2,3,3

    shuffle1    4,5,3,5
    shuffle1    6,7,4,7
    shuffle1    8,9,6,9
    shuffle1    10,11,8,11

    /* level 2 */
    vmovdqa     .Lrevidxd(%rip),%ymm12
    vpermd      (_ZETAS_EXP+(1-\off)*224+112)*2(%rsi),%ymm12,%ymm2
    vpermd      (_ZETAS_EXP+(1-\off)*224+128)*2(%rsi),%ymm12,%ymm10

    butterfly   3,4,6,8,5,7,9,11,2,2,10,10

    vmovdqa     (_16XV)(%rsi),%ymm1
    red16       3

    shuffle2    3,4,10,4
    shuffle2    6,8,3,8
    shuffle2    5,7,6,7
    shuffle2    9,11,5,11

    /* level 3 */
    vpermq      $0x1B,(_ZETAS_EXP+(1-\off)*224+80)*2(%rsi),%ymm2
    vpermq      $0x1B,(_ZETAS_EXP+(1-\off)*224+96)*2(%rsi),%ymm9

    butterfly   10,3,6,5,4,8,7,11,2,2,9,9

    shuffle4    10,3,9,3
    shuffle4    6,5,10,5
    shuffle4    4,8,6,8
    shuffle4    7,11,4,11

    /* level 4 */
    vpermq      $0x4E,(_ZETAS_EXP+(1-\off)*224+48)*2(%rsi),%ymm2
    vpermq      $0x4E,(_ZETAS_EXP+(1-\off)*224+64)*2(%rsi),%ymm7

    butterfly   9,10,6,4,3,5,8,11,2,2,7,7

    red16       9

    shuffle8    9,10,7,10
    shuffle8    6,4,9,4
    shuffle8    3,5,6,5
    shuffle8    8,11,3,11

    /* level 5 */
    vmovdqa     (_ZETAS_EXP+(1-\off)*224+16)*2(%rsi),%ymm2
    vmovdqa     (_ZETAS_EXP+(1-\off)*224+32)*2(%rsi),%ymm8

    butterfly   7,9,6,3,10,4,5,11,2,2,8,8

    vmovdqa     %ymm7,(128*\off+  0)*2(%rdi)
    vmovdqa     %ymm9,(128*\off+ 16)*2(%rdi)
    vmovdqa     %ymm6,(128*\off+ 32)*2(%rdi)
    vmovdqa     %ymm3,(128*\off+ 48)*2(%rdi)
    vmovdqa     %ymm10,(128*\off+ 64)*2(%rdi)
    vmovdqa     %ymm4,(128*\off+ 80)*2(%rdi)
    vmovdqa     %ymm5,(128*\off+ 96)*2(%rdi)
    vmovdqa     %ymm11,(128*\off+112)*2(%rdi)
.endm

/* ---- INVNTT level 6 (cross-half butterfly) ---- */
.macro intt_level6 off
    vmovdqa     (64*\off+  0)*2(%rdi),%ymm4
    vmovdqa     (64*\off+128)*2(%rdi),%ymm8
    vmovdqa     (64*\off+ 16)*2(%rdi),%ymm5
    vmovdqa     (64*\off+144)*2(%rdi),%ymm9
    vpbroadcastq (_ZETAS_EXP+0)*2(%rsi),%ymm2

    vmovdqa     (64*\off+ 32)*2(%rdi),%ymm6
    vmovdqa     (64*\off+160)*2(%rdi),%ymm10
    vmovdqa     (64*\off+ 48)*2(%rdi),%ymm7
    vmovdqa     (64*\off+176)*2(%rdi),%ymm11
    vpbroadcastq (_ZETAS_EXP+4)*2(%rsi),%ymm3

    butterfly   4,5,6,7,8,9,10,11

.if \off == 0
    red16       4
.endif

    vmovdqa     %ymm4,(64*\off+  0)*2(%rdi)
    vmovdqa     %ymm5,(64*\off+ 16)*2(%rdi)
    vmovdqa     %ymm6,(64*\off+ 32)*2(%rdi)
    vmovdqa     %ymm7,(64*\off+ 48)*2(%rdi)
    vmovdqa     %ymm8,(64*\off+128)*2(%rdi)
    vmovdqa     %ymm9,(64*\off+144)*2(%rdi)
    vmovdqa     %ymm10,(64*\off+160)*2(%rdi)
    vmovdqa     %ymm11,(64*\off+176)*2(%rdi)
.endm

/* ---- Main entry point ---- */
.text
.global dke_invntt_dkek_avx2_linux
.type dke_invntt_dkek_avx2_linux, @function
.align 32
dke_invntt_dkek_avx2_linux:
    vmovdqa     (_16XQ)(%rsi),%ymm0

    /* Broadcast FLO and FHI for the initial fqmulprecomp in levels 0-5 */
    mov         $FLO_VAL,%eax
    vmovd       %eax,%xmm2
    vpbroadcastw %xmm2,%ymm2
    mov         $FHI_VAL,%eax
    vmovd       %eax,%xmm3
    vpbroadcastw %xmm3,%ymm3

    intt_levels0t5 0

    /* Re-broadcast FLO/FHI (clobbered by butterfly macros) */
    mov         $FLO_VAL,%eax
    vmovd       %eax,%xmm2
    vpbroadcastw %xmm2,%ymm2
    mov         $FHI_VAL,%eax
    vmovd       %eax,%xmm3
    vpbroadcastw %xmm3,%ymm3

    intt_levels0t5 1

    intt_level6 0
    intt_level6 1

    vzeroupper
    ret
.size dke_invntt_dkek_avx2_linux, .-dke_invntt_dkek_avx2_linux

/* ---- Constant data (inline, avoids qdata dependency for REVIDXB/REVIDXD) ---- */
.section .rodata
.align 32
.Lrevidxb:
    .byte 14,15,12,13,10,11,8,9,6,7,4,5,2,3,0,1
    .byte 14,15,12,13,10,11,8,9,6,7,4,5,2,3,0,1
.Lrevidxd:
    .long 7,6,5,4,3,2,1,0

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
