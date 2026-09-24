
.include "shuffle.inc"
#include "consts.h"

.macro butterfly l,r
vpsllw $0, %ymm\l, %ymm12
vpaddw  %ymm12,%ymm\r,%ymm\l

vpmulhw     %ymm14,%ymm\l,%ymm13
vpaddw      %ymm15,%ymm13,%ymm13
vpsraw      $10,%ymm13,%ymm13
vpmullw     %ymm0,%ymm13,%ymm13
vpsubw      %ymm13,%ymm\l,%ymm\l

vpsubw  %ymm12,%ymm\r,%ymm\r
//a*zeta*qinv
vpmullw %ymm1,%ymm\r,%ymm12
//a*zeta
vpmulhw %ymm2,%ymm\r,%ymm13
//a*zeta*qinv*q
vpmulhw %ymm0,%ymm12,%ymm12
vpsubw  %ymm12,%ymm13,%ymm\r
.endm

.macro fqmul a
//a*phi*qinv
vpmullw %ymm13,%ymm\a,%ymm14
//a*phi
vpmulhw %ymm12,%ymm\a,%ymm15
//a*phi*qinv*q
vpmulhw %ymm0,%ymm14,%ymm14
vpsubw  %ymm14,%ymm15,%ymm\a
.endm

.macro invntt_level6t2_avx off

//////////////////////////////////////////C实现转换AVX实现顺序//////////////////////////////////////////
vmovdqa	  (128*\off + 0)*2(%rdi),%ymm4
vmovdqa	  (128*\off + 16)*2(%rdi),%ymm5
vmovdqa	  (128*\off + 32)*2(%rdi),%ymm6
vmovdqa	  (128*\off + 48)*2(%rdi),%ymm7
vmovdqa	  (128*\off + 64)*2(%rdi),%ymm8
vmovdqa	  (128*\off + 80)*2(%rdi),%ymm9
vmovdqa	  (128*\off + 96)*2(%rdi),%ymm10
vmovdqa	  (128*\off + 112)*2(%rdi),%ymm11

shuffle8	4,8,3,8
shuffle8	5,9,4,9
shuffle8	6,10,5,10
shuffle8	7,11,6,11

shuffle4	3,5,7,5
shuffle4	8,10,3,10
shuffle4	4,6,8,6
shuffle4	9,11,4,11

shuffle2    7,8
shuffle2    5,6
shuffle2    3,4
shuffle2    10,11

shuffle1   7,3
shuffle1   8,4
shuffle1   5,10
shuffle1   6,11
//////////////////////////////////////////C实现转换AVX实现顺序开始//////////////////////////////////////////
/*
vmovdqa	  (128*\off + 0)*2(%rdi),%ymm7
vmovdqa	  (128*\off + 16)*2(%rdi),%ymm3
vmovdqa	  (128*\off + 32)*2(%rdi),%ymm8
vmovdqa	  (128*\off + 48)*2(%rdi),%ymm4
vmovdqa	  (128*\off + 64)*2(%rdi),%ymm5
vmovdqa	  (128*\off + 80)*2(%rdi),%ymm10
vmovdqa	  (128*\off + 96)*2(%rdi),%ymm6
vmovdqa	  (128*\off + 112)*2(%rdi),%ymm11
*/
//level6:4
vmovdqa    (16*\off + 0)*2(%r8),%ymm1
vmovdqa    (16*\off + 0)*2(%rdx),%ymm2
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
butterfly  7,5
butterfly  3,10
butterfly  8,6
butterfly  4,11
shuffle1   7,3
shuffle1   8,4
shuffle1   5,10
shuffle1   6,11


//level5:8
vmovdqa     (16*\off + 64)*2(%r8),%ymm1
vmovdqa     (16*\off + 64)*2(%rdx),%ymm2
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
butterfly	7,3
butterfly	8,4
butterfly	5,10
butterfly	6,11

shuffle2    7,8
shuffle2    5,6
shuffle2    3,4
shuffle2    10,11

//level4:16
vmovdqa     (16*\off + 128)*2(%r8),%ymm1
vmovdqa     (16*\off + 128)*2(%rdx),%ymm2
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
butterfly	7,8
butterfly	5,6
butterfly	3,4
butterfly	10,11

shuffle4	4,11,9,11
shuffle4	8,6,4,6
shuffle4	3,10,8,10
shuffle4	7,5,3,5

//level3:32
vmovdqa	  (16*\off + 192)*2(%r8),%ymm1
vmovdqa   (16*\off + 192)*2(%rdx),%ymm2
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
butterfly	3,5
butterfly	8,10
butterfly	4,6
butterfly	9,11

shuffle8	6,11,7,11
shuffle8	5,10,6,10
shuffle8	4,9,5,9
shuffle8	3,8,4,8

//level2:64
vmovdqa	  (16*\off + 256)*2(%r8),%ymm1
vmovdqa   (16*\off + 256)*2(%rdx),%ymm2
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
butterfly 4,8
butterfly 5,9
butterfly 6,10
butterfly 7,11

vmovdqa	  %ymm4,(128*\off + 0)*2(%rdi)
vmovdqa	  %ymm5,(128*\off + 16)*2(%rdi)
vmovdqa	  %ymm6,(128*\off + 32)*2(%rdi)
vmovdqa	  %ymm7,(128*\off + 48)*2(%rdi)
vmovdqa	  %ymm8,(128*\off + 64)*2(%rdi)
vmovdqa	  %ymm9,(128*\off + 80)*2(%rdi)
vmovdqa	  %ymm10,(128*\off + 96)*2(%rdi)
vmovdqa	  %ymm11,(128*\off + 112)*2(%rdi)

.endm

.macro invntt_level1t0_avx off

vmovdqa	 (16*\off + 0)*2(%rdi),%ymm4
vmovdqa	 (16*\off + 64)*2(%rdi),%ymm5
vmovdqa	 (16*\off + 128)*2(%rdi),%ymm6
vmovdqa	 (16*\off + 192)*2(%rdi),%ymm7
vmovdqa	 (16*\off + 256)*2(%rdi),%ymm8
vmovdqa	 (16*\off + 320)*2(%rdi),%ymm9
vmovdqa	 (16*\off + 384)*2(%rdi),%ymm10
vmovdqa	 (16*\off + 448)*2(%rdi),%ymm11

//level1:128
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
vmovdqa	  (%r8),%ymm1
vmovdqa   (%rdx),%ymm2
butterfly 4,6
butterfly 5,7

vmovdqa	  32(%r8),%ymm1
vmovdqa   32(%rdx),%ymm2
butterfly 8,10
butterfly 9,11

//level0:256
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
vmovdqa	 64(%r8),%ymm1
vmovdqa	 64(%rdx),%ymm2
butterfly 4,8
butterfly 5,9
butterfly 6,10
butterfly 7,11

//phi
vmovdqa     _16XFHI*2(%rsi),%ymm12
//phixqinv
vmovdqa     _16XFLO*2(%rsi),%ymm13
fqmul 4
fqmul 5
fqmul 6
fqmul 7
fqmul 8
fqmul 9
fqmul 10
fqmul 11

vmovdqa	  %ymm4,(16*\off + 0)*2(%rdi)
vmovdqa	  %ymm5,(16*\off + 64)*2(%rdi)
vmovdqa	  %ymm6,(16*\off + 128)*2(%rdi)
vmovdqa	  %ymm7,(16*\off + 192)*2(%rdi)
vmovdqa	  %ymm8,(16*\off + 256)*2(%rdi)
vmovdqa	  %ymm9,(16*\off + 320)*2(%rdi)
vmovdqa	  %ymm10,(16*\off + 384)*2(%rdi)
vmovdqa	  %ymm11,(16*\off + 448)*2(%rdi)

.endm



.text
.global cdecl(invntt_avx)
cdecl(invntt_avx):
//q
vmovdqa		_16XQ*2(%rsi),%ymm0
//zeta
lea		(_ZETAS_INVNTT)*2(%rsi),%rdx
//zetaxqinv
lea		(_ZETASxQINV_INVNTT)*2(%rsi),%r8
//level6t2
invntt_level6t2_avx 0
invntt_level6t2_avx 1
invntt_level6t2_avx 2
invntt_level6t2_avx 3

add		$640,%r8
add		$640,%rdx

//level1t0
invntt_level1t0_avx 0
invntt_level1t0_avx 1
invntt_level1t0_avx 2
invntt_level1t0_avx 3

ret