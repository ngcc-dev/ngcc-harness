#define _16xQ 0
#define _16x2Qinv 4
#define _ZETAS_EXP 16
#define _REV_B 1168
#define _REV_W 1184
#define _INV_N 1200
#define _ZETAS_OFFSET 576

.include "shuffle.inc"
.include "arith.inc"

.macro butterfly rl0,rl1,rh0,rh1,zl0=15,zl1=15,zh0=2,zh1=2
vpsubw		%ymm\rl0,%ymm\rh0,%ymm12
vpaddw		%ymm\rh0,%ymm\rl0,%ymm\rl0
vpsubw		%ymm\rl1,%ymm\rh1,%ymm13
vpaddw		%ymm\rh1,%ymm\rl1,%ymm\rl1

csubq       \rl0, \rl0, \rh0
csubq       \rl1, \rl1, \rh1
caddq       12, 12,\rh0
caddq       13, 13,\rh1

vpmullw		%ymm\zl0,%ymm12,%ymm\rh0
vpmullw		%ymm\zl1,%ymm13,%ymm\rh1

vpmulhw		%ymm\zh0,%ymm12,%ymm12
vpmulhw		%ymm\zh1,%ymm13,%ymm13

vpmulhw		%ymm0,%ymm\rh0,%ymm\rh0
vpmulhw		%ymm0,%ymm\rh1,%ymm\rh1

vpsubw		%ymm\rh0,%ymm12,%ymm\rh0
vpsubw		%ymm\rh1,%ymm13,%ymm\rh1

caddq       \rh0, \rh0,12
caddq       \rh1, \rh1,13

.endm

.macro gs_reverse_level0to6 off

vmovdqa	    (128*\off + 0)*2(%rdi),%ymm3
vmovdqa		(128*\off + 16)*2(%rdi),%ymm5
vmovdqa		(128*\off + 32)*2(%rdi),%ymm4
vmovdqa		(128*\off + 48)*2(%rdi),%ymm7

vmovdqa	    (128*\off + 64)*2(%rdi),%ymm6
vmovdqa		(128*\off + 80)*2(%rdi),%ymm9
vmovdqa		(128*\off + 96)*2(%rdi),%ymm8
vmovdqa		(128*\off + 112)*2(%rdi),%ymm11

shuffle8    3, 5, 10, 5
shuffle8    4, 7, 3, 7
shuffle8    6, 9, 4, 9
shuffle8    8, 11, 6, 11

shuffle4    10,5,8,5
shuffle4    3,7,10,7
shuffle4    4,9,3,9
shuffle4    6,11,4,11

shuffle2    8,5,6,5
shuffle2    10,7,8,7
shuffle2    3,9,10,9
shuffle2    4,11,3,11

shuffle1	6,5,4,5
shuffle1	8,7,6,7
shuffle1	10,9,8,9
shuffle1	3,11,10,11

; # level 6 

; # reversing the halfword order of packed twiddle factors 

vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+504)*2(%rsi),%ymm14
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+504+16)*2(%rsi),%ymm3
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+504+32)*2(%rsi),%ymm15
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+504+48)*2(%rsi),%ymm2

vmovdqu		(_REV_B)*2(%rsi),%ymm1

vpshufb     %ymm1,%ymm14,%ymm14
vpshufb     %ymm1,%ymm3,%ymm3
vpshufb     %ymm1,%ymm15,%ymm15
vpshufb     %ymm1,%ymm2,%ymm2
; # end reverse 

butterfly	4,6,5,7,15,14,2,3

; # reversing the halfword order of packed twiddle factors
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+440)*2(%rsi),%ymm14
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+440+16)*2(%rsi),%ymm3
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+440+32)*2(%rsi),%ymm15
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+440+48)*2(%rsi),%ymm2

vpshufb     %ymm1,%ymm14,%ymm14
vpshufb     %ymm1,%ymm3,%ymm3
vpshufb     %ymm1,%ymm15,%ymm15
vpshufb     %ymm1,%ymm2,%ymm2
; # end reverse 

butterfly	8,10,9,11,15,14,2,3


shuffle1	4,5,2,5
shuffle1	6,7,4,7
shuffle1	8,9,6,9
shuffle1	10,11,8,11

; # level 5

; # reversing the word order of packed twiddle factors 
vmovdqu		(_REV_W)*2(%rsi),%ymm1

vpermd		(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+376)*2(%rsi),%ymm1,%ymm14
vpermd		(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+376+16)*2(%rsi),%ymm1,%ymm3
vpermd		(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+376+32)*2(%rsi),%ymm1,%ymm15
vpermd		(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+376+48)*2(%rsi),%ymm1,%ymm10
; # end reverse 

butterfly	2,4,5,7,15,14,10,3

; # reversing the word order of packed twiddle factors 
vpermd		(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+312)*2(%rsi),%ymm1,%ymm14
vpermd		(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+312+16)*2(%rsi),%ymm1,%ymm3
vpermd		(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+312+32)*2(%rsi),%ymm1,%ymm15
vpermd		(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+312+48)*2(%rsi),%ymm1,%ymm10
; # end reverse 

butterfly	6,8,9,11,15,14,10,3


shuffle2	2,5,10,5
shuffle2	4,7,2,7
shuffle2	6,9,4,9
shuffle2	8,11,6,11



; # level 4 

; # reversing the doubleword order of packed twiddle factors 
vpermq		$0x1B,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+248)*2(%rsi),%ymm14
vpermq		$0x1B,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+248+16)*2(%rsi),%ymm3
vpermq		$0x1B,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+248+32)*2(%rsi),%ymm15
vpermq		$0x1B,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+248+48)*2(%rsi),%ymm8
; # end reverse 

butterfly	10,2,5,7,15,14,8,3

; # reversing the doubleword order of packed twiddle factors 
vpermq		$0x1B,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+184)*2(%rsi),%ymm14
vpermq		$0x1B,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+184+16)*2(%rsi),%ymm3
vpermq		$0x1B,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+184+32)*2(%rsi),%ymm15
vpermq		$0x1B,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+184+48)*2(%rsi),%ymm8
; # end reverse 

butterfly	4,6,9,11,15,14,8,3

shuffle4	10,5,8,5
shuffle4	2,7,10,7
shuffle4	4,9,2,9
shuffle4	6,11,4,11

; # level 3 

; # reversing the quadword order of packed twiddle factors 
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+120)*2(%rsi),%ymm14
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+120+16)*2(%rsi),%ymm3
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+120+32)*2(%rsi),%ymm15
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+120+48)*2(%rsi),%ymm6
; # end reverse 

butterfly	8,10,5,7,15,14,6,3

; # reversing the quadword order of packed twiddle factors 
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+56)*2(%rsi),%ymm14
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+56+16)*2(%rsi),%ymm3
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+56+32)*2(%rsi),%ymm15
vpermq		$0x4E,(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+56+48)*2(%rsi),%ymm6
; # end reverse 

butterfly	2,4,9,11,15,14,6,3

shuffle8	8,5,6,5
shuffle8	10,7,8,7
shuffle8	2,9,10,9
shuffle8	4,11,2,11

; # level 2 

vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+40)*2(%rsi),%ymm14
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+44)*2(%rsi),%ymm3
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+48)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+52)*2(%rsi),%ymm4

butterfly	6,8,5,7,15,14,4,3

vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+24)*2(%rsi),%ymm14
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+28)*2(%rsi),%ymm3
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+32)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+36)*2(%rsi),%ymm4

butterfly	10,2,9,11,15,14,4,3

; # level 1 

vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+8)*2(%rsi),%ymm14
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+12)*2(%rsi),%ymm3
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+16)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+20)*2(%rsi),%ymm4

butterfly	6,5,8,7,15,15,4,4
butterfly	10,9,2,11,14,14,3,3

; # level 0 

vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+0)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*(1-\off) + 8 + _ZETAS_EXP+4)*2(%rsi),%ymm4

butterfly	6,5,10,9,15,15,4,4
butterfly	8,7,2,11,15,15,4,4


; # multiplication by inverse of N 
vpbroadcastq	(_INV_N)*2(%rsi),%ymm15
vpbroadcastq	(_INV_N+4)*2(%rsi),%ymm4

mul        6,5,15,15,4,4
mul        8,7,15,15,4,4
mul        10,9,15,15,4,4
mul        2,11,15,15,4,4

vmovdqa	    %ymm6, (128*\off + 0)*2(%rdi)
vmovdqa		%ymm5, (128*\off + 16)*2(%rdi)
vmovdqa		%ymm8, (128*\off + 32)*2(%rdi)
vmovdqa		%ymm7, (128*\off + 48)*2(%rdi)
vmovdqa	    %ymm10, (128*\off + 64)*2(%rdi)
vmovdqa		%ymm9, (128*\off + 80)*2(%rdi)
vmovdqa		%ymm2, (128*\off + 96)*2(%rdi)
vmovdqa		%ymm11, (128*\off + 112)*2(%rdi)

.endm

.macro gs_reverse_level7 off

vmovdqa	    (64*\off + 0)*2(%rdi),%ymm4
vmovdqa		(64*\off + 16)*2(%rdi),%ymm5
vmovdqa		(64*\off + 32)*2(%rdi),%ymm6
vmovdqa		(64*\off + 48)*2(%rdi),%ymm7

vmovdqa	    (64*\off + 128)*2(%rdi),%ymm8
vmovdqa		(64*\off + 144)*2(%rdi),%ymm9
vmovdqa		(64*\off + 160)*2(%rdi),%ymm10
vmovdqa		(64*\off + 176)*2(%rdi),%ymm11

butterfly	4,5,8,9,15,15,2,2
butterfly	6,7,10,11,15,15,2,2

/*
mul        4,5,14,14,3,3
mul        6,7,14,14,3,3
mul        8,9,14,14,3,3
mul        10,11,14,14,3,3
*/
vmovdqa	    %ymm4, (64*\off + 0)*2(%rdi)
vmovdqa		%ymm5, (64*\off + 16)*2(%rdi)
vmovdqa		%ymm6, (64*\off + 32)*2(%rdi)
vmovdqa		%ymm7, (64*\off + 48)*2(%rdi)
vmovdqa	    %ymm8, (64*\off + 128)*2(%rdi)
vmovdqa		%ymm9, (64*\off + 144)*2(%rdi)
vmovdqa		%ymm10, (64*\off + 160)*2(%rdi)
vmovdqa		%ymm11, (64*\off + 176)*2(%rdi)

.endm

.text
.global invntt_avx
invntt_avx:
vpbroadcastq    _16xQ*2(%rsi),%ymm0

gs_reverse_level0to6 0
gs_reverse_level0to6 1

vpbroadcastq	(_ZETAS_EXP+0)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_EXP+4)*2(%rsi),%ymm2

vpbroadcastq	(_INV_N)*2(%rsi),%ymm14
vpbroadcastq	(_INV_N+4)*2(%rsi),%ymm3

gs_reverse_level7 0
gs_reverse_level7 1


ret

