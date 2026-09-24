.include "arith.inc"

#define _16xQ 0
#define _16x2Qinv 4
#define _BARRET_CONST 328
#define _MONT_CONST 332

.text
.global basemul_mont_avx
basemul_mont_avx:

#load a

vmovdqa	    (%rsi),%ymm4
vmovdqa		(16)*2(%rsi),%ymm5
vmovdqa		(32)*2(%rsi),%ymm6
vmovdqa		(48)*2(%rsi),%ymm7

vpbroadcastq    (_16xQ)*2(%rcx),%ymm0
vpbroadcastq    (_16x2Qinv)*2(%rcx),%ymm1
vpbroadcastq    (_MONT_CONST)*2(%rcx),%ymm15
vpbroadcastq    (_MONT_CONST+4)*2(%rcx),%ymm2

mul        4,5,15,15,2,2
mul        6,7,15,15,2,2

vmovdqa	    (%rdx),%ymm8
vmovdqa		(16)*2(%rdx),%ymm9

vpmullw		%ymm8,%ymm1,%ymm15
vpmullw		%ymm9,%ymm1,%ymm14

mul        4,5,15,14,8,9

vmovdqa     %ymm4, (%rdi)
vmovdqa     %ymm5, (16)*2(%rdi)

vmovdqa	    (32)*2(%rdx),%ymm8
vmovdqa		(48)*2(%rdx),%ymm9

vpmullw		%ymm8,%ymm1,%ymm15
vpmullw		%ymm9,%ymm1,%ymm14

mul        6,7,15,14,8,9

vmovdqa     %ymm6, (32)*2(%rdi)
vmovdqa     %ymm7, (48)*2(%rdi)

ret

.text
.global polyadd_avx
polyadd_avx:

vpbroadcastq    (_16xQ)*2(%rcx),%ymm0

vmovdqa	    (%rsi),%ymm4
vmovdqa		(16)*2(%rsi),%ymm5
vmovdqa		(32)*2(%rsi),%ymm6
vmovdqa		(48)*2(%rsi),%ymm7

vmovdqa	    (%rdx),%ymm8
vmovdqa		(16)*2(%rdx),%ymm9
vmovdqa		(32)*2(%rdx),%ymm10
vmovdqa		(48)*2(%rdx),%ymm11

vpaddw		%ymm8,%ymm4,%ymm4
vpaddw		%ymm9,%ymm5,%ymm5
vpaddw		%ymm10,%ymm6,%ymm6
vpaddw		%ymm11,%ymm7,%ymm7

vpbroadcastq    (_BARRET_CONST)*2(%rcx),%ymm1
barret 4
barret 5
barret 6
barret 7

vmovdqa     %ymm4, (%rdi)
vmovdqa     %ymm5, (16)*2(%rdi)
vmovdqa     %ymm6, (32)*2(%rdi)
vmovdqa     %ymm7, (48)*2(%rdi)

ret

.text
.global polysub_avx
polysub_avx:

vpbroadcastq    (_16xQ)*2(%rcx),%ymm0

vmovdqa	    (%rsi),%ymm4
vmovdqa		(16)*2(%rsi),%ymm5
vmovdqa		(32)*2(%rsi),%ymm6
vmovdqa		(48)*2(%rsi),%ymm7

vmovdqa	    (%rdx),%ymm8
vmovdqa		(16)*2(%rdx),%ymm9
vmovdqa		(32)*2(%rdx),%ymm10
vmovdqa		(48)*2(%rdx),%ymm11

vpsubw		%ymm8,%ymm4,%ymm4
vpsubw		%ymm9,%ymm5,%ymm5
vpsubw		%ymm10,%ymm6,%ymm6
vpsubw		%ymm11,%ymm7,%ymm7

caddq 4
caddq 5
caddq 6
caddq 7

vmovdqa     %ymm4, (%rdi)
vmovdqa     %ymm5, (16)*2(%rdi)
vmovdqa     %ymm6, (32)*2(%rdi)
vmovdqa     %ymm7, (48)*2(%rdi)

ret 

.text
.global polyreduce_avx
polyreduce_avx:

vmovdqa	    (%rdi),%ymm4
vmovdqa		(16)*2(%rdi),%ymm5
vmovdqa		(32)*2(%rdi),%ymm6
vmovdqa		(48)*2(%rdi),%ymm7

vpbroadcastq    (_BARRET_CONST)*2(%rsi),%ymm1
vpbroadcastq    (_16xQ)*2(%rsi),%ymm0

barret  4
barret  5
barret  6
barret  7

vmovdqa     %ymm4, (%rdi)
vmovdqa     %ymm5, (16)*2(%rdi)
vmovdqa     %ymm6, (32)*2(%rdi)
vmovdqa     %ymm7, (48)*2(%rdi)

ret