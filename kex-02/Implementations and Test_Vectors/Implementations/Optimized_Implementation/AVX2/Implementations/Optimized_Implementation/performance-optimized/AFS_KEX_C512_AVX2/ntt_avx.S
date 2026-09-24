
.include "shuffle.inc"
#include "consts.h"

.macro butterfly l,r
//a*zeta*qinv
vpmullw %ymm1,%ymm\r,%ymm12
//a*zeta
vpmulhw %ymm2,%ymm\r,%ymm13
//a*zeta*qinv*q
vpmulhw %ymm0,%ymm12,%ymm12
vpsubw  %ymm12,%ymm13,%ymm12
vpsubw  %ymm12,%ymm\l,%ymm\r
vpaddw  %ymm12,%ymm\l,%ymm\l
.endm

.macro barret a
vpmulhw     %ymm14,%ymm\a,%ymm13
vpaddw      %ymm15,%ymm13,%ymm13
vpsraw      $10,%ymm13,%ymm13
vpmullw     %ymm0,%ymm13,%ymm13
vpsubw      %ymm13,%ymm\a,%ymm\a
.endm

.macro ntt_level0t1_avx off

vmovdqa	 (16*\off + 0)*2(%rdi),%ymm4
vmovdqa	 (16*\off + 64)*2(%rdi),%ymm5
vmovdqa	 (16*\off + 128)*2(%rdi),%ymm6
vmovdqa	 (16*\off + 192)*2(%rdi),%ymm7
vmovdqa	 (16*\off + 256)*2(%rdi),%ymm8
vmovdqa	 (16*\off + 320)*2(%rdi),%ymm9
vmovdqa	 (16*\off + 384)*2(%rdi),%ymm10
vmovdqa	 (16*\off + 448)*2(%rdi),%ymm11

//level0:256
vmovdqa	 (%r8),%ymm1
vmovdqa	 (%rdx),%ymm2
butterfly 4,8
butterfly 5,9
butterfly 6,10
butterfly 7,11


//level1:128
vmovdqa	  32(%r8),%ymm1
vmovdqa   32(%rdx),%ymm2
butterfly 4,6
butterfly 5,7
vmovdqa	  64(%r8),%ymm1
vmovdqa   64(%rdx),%ymm2
butterfly 8,10
butterfly 9,11

vmovdqa	  %ymm4,(16*\off + 0)*2(%rdi)
vmovdqa	  %ymm5,(16*\off + 64)*2(%rdi)
vmovdqa	  %ymm6,(16*\off + 128)*2(%rdi)
vmovdqa	  %ymm7,(16*\off + 192)*2(%rdi)
vmovdqa	  %ymm8,(16*\off + 256)*2(%rdi)
vmovdqa	  %ymm9,(16*\off + 320)*2(%rdi)
vmovdqa	  %ymm10,(16*\off + 384)*2(%rdi)
vmovdqa	  %ymm11,(16*\off + 448)*2(%rdi)

.endm

.macro ntt_level2t6_avx off

vmovdqa	  (128*\off + 0)*2(%rdi),%ymm4
vmovdqa	  (128*\off + 16)*2(%rdi),%ymm5
vmovdqa	  (128*\off + 32)*2(%rdi),%ymm6
vmovdqa	  (128*\off + 48)*2(%rdi),%ymm7
vmovdqa	  (128*\off + 64)*2(%rdi),%ymm8
vmovdqa	  (128*\off + 80)*2(%rdi),%ymm9
vmovdqa	  (128*\off + 96)*2(%rdi),%ymm10
vmovdqa	  (128*\off + 112)*2(%rdi),%ymm11

//level2:64
vmovdqa	  (16*\off + 0)*2(%r8),%ymm1
vmovdqa   (16*\off + 0)*2(%rdx),%ymm2
butterfly 4,8
butterfly 5,9
butterfly 6,10
butterfly 7,11
/*
vmovdqa	  %ymm4,(128*\off + 0)*2(%rdi)
vmovdqa	  %ymm5,(128*\off + 16)*2(%rdi)
vmovdqa	  %ymm6,(128*\off + 32)*2(%rdi)
vmovdqa	  %ymm7,(128*\off + 48)*2(%rdi)
vmovdqa	  %ymm8,(128*\off + 64)*2(%rdi)
vmovdqa	  %ymm9,(128*\off + 80)*2(%rdi)
vmovdqa	  %ymm10,(128*\off + 96)*2(%rdi)
vmovdqa	  %ymm11,(128*\off + 112)*2(%rdi)
*/
//level3:32
shuffle8	4,8,3,8
shuffle8	5,9,4,9
shuffle8	6,10,5,10
shuffle8	7,11,6,11
vmovdqa     (16*\off + 64)*2(%r8),%ymm1
vmovdqa     (16*\off + 64)*2(%rdx),%ymm2
butterfly	3,5
butterfly	8,10
butterfly	4,6
butterfly	9,11
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
barret	3
barret	4
barret	5
barret	6
barret	8
barret	9
barret	10
barret	11
/*
shuffle8	6,11,7,11
shuffle8	5,10,6,10
shuffle8	4,9,5,9
shuffle8	3,8,4,8
vmovdqa	  %ymm4,(128*\off + 0)*2(%rdi)
vmovdqa	  %ymm5,(128*\off + 16)*2(%rdi)
vmovdqa	  %ymm6,(128*\off + 32)*2(%rdi)
vmovdqa	  %ymm7,(128*\off + 48)*2(%rdi)
vmovdqa	  %ymm8,(128*\off + 64)*2(%rdi)
vmovdqa	  %ymm9,(128*\off + 80)*2(%rdi)
vmovdqa	  %ymm10,(128*\off + 96)*2(%rdi)
vmovdqa	  %ymm11,(128*\off + 112)*2(%rdi)
*/

//level4:16
shuffle4	3,5,7,5
shuffle4	8,10,3,10
shuffle4	4,6,8,6
shuffle4	9,11,4,11
vmovdqa     (16*\off + 128)*2(%r8),%ymm1
vmovdqa     (16*\off + 128)*2(%rdx),%ymm2
butterfly	7,8
butterfly	5,6
butterfly	3,4
butterfly	10,11
vmovdqa     _16XV*2(%rsi),%ymm14
vmovdqa     _16XBARRETCONST*2(%rsi),%ymm15
barret	    3
barret	    4
barret	    5
barret	    6
barret      7
barret      8
barret	    10
barret	    11
/*
shuffle4	4,11,9,11
shuffle4	8,6,4,6
shuffle4	3,10,8,10
shuffle4	7,5,3,5
shuffle8	6,11,7,11
shuffle8	5,10,6,10
shuffle8	4,9,5,9
shuffle8	3,8,4,8
vmovdqa	  %ymm4,(128*\off + 0)*2(%rdi)
vmovdqa	  %ymm5,(128*\off + 16)*2(%rdi)
vmovdqa	  %ymm6,(128*\off + 32)*2(%rdi)
vmovdqa	  %ymm7,(128*\off + 48)*2(%rdi)
vmovdqa	  %ymm8,(128*\off + 64)*2(%rdi)
vmovdqa	  %ymm9,(128*\off + 80)*2(%rdi)
vmovdqa	  %ymm10,(128*\off + 96)*2(%rdi)
vmovdqa	  %ymm11,(128*\off + 112)*2(%rdi)
*/

//level5:8
shuffle2    7,8
shuffle2    5,6
shuffle2    3,4
shuffle2    10,11
vmovdqa     (16*\off + 192)*2(%r8),%ymm1
vmovdqa     (16*\off + 192)*2(%rdx),%ymm2
butterfly	7,3
butterfly	8,4
butterfly	5,10
butterfly	6,11
/*
shuffle2    7,8
shuffle2    5,6
shuffle2    3,4
shuffle2    10,11
shuffle4	4,11,9,11
shuffle4	8,6,4,6
shuffle4	3,10,8,10
shuffle4	7,5,3,5
shuffle8	6,11,7,11
shuffle8	5,10,6,10
shuffle8	4,9,5,9
shuffle8	3,8,4,8
vmovdqa	  %ymm4,(128*\off + 0)*2(%rdi)
vmovdqa	  %ymm5,(128*\off + 16)*2(%rdi)
vmovdqa	  %ymm6,(128*\off + 32)*2(%rdi)
vmovdqa	  %ymm7,(128*\off + 48)*2(%rdi)
vmovdqa	  %ymm8,(128*\off + 64)*2(%rdi)
vmovdqa	  %ymm9,(128*\off + 80)*2(%rdi)
vmovdqa	  %ymm10,(128*\off + 96)*2(%rdi)
vmovdqa	  %ymm11,(128*\off + 112)*2(%rdi)
*/

//level6:4
shuffle1   7,3
shuffle1   8,4
shuffle1   5,10
shuffle1   6,11
vmovdqa    (16*\off + 256)*2(%r8),%ymm1
vmovdqa    (16*\off + 256)*2(%rdx),%ymm2
butterfly  7,5
butterfly  3,10
butterfly  8,6
butterfly  4,11

//////////////////////////////////////////C实现顺序开始//////////////////////////////////////////

shuffle1   7,3
shuffle1   8,4
shuffle1   5,10
shuffle1   6,11

shuffle2    7,8
shuffle2    5,6
shuffle2    3,4
shuffle2    10,11

shuffle4	4,11,9,11
shuffle4	8,6,4,6
shuffle4	3,10,8,10
shuffle4	7,5,3,5


shuffle8	6,11,7,11
shuffle8	5,10,6,10
shuffle8	4,9,5,9
shuffle8	3,8,4,8

vmovdqa		%ymm4,(128*\off + 0)*2(%rdi)
vmovdqa		%ymm5,(128*\off + 16)*2(%rdi)
vmovdqa		%ymm6,(128*\off + 32)*2(%rdi)
vmovdqa		%ymm7,(128*\off + 48)*2(%rdi)
vmovdqa		%ymm8,(128*\off + 64)*2(%rdi)
vmovdqa		%ymm9,(128*\off + 80)*2(%rdi)
vmovdqa		%ymm10,(128*\off + 96)*2(%rdi)
vmovdqa		%ymm11,(128*\off + 112)*2(%rdi)

//////////////////////////////////////////C实现顺序结束//////////////////////////////////////////

/*
vmovdqa		%ymm7,(128*\off + 0)*2(%rdi)
vmovdqa		%ymm3,(128*\off + 16)*2(%rdi)
vmovdqa		%ymm8,(128*\off + 32)*2(%rdi)
vmovdqa		%ymm4,(128*\off + 48)*2(%rdi)
vmovdqa		%ymm5,(128*\off + 64)*2(%rdi)
vmovdqa		%ymm10,(128*\off + 80)*2(%rdi)
vmovdqa		%ymm6,(128*\off + 96)*2(%rdi)
vmovdqa		%ymm11,(128*\off + 112)*2(%rdi)
*/


.endm

.text
.global cdecl(ntt_avx)
cdecl(ntt_avx):
//q
vmovdqa		_16XQ*2(%rsi),%ymm0
#levels0t1
//zeta
lea		(_ZETAS_NTT+16)*2(%rsi),%rdx
//zetaxqinv
lea		(_ZETASxQINV_NTT+16)*2(%rsi),%r8
//levels0t1
ntt_level0t1_avx 0
ntt_level0t1_avx 1
ntt_level0t1_avx 2
ntt_level0t1_avx 3
//levels2t6
add		$96,%r8
add		$96,%rdx

ntt_level2t6_avx 0
ntt_level2t6_avx 1
ntt_level2t6_avx 2
ntt_level2t6_avx 3

ret
