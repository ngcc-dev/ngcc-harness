
.include "shuffle.inc"
#include "consts.h"
.macro levelzero l,r
vpmullw %ymm0,%ymm\r,%ymm12
//a*zeta
vpmulhw %ymm3,%ymm\r,%ymm13
//a*zeta*qinv*q
vpmulhw %ymm1,%ymm12,%ymm12
vpsubw  %ymm12,%ymm13,%ymm12
vpaddw %ymm\l,%ymm\r,%ymm\r
vpsubw %ymm12,%ymm\r,%ymm\r
vpaddw %ymm12,%ymm\l,%ymm\l
.endm

.macro butterfly l,r,z
//a*zeta*qinv
vpmullw %ymm0,%ymm\r,%ymm12
//a*zeta
vpmulhw %ymm\z,%ymm\r,%ymm13
//a*zeta*qinv*q
vpmulhw %ymm1,%ymm12,%ymm12
vpsubw  %ymm12,%ymm13,%ymm12
//a[j]-t
vpsubw %ymm12,%ymm\l,%ymm\r
//a[j] + t
vpaddw %ymm12,%ymm\l,%ymm\l
.endm

.text
ntt_levels0t2_avx:
level0:

vmovdqa	 (%rdx),%ymm3
vmovdqa	 (%r8),%ymm0

vmovdqa	 (%rdi),%ymm4
vmovdqa	 256(%rdi),%ymm5
vmovdqa	 512(%rdi),%ymm6
vmovdqa	 768(%rdi),%ymm7

vmovdqa	 1024(%rdi),%ymm8
vmovdqa	 1280(%rdi),%ymm9
vmovdqa	 1536(%rdi),%ymm10
vmovdqa	 1792(%rdi),%ymm11

levelzero 4,8
levelzero 5,9
levelzero 6,10
levelzero 7,11

level1:
vmovdqa     32(%rdx),%ymm3
vmovdqa	    32(%r8),%ymm0
butterfly   4,6,3
butterfly   5,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa	    64(%r8),%ymm0
butterfly   8,10,3
butterfly   9,11,3
level2:

vmovdqa     96(%rdx),%ymm3
vmovdqa	    96(%r8),%ymm0
butterfly   4,5,3
vmovdqa     128(%rdx),%ymm3
vmovdqa	    128(%r8),%ymm0
butterfly   6,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa	    160(%r8),%ymm0
butterfly   8,9,3
vmovdqa     192(%rdx),%ymm3
vmovdqa	    192(%r8),%ymm0
butterfly   10,11,3

#store
vmovdqa		%ymm4,(%rdi)
vmovdqa		%ymm5,256(%rdi)
vmovdqa		%ymm6,512(%rdi)
vmovdqa		%ymm7,768(%rdi)
vmovdqa		%ymm8,1024(%rdi)
vmovdqa		%ymm9,1280(%rdi)
vmovdqa		%ymm10,1536(%rdi)
vmovdqa		%ymm11,1792(%rdi)
ret
ntt_levels3t6_avx:
#load
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11
level3://64
vmovdqa     (%rdx),%ymm3
vmovdqa	    (%r8),%ymm0
butterfly	4,8,3
butterfly	5,9,3
butterfly	6,10,3
butterfly	7,11,3


levvel4://32
shuffle8	4,8,3,8
shuffle8	5,9,4,9
shuffle8	6,10,5,10
shuffle8	7,11,6,11

vmovdqa     32(%rdx),%ymm7
vmovdqa	    32(%r8),%ymm0
butterfly	3,5,7
butterfly	8,10,7
butterfly	4,6,7
butterfly	9,11,7

level5://16
vmovdqa     64(%rdx),%ymm7
vmovdqa	    64(%r8),%ymm0
butterfly	3,4,7
butterfly	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa	    96(%r8),%ymm0
butterfly	5,6,7
butterfly	10,11,7

level6://8
vmovdqa     128(%rdx),%ymm7
vmovdqa	    128(%r8),%ymm0
butterfly	3,8,7
vmovdqa     160(%rdx),%ymm7
vmovdqa	    160(%r8),%ymm0
butterfly	4,9,7
vmovdqa     192(%rdx),%ymm7
vmovdqa	    192(%r8),%ymm0
butterfly	5,10,7
vmovdqa     224(%rdx),%ymm7
vmovdqa	    224(%r8),%ymm0
butterfly	6,11,7


shuffle4	3,5,7,5
shuffle4	8,10,3,10
shuffle4	4,6,8,6
shuffle4	9,11,4,11

shuffle2   7,8
shuffle2   5,6
shuffle2   3,4
shuffle2   10,11

shuffle1   7,3
shuffle1   8,4
shuffle1   5,10
shuffle1   6,11

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm8,64(%rdi)
vmovdqa		%ymm4,96(%rdi)
vmovdqa		%ymm5,128(%rdi)
vmovdqa		%ymm10,160(%rdi)
vmovdqa		%ymm6,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
ret

.global ntt_avx
ntt_avx:


vmovdqa		32(%rsi),%ymm1
vmovdqa		64(%rsi),%ymm2

#levels0t2
lea		(48+16)*2(%rsi),%rdx
lea		(5360*2)(%rsi),%r8
xor		%eax,%eax
_looptop1:
call		ntt_levels0t2_avx
add		$32,%rdi
add		$32,%eax
cmp		$256,%eax
jne		_looptop1

sub		$256,%rdi
add		$224,%rdx
add		$224,%r8
xor		%eax,%eax

_looptop2:
call		ntt_levels3t6_avx
add		$256,%rdi
add		$256,%rdx
add		$256,%r8
add		$32,%eax
cmp		$256,%eax
jne		_looptop2

ret
