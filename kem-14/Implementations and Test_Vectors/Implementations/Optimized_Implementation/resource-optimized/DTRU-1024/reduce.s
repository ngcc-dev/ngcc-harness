#include "consts.h"
.macro barret v,l,r
vpmulhw     %ymm\v,%ymm\l,%ymm12
vpsraw      $10,%ymm12,%ymm12
vpmullw     %ymm14,%ymm12,%ymm12
vpsubw      %ymm12,%ymm\l,%ymm\r
.endm

.macro fqcsubq l,h
# a >> 15
vpsraw      $15,%ymm\l,%ymm\h
# (a >> 15) & Q
vpand       %ymm14,%ymm\h,%ymm\h
# a + (a >> 15) & Q
vpaddw      %ymm\l,%ymm\h,%ymm\h
# a - Q
vpsubw      %ymm14,%ymm\h,%ymm\h
# a >> 15
vpsraw      $15,%ymm\h,%ymm\l
# (a >> 15) & Q
vpand       %ymm14,%ymm\l,%ymm\l
# a + (a >> 15) & Q
vpaddw      %ymm\h,%ymm\l,%ymm\h
.endm

.macro fqred16 l
vpsllw  $3,%ymm\l,%ymm8
vpsrlw  $3,%ymm8,%ymm8
vpsraw  $13,%ymm\l,%ymm\l
vpsllw  $9,%ymm\l,%ymm9
vpsubw  %ymm\l,%ymm9,%ymm\l
vpaddw  %ymm8,%ymm\l,%ymm\l
.endm



.global freeze_avx
freeze_avx:
.p2align 5
//q
vmovdqa		32(%rsi),%ymm14
xor		%rax,%rax
_looptop_freeze:
vmovdqa		(%rdi),%ymm0
vmovdqa		32(%rdi),%ymm1
vmovdqa		64(%rdi),%ymm2
vmovdqa		96(%rdi),%ymm3

//v
vmovdqa		64(%rsi),%ymm15

barret      15,0,0
barret      15,1,1
barret      15,2,2
barret      15,3,3


fqcsubq     0,6
fqcsubq     1,7
fqcsubq     2,8
fqcsubq     3,9


vmovdqa		%ymm6,(%rdi)
vmovdqa		%ymm7,32(%rdi)
vmovdqa		%ymm8,64(%rdi)
vmovdqa		%ymm9,96(%rdi)


add		$128,%rdi
add		$1,%rax
cmp		$16,%rax
jb		_looptop_freeze

ret

.global barret_avx
barret_avx:
.p2align 5
vmovdqa	32(%rsi),%ymm14
xor		%rax,%rax
_looptop_barret:
vmovdqa		(%rdi),%ymm0
vmovdqa		32(%rdi),%ymm1
vmovdqa		64(%rdi),%ymm2
vmovdqa		96(%rdi),%ymm3
vmovdqa		128(%rdi),%ymm4
vmovdqa		160(%rdi),%ymm5
vmovdqa	    192(%rdi),%ymm6
vmovdqa		224(%rdi),%ymm7

//v
vmovdqa		64(%rsi),%ymm15

barret      15,0,0
barret      15,1,1
barret      15,2,2
barret      15,3,3
barret      15,4,4
barret      15,5,5
barret      15,6,6
barret      15,7,7


vmovdqa		%ymm0,(%rdi)
vmovdqa		%ymm1,32(%rdi)
vmovdqa		%ymm2,64(%rdi)
vmovdqa		%ymm3,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm5,160(%rdi)
vmovdqa		%ymm6,192(%rdi)
vmovdqa		%ymm7,224(%rdi)


add		$256,%rdi
add		$1,%rax
cmp		$8,%rax
jb		_looptop_barret

ret

.global fqred16_avx
fqred16_avx:
.p2align 5
xor         %eax,%eax
_looptop_fqred16:
vmovdqa		(%rdi),%ymm0
vmovdqa		32(%rdi),%ymm1
vmovdqa		64(%rdi),%ymm2
vmovdqa		96(%rdi),%ymm3
vmovdqa		128(%rdi),%ymm4
vmovdqa		160(%rdi),%ymm5
vmovdqa		192(%rdi),%ymm6
vmovdqa		224(%rdi),%ymm7

fqred16     0
fqred16     1
fqred16     2
fqred16     3
fqred16     4
fqred16     5
fqred16     6
fqred16     7

vmovdqa		%ymm0,(%rdi)
vmovdqa		%ymm1,32(%rdi)
vmovdqa		%ymm2,64(%rdi)
vmovdqa		%ymm3,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm5,160(%rdi)
vmovdqa		%ymm6,192(%rdi)
vmovdqa		%ymm7,224(%rdi)
add		$256,%rdi
add		$1,%eax
cmp		$8,%eax
jb		_looptop_fqred16
ret
