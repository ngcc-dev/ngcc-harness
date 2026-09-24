#define _16xQ 0
#define _16x2Qinv 4
#define _ZETAS_EXP 16
#define _ZETAS_OFFSET 576


.include "shuffle.inc"
.include "arith.inc"

.macro update rln,rl0,rl1,rl2,rl3,rh0,rh1,rh2,rh3
vpaddw		%ymm\rh0,%ymm\rl0,%ymm\rln
vpsubw		%ymm\rh0,%ymm\rl0,%ymm\rh0

vpaddw		%ymm\rh1,%ymm\rl1,%ymm\rl0
vpsubw		%ymm\rh1,%ymm\rl1,%ymm\rh1

vpaddw		%ymm\rh2,%ymm\rl2,%ymm\rl1
vpsubw		%ymm\rh2,%ymm\rl2,%ymm\rh2

vpaddw		%ymm\rh3,%ymm\rl3,%ymm\rl2
vpsubw		%ymm\rh3,%ymm\rl3,%ymm\rh3



caddq       \rh0,\rh0,12
caddq       \rh1,\rh1,12
caddq       \rh2,\rh2,12
caddq       \rh3,\rh3,12

csubq       \rl2, \rl3, 12
csubq       \rl1, \rl2, 12
csubq       \rl0, \rl1, 12
csubq       \rln, \rl0, 12

.endm

.macro ct_forward_level_0 off

vmovdqa	    (64*\off + 0)*2(%rdi),%ymm4
vmovdqa		(64*\off + 16)*2(%rdi),%ymm5
vmovdqa		(64*\off + 32)*2(%rdi),%ymm6
vmovdqa		(64*\off + 48)*2(%rdi),%ymm7

;#level 0

vmovdqa	    (64*\off + 128)*2(%rdi),%ymm8
vmovdqa		(64*\off + 144)*2(%rdi),%ymm9
vmovdqa		(64*\off + 160)*2(%rdi),%ymm10
vmovdqa		(64*\off + 176)*2(%rdi),%ymm11

mul		8,9
mul     10,11

update		3,4,5,6,7,8,9,10,11

vmovdqa	    %ymm4,(64*\off + 0)*2(%rdi)
vmovdqa		%ymm5,(64*\off + 16)*2(%rdi)
vmovdqa		%ymm6,(64*\off + 32)*2(%rdi)
vmovdqa		%ymm7,(64*\off + 48)*2(%rdi)
vmovdqa		%ymm8,(64*\off + 128)*2(%rdi)
vmovdqa		%ymm9,(64*\off + 144)*2(%rdi)
vmovdqa		%ymm10,(64*\off + 160)*2(%rdi)
vmovdqa		%ymm11,(64*\off + 176)*2(%rdi)

.endm

.macro ct_forward_level1to7 off

vmovdqa	    (128*\off + 0)*2(%rdi),%ymm4
vmovdqa		(128*\off + 16)*2(%rdi),%ymm5
vmovdqa		(128*\off + 32)*2(%rdi),%ymm6
vmovdqa		(128*\off + 48)*2(%rdi),%ymm7

;#level 0

vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+0)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+4)*2(%rsi),%ymm2

vmovdqa	    (128*\off + 64)*2(%rdi),%ymm8
vmovdqa		(128*\off + 80)*2(%rdi),%ymm9
vmovdqa		(128*\off + 96)*2(%rdi),%ymm10
vmovdqa		(128*\off + 112)*2(%rdi),%ymm11

mul		8,9
mul     10,11

update		3,4,5,6,7,8,9,10,11


;#level 1 
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+8)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+12)*2(%rsi),%ymm2

mul     6,7

vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+16)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+20)*2(%rsi),%ymm2

mul     10,11

update       3,4,5,8,9,6,7,10,11


;#level 2  

vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+24)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+28)*2(%rsi),%ymm2
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+32)*2(%rsi),%ymm14
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+36)*2(%rsi),%ymm3

mul        5, 7, 15, 14, 2, 3

vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+40)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+44)*2(%rsi),%ymm2
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+48)*2(%rsi),%ymm14
vpbroadcastq	(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+52)*2(%rsi),%ymm3

mul        9, 11, 15, 14, 2, 3

update      3, 4, 6, 8, 10, 5, 7, 9, 11

;#level 3 

shuffle8	4,5,3,5
shuffle8	6,7,4,7
shuffle8	8,9,6,9
shuffle8	10,11,8,11

vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+56)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+56+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+56+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+56+48)*2(%rsi),%ymm10

mul		5, 7, 15, 14, 2, 10

vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+120)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+120+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+120+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+120+48)*2(%rsi),%ymm10

mul     9, 11, 15, 14, 2, 10

update       10,3,4,6,8,5,7,9,11

;#level 4 

shuffle4	3,5,10,5
shuffle4	4,7,3,7
shuffle4	6,9,4,9
shuffle4	8,11,6,11

vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+184)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+184+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+184+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+184+48)*2(%rsi),%ymm8

mul        5, 7, 15, 14, 2, 8

vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+248)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+248+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+248+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+248+48)*2(%rsi),%ymm8

mul        9, 11, 15, 14, 2, 8

update      8,10,3,4,6,5,7,9,11

;#level 5 

shuffle2	10,5,8,5
shuffle2	3,7,10,7
shuffle2	4,9,3,9
shuffle2	6,11,4,11

vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+312)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+312+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+312+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+312+48)*2(%rsi),%ymm6

mul        5, 7, 15, 14, 2, 6

vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+376)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+376+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+376+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+376+48)*2(%rsi),%ymm6

mul        9, 11, 15, 14, 2, 6

update      6,8,10,3,4,5,7,9,11

;#level 6 

shuffle1	8,5,6,5
shuffle1	10,7,8,7
shuffle1	3,9,10,9
shuffle1	4,11,3,11

vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+440)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+440+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+440+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+440+48)*2(%rsi),%ymm4

mul        5, 7, 15, 14, 2, 4

vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+504)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+504+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+504+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_OFFSET*\off + 8 + _ZETAS_EXP+504+48)*2(%rsi),%ymm4

mul        9, 11, 15, 14, 2, 4

update      4,6,8,10,3,5,7,9,11

;#Store 
shuffle1	6,5, 4,5
shuffle1	8,7, 6,7
shuffle1	10,9, 8,9
shuffle1	3,11, 10,11

shuffle2	4,5, 3,5
shuffle2	6,7, 4,7,
shuffle2    8,9, 6,9,
shuffle2    10,11, 8,11

shuffle4       3,5, 10,5
shuffle4       4,7, 3,7
shuffle4       6,9, 4,9
shuffle4       8,11, 6,11

shuffle8       10,5, 8,5
shuffle8       3,7, 10,7
shuffle8       4,9, 3,9
shuffle8       6,11, 4,11

;#Store 

vmovdqa		%ymm8,(128*\off + 0)*2(%rdi)
vmovdqa		%ymm5,(128*\off + 16)*2(%rdi)
vmovdqa		%ymm10,(128*\off + 32)*2(%rdi)
vmovdqa		%ymm7,(128*\off + 48)*2(%rdi)
vmovdqa		%ymm3,(128*\off + 64)*2(%rdi)
vmovdqa		%ymm9,(128*\off + 80)*2(%rdi) 
vmovdqa		%ymm4,(128*\off + 96)*2(%rdi)
vmovdqa		%ymm11,(128*\off + 112)*2(%rdi)

.endm


.text
.global ntt_avx
ntt_avx:

vpbroadcastq	(_16xQ)*2(%rsi),%ymm0

vpbroadcastq	(_ZETAS_EXP+0)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_EXP+4)*2(%rsi),%ymm2

ct_forward_level_0 0
ct_forward_level_0 1

ct_forward_level1to7 0
ct_forward_level1to7 1

ret