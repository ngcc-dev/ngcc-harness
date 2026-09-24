/* ============================================================
   BIT INTT (inverse) – AVX2, q=119297, n=512
   Gentleman-Sande, strides 2,4,8,16,32,64,128,256, then * N^{-1}.
   Input and output in NATURAL order (inverse of ntt_avx).
   ============================================================ */

#include "consts.h"
.include "shuffle.inc"

.macro butterfly_inv l,h,zl0=1,zl1=1,zh0=2,zh1=2
    vpaddd    %ymm\h, %ymm\l, %ymm13       /* sum  -> ymm13 */
    vpsubd    %ymm\h, %ymm\l, %ymm14       /* diff -> ymm14 */

    vpmuldq   %ymm\zh0, %ymm14, %ymm12     /* zh0 * diff_even */
    vmovshdup %ymm14, %ymm\l               /* diff_odd */
    vpmuldq   %ymm\zh1, %ymm\l, %ymm\h     /* zh1 * diff_odd */

    vpmuldq   %ymm\zl0, %ymm14, %ymm14     /* zl0 * diff_even */
    vpmuldq   %ymm0, %ymm14, %ymm14        /* q * ... */
    vpmuldq   %ymm\zl1, %ymm\l, %ymm\l     /* zl1 * diff_odd */
    vpmuldq   %ymm0, %ymm\l, %ymm\l        /* q * ... */

    vmovshdup %ymm12, %ymm12
    vmovshdup %ymm14, %ymm14
    vmovshdup %ymm\h, %ymm\h

    vpblendd  $0xAA, %ymm\h, %ymm12, %ymm12
    vpblendd  $0xAA, %ymm\l, %ymm14, %ymm14

    vpsubd    %ymm14, %ymm12, %ymm\h        /* h = montred(zeta*diff) = cooked - corr */
    vmovdqa   %ymm13, %ymm\l                /* l = sum */
.endm

/* Phase B: within 64-coeff block, strides 2,4 (intra) + 8,16,32 (cross).
   Si lane j = a[64*off + 8*i + j]. */
.macro inv_phaseB off
    vmovdqa  256*\off+  0(%rdi), %ymm4
    vmovdqa  256*\off+ 32(%rdi), %ymm5
    vmovdqa  256*\off+ 64(%rdi), %ymm6
    vmovdqa  256*\off+ 96(%rdi), %ymm7
    vmovdqa  256*\off+128(%rdi), %ymm8
    vmovdqa  256*\off+160(%rdi), %ymm9
    vmovdqa  256*\off+192(%rdi), %ymm10
    vmovdqa  256*\off+224(%rdi), %ymm11

    /* t=2 (stride 2, intra) via shuffle4 ; vector base 320+32off+8g */
    shuffle4 4,5,3,15
    vmovdqa (_ZETAS_INV_QINV+320+32*\off+0)*4(%rsi), %ymm1
    vmovdqa (_ZETAS_INV+320+32*\off+0)*4(%rsi), %ymm2
    butterfly_inv 3,15
    shuffle4 3,15,4,5
    shuffle4 6,7,3,15
    vmovdqa (_ZETAS_INV_QINV+320+32*\off+8)*4(%rsi), %ymm1
    vmovdqa (_ZETAS_INV+320+32*\off+8)*4(%rsi), %ymm2
    butterfly_inv 3,15
    shuffle4 3,15,6,7
    shuffle4 8,9,3,15
    vmovdqa (_ZETAS_INV_QINV+320+32*\off+16)*4(%rsi), %ymm1
    vmovdqa (_ZETAS_INV+320+32*\off+16)*4(%rsi), %ymm2
    butterfly_inv 3,15
    shuffle4 3,15,8,9
    shuffle4 10,11,3,15
    vmovdqa (_ZETAS_INV_QINV+320+32*\off+24)*4(%rsi), %ymm1
    vmovdqa (_ZETAS_INV+320+32*\off+24)*4(%rsi), %ymm2
    butterfly_inv 3,15
    shuffle4 3,15,10,11

    /* t=4 (stride 4, intra) via shuffle8 ; vector base 64+32off+8g */
    shuffle8 4,5,3,15
    vmovdqa (_ZETAS_INV_QINV+64+32*\off+0)*4(%rsi), %ymm1
    vmovdqa (_ZETAS_INV+64+32*\off+0)*4(%rsi), %ymm2
    butterfly_inv 3,15
    shuffle8 3,15,4,5
    shuffle8 6,7,3,15
    vmovdqa (_ZETAS_INV_QINV+64+32*\off+8)*4(%rsi), %ymm1
    vmovdqa (_ZETAS_INV+64+32*\off+8)*4(%rsi), %ymm2
    butterfly_inv 3,15
    shuffle8 3,15,6,7
    shuffle8 8,9,3,15
    vmovdqa (_ZETAS_INV_QINV+64+32*\off+16)*4(%rsi), %ymm1
    vmovdqa (_ZETAS_INV+64+32*\off+16)*4(%rsi), %ymm2
    butterfly_inv 3,15
    shuffle8 3,15,8,9
    shuffle8 10,11,3,15
    vmovdqa (_ZETAS_INV_QINV+64+32*\off+24)*4(%rsi), %ymm1
    vmovdqa (_ZETAS_INV+64+32*\off+24)*4(%rsi), %ymm2
    butterfly_inv 3,15
    shuffle8 3,15,10,11

    /* t=8 (stride 8, cross) : (4,5),(6,7),(8,9),(10,11) -> Mn[32+4off+g] */
    vpbroadcastd (_ZETAS_INV_QINV+32+4*\off)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+32+4*\off)*4(%rsi), %ymm2
    butterfly_inv 4,5
    vpbroadcastd (_ZETAS_INV_QINV+33+4*\off)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+33+4*\off)*4(%rsi), %ymm2
    butterfly_inv 6,7
    vpbroadcastd (_ZETAS_INV_QINV+34+4*\off)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+34+4*\off)*4(%rsi), %ymm2
    butterfly_inv 8,9
    vpbroadcastd (_ZETAS_INV_QINV+35+4*\off)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+35+4*\off)*4(%rsi), %ymm2
    butterfly_inv 10,11

    /* t=16 (stride 16, cross) : (4,6),(5,7)->Mn[16+2off] ; (8,10),(9,11)->Mn[17+2off] */
    vpbroadcastd (_ZETAS_INV_QINV+16+2*\off)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+16+2*\off)*4(%rsi), %ymm2
    butterfly_inv 4,6
    butterfly_inv 5,7
    vpbroadcastd (_ZETAS_INV_QINV+17+2*\off)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+17+2*\off)*4(%rsi), %ymm2
    butterfly_inv 8,10
    butterfly_inv 9,11

    /* t=32 (stride 32, cross) : (4,8),(5,9),(6,10),(7,11) -> Mn[8+off] */
    vpbroadcastd (_ZETAS_INV_QINV+8+\off)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+8+\off)*4(%rsi), %ymm2
    butterfly_inv 4,8
    butterfly_inv 5,9
    butterfly_inv 6,10
    butterfly_inv 7,11

    vmovdqa %ymm4,  256*\off+  0(%rdi)
    vmovdqa %ymm5,  256*\off+ 32(%rdi)
    vmovdqa %ymm6,  256*\off+ 64(%rdi)
    vmovdqa %ymm7,  256*\off+ 96(%rdi)
    vmovdqa %ymm8,  256*\off+128(%rdi)
    vmovdqa %ymm9,  256*\off+160(%rdi)
    vmovdqa %ymm10, 256*\off+192(%rdi)
    vmovdqa %ymm11, 256*\off+224(%rdi)
.endm

/* Phase A: cross-register strides 64,128,256, then scale by N^{-1}.
   Ri lane j = a[i*64 + 8*off + j]. */
.macro inv_phaseA off
    vmovdqa    0+32*\off(%rdi), %ymm4
    vmovdqa  256+32*\off(%rdi), %ymm5
    vmovdqa  512+32*\off(%rdi), %ymm6
    vmovdqa  768+32*\off(%rdi), %ymm7
    vmovdqa 1024+32*\off(%rdi), %ymm8
    vmovdqa 1280+32*\off(%rdi), %ymm9
    vmovdqa 1536+32*\off(%rdi), %ymm10
    vmovdqa 1792+32*\off(%rdi), %ymm11

    /* t=64 (stride 64) : (4,5),(6,7),(8,9),(10,11) -> Mn[4..7] */
    vpbroadcastd (_ZETAS_INV_QINV+4)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+4)*4(%rsi), %ymm2
    butterfly_inv 4,5
    vpbroadcastd (_ZETAS_INV_QINV+5)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+5)*4(%rsi), %ymm2
    butterfly_inv 6,7
    vpbroadcastd (_ZETAS_INV_QINV+6)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+6)*4(%rsi), %ymm2
    butterfly_inv 8,9
    vpbroadcastd (_ZETAS_INV_QINV+7)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+7)*4(%rsi), %ymm2
    butterfly_inv 10,11

    /* t=128 (stride 128) : (4,6),(5,7)->Mn[2] ; (8,10),(9,11)->Mn[3] */
    vpbroadcastd (_ZETAS_INV_QINV+2)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+2)*4(%rsi), %ymm2
    butterfly_inv 4,6
    butterfly_inv 5,7
    vpbroadcastd (_ZETAS_INV_QINV+3)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+3)*4(%rsi), %ymm2
    butterfly_inv 8,10
    butterfly_inv 9,11

    /* t=256 (stride 256) : (4,8),(5,9),(6,10),(7,11) -> Mn[1] */
    vpbroadcastd (_ZETAS_INV_QINV+1)*4(%rsi), %ymm1
    vpbroadcastd (_ZETAS_INV+1)*4(%rsi), %ymm2
    butterfly_inv 4,8
    butterfly_inv 5,9
    butterfly_inv 6,10
    butterfly_inv 7,11

    /* scale all 8 registers by N^{-1} = _8XDIV (montgomery) */
    vmovdqa _8XDIV_QINV*4(%rsi), %ymm1
    vmovdqa _8XDIV*4(%rsi), %ymm2
    scale_one 4
    scale_one 5
    scale_one 6
    scale_one 7
    scale_one 8
    scale_one 9
    scale_one 10
    scale_one 11

    vmovdqa %ymm4,    0+32*\off(%rdi)
    vmovdqa %ymm5,  256+32*\off(%rdi)
    vmovdqa %ymm6,  512+32*\off(%rdi)
    vmovdqa %ymm7,  768+32*\off(%rdi)
    vmovdqa %ymm8, 1024+32*\off(%rdi)
    vmovdqa %ymm9, 1280+32*\off(%rdi)
    vmovdqa %ymm10,1536+32*\off(%rdi)
    vmovdqa %ymm11,1792+32*\off(%rdi)
.endm

/* montred(X * _8XDIV) keeping X in place ; ymm1=_8XDIV_QINV, ymm2=_8XDIV */
.macro scale_one X
    vpmuldq   %ymm1, %ymm\X, %ymm12
    vmovshdup %ymm\X, %ymm3
    vpmuldq   %ymm1, %ymm3, %ymm14
    vpmuldq   %ymm2, %ymm\X, %ymm\X
    vpmuldq   %ymm2, %ymm3, %ymm3
    vpmuldq   %ymm0, %ymm12, %ymm12
    vpmuldq   %ymm0, %ymm14, %ymm14
    vpsubd    %ymm12, %ymm\X, %ymm\X
    vpsubd    %ymm14, %ymm3, %ymm3
    vmovshdup %ymm\X, %ymm\X
    vpblendd  $0xAA, %ymm3, %ymm\X, %ymm\X
    /* canonicalize montred result (-q,q) -> [0,q) */
    vpsrad    $31, %ymm\X, %ymm3
    vpand     %ymm0, %ymm3, %ymm3
    vpaddd    %ymm3, %ymm\X, %ymm\X
.endm

.text
.global invntt_avx
invntt_avx:
    vmovdqa _8XQ*4(%rsi), %ymm0

    inv_phaseB 0
    inv_phaseB 1
    inv_phaseB 2
    inv_phaseB 3
    inv_phaseB 4
    inv_phaseB 5
    inv_phaseB 6
    inv_phaseB 7

    inv_phaseA 0
    inv_phaseA 1
    inv_phaseA 2
    inv_phaseA 3
    inv_phaseA 4
    inv_phaseA 5
    inv_phaseA 6
    inv_phaseA 7

    ret

.section .note.GNU-stack,"",@progbits
