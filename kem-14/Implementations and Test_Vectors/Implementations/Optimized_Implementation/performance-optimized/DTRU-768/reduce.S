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

.global freeze_avx
freeze_avx:
.p2align 5
vmovdqa		_16xq(%rip),%ymm14
xor		%rax,%rax
_looptop_freeze:
vmovdqa		(%rdi),%ymm0
vmovdqa		32(%rdi),%ymm1
vmovdqa		64(%rdi),%ymm2
vmovdqa		96(%rdi),%ymm3
vmovdqa		128(%rdi),%ymm4
vmovdqa		160(%rdi),%ymm5

vmovdqa		_16xv(%rip),%ymm15

barret      15,0,0
barret      15,1,1
barret      15,2,2
barret      15,3,3
barret      15,4,4
barret      15,5,5

fqcsubq     0,6
fqcsubq     1,7
fqcsubq     2,8
fqcsubq     3,9
fqcsubq     4,10
fqcsubq     5,11

vmovdqa		%ymm6,(%rdi)
vmovdqa		%ymm7,32(%rdi)
vmovdqa		%ymm8,64(%rdi)
vmovdqa		%ymm9,96(%rdi)
vmovdqa		%ymm10,128(%rdi)
vmovdqa		%ymm11,160(%rdi)

add		$192,%rdi
add		$1,%rax
cmp		$8,%rax
jb		_looptop_freeze

ret

.global barret_avx
barret_avx:
.p2align 5
vmovdqa		_16xq(%rip),%ymm14
xor		%rax,%rax
_looptop_barret:
vmovdqa		(%rdi),%ymm0
vmovdqa		32(%rdi),%ymm1
vmovdqa		64(%rdi),%ymm2
vmovdqa		96(%rdi),%ymm3
vmovdqa		128(%rdi),%ymm4
vmovdqa		160(%rdi),%ymm5

vmovdqa		_16xv(%rip),%ymm15

barret      15,0,0
barret      15,1,1
barret      15,2,2
barret      15,3,3
barret      15,4,4
barret      15,5,5

vmovdqa		%ymm0,(%rdi)
vmovdqa		%ymm1,32(%rdi)
vmovdqa		%ymm2,64(%rdi)
vmovdqa		%ymm3,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm5,160(%rdi)

add		$192,%rdi
add		$1,%rax
cmp		$8,%rax
jb		_looptop_barret

ret
