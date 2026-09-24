.include "arith.inc"

#define _16xQ 0
#define _16x2Qinv 4
#define _BARRET_CONST 616
#define _MONT_CONST 620

.macro polyreduce off

vmovdqa	    (\off*64)*2(%rdi),%ymm4
vmovdqa		(\off*64+16)*2(%rdi),%ymm5
vmovdqa		(\off*64+32)*2(%rdi),%ymm6
vmovdqa		(\off*64+48)*2(%rdi),%ymm7

barret  4
barret  5
barret  6
barret  7

vmovdqa     %ymm4, (\off*64)*2(%rdi)
vmovdqa     %ymm5, (\off*64+16)*2(%rdi)
vmovdqa     %ymm6, (\off*64+32)*2(%rdi)
vmovdqa     %ymm7, (\off*64+48)*2(%rdi)

.endm

.macro polysub off

vmovdqa	    (\off*64)*2(%rsi),%ymm4
vmovdqa		(\off*64+16)*2(%rsi),%ymm5
vmovdqa		(\off*64+32)*2(%rsi),%ymm6
vmovdqa		(\off*64+48)*2(%rsi),%ymm7

vmovdqa	    (\off*64)*2(%rdx),%ymm8
vmovdqa		(\off*64+16)*2(%rdx),%ymm9
vmovdqa		(\off*64+32)*2(%rdx),%ymm10
vmovdqa		(\off*64+48)*2(%rdx),%ymm11

vpsubw		%ymm8,%ymm4,%ymm4
vpsubw		%ymm9,%ymm5,%ymm5
vpsubw		%ymm10,%ymm6,%ymm6
vpsubw		%ymm11,%ymm7,%ymm7

caddq 4,4
caddq 5,5
caddq 6,6
caddq 7,7

vmovdqa     %ymm4, (\off*64)*2(%rdi)
vmovdqa     %ymm5, (\off*64+16)*2(%rdi)
vmovdqa     %ymm6, (\off*64+32)*2(%rdi)
vmovdqa     %ymm7, (\off*64+48)*2(%rdi)

.endm


.macro polyadd off

vmovdqa	    (\off*64)*2(%rsi),%ymm4
vmovdqa		(\off*64+16)*2(%rsi),%ymm5
vmovdqa		(\off*64+32)*2(%rsi),%ymm6
vmovdqa		(\off*64+48)*2(%rsi),%ymm7

vmovdqa	    (\off*64)*2(%rdx),%ymm8
vmovdqa		(\off*64+16)*2(%rdx),%ymm9
vmovdqa		(\off*64+32)*2(%rdx),%ymm10
vmovdqa		(\off*64+48)*2(%rdx),%ymm11

vpaddw		%ymm8,%ymm4,%ymm4
vpaddw		%ymm9,%ymm5,%ymm5
vpaddw		%ymm10,%ymm6,%ymm6
vpaddw		%ymm11,%ymm7,%ymm7

barret  4
barret  5
barret  6
barret  7

vmovdqa     %ymm4, (\off*64)*2(%rdi)
vmovdqa     %ymm5, (\off*64+16)*2(%rdi)
vmovdqa     %ymm6, (\off*64+32)*2(%rdi)
vmovdqa     %ymm7, (\off*64+48)*2(%rdi)

.endm

.macro bmul off

vmovdqa	    (\off*64)*2(%rsi),%ymm4
vmovdqa		(\off*64+16)*2(%rsi),%ymm5
vmovdqa		(\off*64+32)*2(%rsi),%ymm6
vmovdqa		(\off*64+48)*2(%rsi),%ymm7

mul        4,5,15,15,2,2
mul        6,7,15,15,2,2

vmovdqa	    (\off*64)*2(%rdx),%ymm8
vmovdqa		(\off*64+16)*2(%rdx),%ymm9

vpmullw		%ymm8,%ymm1,%ymm10
vpmullw		%ymm9,%ymm1,%ymm11

mul        4,5,10,11,8,9

vmovdqa     %ymm4, (\off*64)*2(%rdi)
vmovdqa     %ymm5, (\off*64+16)*2(%rdi)

vmovdqa	    (\off*64+32)*2(%rdx),%ymm8
vmovdqa		(\off*64+48)*2(%rdx),%ymm9

vpmullw		%ymm8,%ymm1,%ymm10
vpmullw		%ymm9,%ymm1,%ymm11

mul        6,7,10,11,8,9

vmovdqa     %ymm6, (\off*64+32)*2(%rdi)
vmovdqa     %ymm7, (\off*64+48)*2(%rdi)

.endm

.text
.global basemul_mont_avx
basemul_mont_avx:

vpbroadcastq    (_16xQ)*2(%rcx),%ymm0
vpbroadcastq    (_16x2Qinv)*2(%rcx),%ymm1
vpbroadcastq    (_MONT_CONST)*2(%rcx),%ymm15
vpbroadcastq    (_MONT_CONST+4)*2(%rcx),%ymm2

bmul 0
bmul 1

ret


.text
.global polyadd_avx
polyadd_avx:

vpbroadcastq    (_16xQ)*2(%rcx),%ymm0
vpbroadcastq    (_BARRET_CONST)*2(%rcx),%ymm1

polyadd 0
polyadd 1

ret

.text
.global polysub_avx
polysub_avx:

vpbroadcastq    (_16xQ)*2(%rcx),%ymm0

polysub 0
polysub 1

ret

.text
.global polyreduce_avx
polyreduce_avx:

vpbroadcastq    (_BARRET_CONST)*2(%rsi),%ymm1
vpbroadcastq    (_16xQ)*2(%rsi),%ymm0

polyreduce 0
polyreduce 1

ret