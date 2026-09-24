#include "consts.h"
.macro update3 off,l,h,z,rh0,rh1,rh2
vpaddw      %ymm\l,%ymm\h,%ymm15
vpsubw      %ymm\h,%ymm\z,%ymm1
vpaddw      %ymm\l,%ymm\z,%ymm2
vpaddw      \off(%rdi),%ymm15,%ymm\rh0
vpaddw      \off(%rdi),%ymm1,%ymm\rh1
vmovdqa     \off(%rdi),%ymm1
vpsubw      %ymm2,%ymm1,%ymm\rh2
.endm


.macro butterfly l,h,z=12,zl0=1,zl1=2,zh0=0
#mul
vpmullw		%ymm\zl0,%ymm\h,%ymm\z
vpmulhw		%ymm\zl1,%ymm\h,%ymm\h
#reduce
vpmulhw		%ymm\zh0,%ymm\z,%ymm\z
vpsubw		%ymm\z,%ymm\h,%ymm\z
#update
vpsubw      %ymm\z,%ymm\l,%ymm\h
vpaddw      %ymm\l,%ymm\z,%ymm\l
.endm
.macro update1 off,l,h,rh0,rh1
vpaddw		%ymm\h,%ymm\l,%ymm\rh0
vpsubw		%ymm\h,%ymm\l,%ymm\h
vpaddw		\off(%rsi),%ymm\h,%ymm\rh1
.endm
.macro update2 l,h,rh0,rh1
vpaddw		%ymm\l,%ymm\h,%ymm\rh0
vpsubw		%ymm\l,%ymm\h,%ymm\rh1
.endm

.macro fqmul l1,l2,h,r
#mul
vpmullw		%ymm\l1,%ymm\h,%ymm\r
vpmulhw		%ymm\l2,%ymm\h,%ymm\h
#reduce
vpmulhw		%ymm0,%ymm\r,%ymm\r
vpsubw		%ymm\r,%ymm\h,%ymm\r
.endm
# v store barret_reduce v
.macro barret v,l,r
vpmulhw     %ymm\v,%ymm\l,%ymm1
vpsraw      $10,%ymm1,%ymm1
vpmullw     %ymm0,%ymm1,%ymm1
vpsubw      %ymm1,%ymm\l,%ymm\r
.endm
# shuffle
.macro shuffle8 rh0,rh1,rh2,rh3
vperm2i128	$0x20,%ymm\rh0,%ymm\rh1,%ymm\rh2
vperm2i128	$0x31,%ymm\rh0,%ymm\rh1,%ymm\rh3
.endm
.macro shuffle4 rh0,rh1,rh2,rh3
vpunpcklqdq	%ymm\rh0,%ymm\rh1,%ymm\rh2
vpunpckhqdq	%ymm\rh0,%ymm\rh1,%ymm\rh3
.endm
.macro shuffle2 rh0,rh1,rh2,rh3
vpsllq		$32,%ymm\rh0,%ymm\rh2
vpblendd	$0xAA,%ymm\rh2,%ymm\rh1,%ymm\rh2
vpsrlq		$32,%ymm\rh1,%ymm\rh1
vpblendd	$0xAA,%ymm\rh0,%ymm\rh1,%ymm\rh3
.endm
.macro shuffle1 rh0,rh1,rh2,rh3
vpslld		$16,%ymm\rh0,%ymm\rh2
vpblendw	$0xAA,%ymm\rh2,%ymm\rh1,%ymm\rh2
vpsrld		$16,%ymm\rh1,%ymm\rh1
vpblendw	$0xAA,%ymm\rh0,%ymm\rh1,%ymm\rh3
.endm
.macro level0t2 y,xh1,xh2,xh3,off
# level 0
vmovdqa    (\y+  0+32*\off)(%rdi),%ymm4
vmovdqa    (\y+ 96+32*\off)(%rdi),%ymm5
vmovdqa    (\y+192+32*\off)(%rdi),%ymm6
vmovdqa    (\y+288+32*\off)(%rdi),%ymm7
vmovdqa    (\y+384+32*\off)(%rdi),%ymm8
vmovdqa    (\y+480+32*\off)(%rdi),%ymm9
vmovdqa    (\y+576+32*\off)(%rdi),%ymm10
vmovdqa    (\y+672+32*\off)(%rdi),%ymm11

vpbroadcastw   (4+\xh1)(%rdx),%ymm1   
vpbroadcastw   (6+\xh1)(%rdx),%ymm2  


butterfly  4,8
butterfly  5,9
butterfly  6,10
butterfly  7,11
# level 1
vpbroadcastw   (12+\xh2)(%rdx),%ymm1   
vpbroadcastw   (14+\xh2)(%rdx),%ymm2  
butterfly  4,6 
butterfly  5,7

vpbroadcastw   (16+\xh2)(%rdx),%ymm1   
vpbroadcastw   (18+\xh2)(%rdx),%ymm2 
butterfly  8,10
butterfly  9,11

# level 2
vpbroadcastw   (28+\xh3)(%rdx),%ymm1
vpbroadcastw   (30+\xh3)(%rdx),%ymm2
butterfly  4,5
vpbroadcastw   (32+\xh3)(%rdx),%ymm1
vpbroadcastw   (34+\xh3)(%rdx),%ymm2
butterfly  6,7
vpbroadcastw   (36+\xh3)(%rdx),%ymm1
vpbroadcastw   (38+\xh3)(%rdx),%ymm2
butterfly  8,9
vpbroadcastw   (40+\xh3)(%rdx),%ymm1
vpbroadcastw   (42+\xh3)(%rdx),%ymm2
butterfly  10,11


vmovdqa		%ymm4,(\y+  0+32*\off)(%rdi)
vmovdqa		%ymm5,(\y+ 96+32*\off)(%rdi)
vmovdqa		%ymm6,(\y+192+32*\off)(%rdi)
vmovdqa		%ymm7,(\y+288+32*\off)(%rdi)
vmovdqa		%ymm8,(\y+384+32*\off)(%rdi)
vmovdqa		%ymm9,(\y+480+32*\off)(%rdi)
vmovdqa		%ymm10,(\y+576+32*\off)(%rdi)
vmovdqa		%ymm11,(\y+672+32*\off)(%rdi)
.endm


.text
.global cdecl(ntt_avx)
cdecl(ntt_avx):
vmovdqa		_16xq(%rip),%ymm0
lea		zetas_exp(%rip),%rdx

#zetas
vpbroadcastw	(%rdx),%ymm1
vpbroadcastw	2(%rdx),%ymm2

xor		%rax,%rax
.p2align 5
# the first level
_loop0:
#load
vmovdqa		768(%rsi),%ymm4
vmovdqa		800(%rsi),%ymm5
vmovdqa		832(%rsi),%ymm6
vmovdqa		864(%rsi),%ymm7
vmovdqa		896(%rsi),%ymm8
vmovdqa		928(%rsi),%ymm9

fqmul   1,2,4,10
fqmul   1,2,5,11
fqmul   1,2,6,12
fqmul   1,2,7,13
fqmul   1,2,8,14
fqmul   1,2,9,15

#load
vmovdqa		(%rsi),%ymm4
vmovdqa		32(%rsi),%ymm5
vmovdqa		64(%rsi),%ymm6
vmovdqa		96(%rsi),%ymm7
vmovdqa		128(%rsi),%ymm8
vmovdqa		160(%rsi),%ymm9

#update
update1      768,4,10,3,10
update1      800,5,11,4,11
update1      832,6,12,5,12
update1      864,7,13,6,13
update1      896,8,14,7,14
update1      928,9,15,8,15

#store
vmovdqa		%ymm3,(%rdi)
vmovdqa		%ymm4,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm6,96(%rdi)
vmovdqa		%ymm7,128(%rdi)
vmovdqa		%ymm8,160(%rdi)
vmovdqa		%ymm10,768(%rdi)
vmovdqa		%ymm11,800(%rdi)
vmovdqa		%ymm12,832(%rdi)
vmovdqa		%ymm13,864(%rdi)
vmovdqa		%ymm14,896(%rdi)
vmovdqa		%ymm15,928(%rdi)

add		$192,%rdi
add		$192,%rsi
add		$192,%rax
cmp		$768,%rax
jb		_loop0

sub		$768,%rdi



level0t2    0,0,0,0,0
level0t2    0,0,0,0,1
level0t2    0,0,0,0,2
level0t2    768,4,8,16,0
level0t2    768,4,8,16,1
level0t2    768,4,8,16,2

xor		%rax,%rax
.p2align 5
lea		zetas_exp(%rip),%r8

_looptop_level3t5:
vmovdqa    (%rdi),%ymm3
vmovdqa    32(%rdi),%ymm4
vmovdqa    64(%rdi),%ymm5
vmovdqa    96(%rdi),%ymm10
vmovdqa    128(%rdi),%ymm11
vmovdqa    160(%rdi),%ymm12
vmovdqa    192(%rdi),%ymm6
vmovdqa    224(%rdi),%ymm7
vmovdqa    256(%rdi),%ymm8
vmovdqa    288(%rdi),%ymm13
vmovdqa    320(%rdi),%ymm14
vmovdqa    352(%rdi),%ymm15

#shuffle
shuffle8    10,3,2,3
shuffle8    11,4,10,4
shuffle8    12,5,11,5
shuffle8    13,6,12,6
shuffle8    14,7,13,7
shuffle8    15,8,14,8

#store
vmovdqa		%ymm2,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm10,64(%rdi)
vmovdqa		%ymm12,192(%rdi)
vmovdqa		%ymm6,224(%rdi)
vmovdqa		%ymm13,256(%rdi)

#level 3
#load
vmovdqa		%ymm5,%ymm6
vmovdqa		%ymm11,%ymm5
vmovdqa		%ymm8,%ymm9
vmovdqa		%ymm14,%ymm8
vmovdqu		60(%rdx),%ymm12
vmovdqu		92(%rdx),%ymm15
vmovdqu		316(%rdx),%ymm2
vmovdqu		348(%rdx),%ymm3

#mul
fqmul       12,2,4,10
fqmul       12,2,5,11
fqmul       12,2,6,12
fqmul       15,3,7,13
fqmul       15,3,8,14
fqmul       15,3,9,15

#load
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		192(%rdi),%ymm7
vmovdqa		224(%rdi),%ymm8
vmovdqa		256(%rdi),%ymm9

#update
update2     10,4,3,10
update2     11,5,4,11
update2     12,6,5,12
update2     13,7,6,13
update2     14,8,7,14
update2     15,9,8,15

#shuffle
shuffle4    10,3,2,3
shuffle4    11,4,10,4
shuffle4    12,5,11,5
shuffle4    13,6,12,6
shuffle4    14,7,13,7
shuffle4    15,8,14,8

#store
vmovdqa		%ymm2,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm10,64(%rdi)
vmovdqa		%ymm12,192(%rdi)
vmovdqa		%ymm6,224(%rdi)
vmovdqa		%ymm13,256(%rdi)

#level 4
#load
vmovdqa		%ymm5,%ymm6
vmovdqa		%ymm11,%ymm5
vmovdqa		%ymm8,%ymm9
vmovdqa		%ymm14,%ymm8
vmovdqu		572(%rdx),%ymm12
vmovdqu		604(%rdx),%ymm15
vmovdqu		828(%rdx),%ymm2
vmovdqu		860(%rdx),%ymm3

#mul
fqmul       12,2,4,10
fqmul       12,2,5,11
fqmul       12,2,6,12
fqmul       15,3,7,13
fqmul       15,3,8,14
fqmul       15,3,9,15

#load
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		192(%rdi),%ymm7
vmovdqa		224(%rdi),%ymm8
vmovdqa		256(%rdi),%ymm9

#update
update2     10,4,3,10
update2     11,5,4,11
update2     12,6,5,12
update2     13,7,6,13
update2     14,8,7,14
update2     15,9,8,15

#shuffle
shuffle2    10,3,2,3
shuffle2    11,4,10,4
shuffle2    12,5,11,5
shuffle2    13,6,12,6
shuffle2    14,7,13,7
shuffle2    15,8,14,8

#store
vmovdqa		%ymm2,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm10,64(%rdi)
vmovdqa		%ymm12,192(%rdi)
vmovdqa		%ymm6,224(%rdi)
vmovdqa		%ymm13,256(%rdi)

#level 5
#load
vmovdqa		%ymm5,%ymm6
vmovdqa		%ymm11,%ymm5
vmovdqa		%ymm8,%ymm9
vmovdqa		%ymm14,%ymm8
vmovdqu		1084(%rdx),%ymm12
vmovdqu		1116(%rdx),%ymm15
vmovdqu		1340(%rdx),%ymm2
vmovdqu		1372(%rdx),%ymm3

#mul
fqmul       12,2,4,10
fqmul       12,2,5,11
fqmul       12,2,6,12
fqmul       15,3,7,13
fqmul       15,3,8,14
fqmul       15,3,9,15

#load
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		192(%rdi),%ymm7
vmovdqa		224(%rdi),%ymm8
vmovdqa		256(%rdi),%ymm9

#update
update2     10,4,3,10
update2     11,5,4,11
update2     12,6,5,12
update2     13,7,6,13
update2     14,8,7,14
update2     15,9,8,15

vmovdqa		_16xv(%rip),%ymm2
#reduce
barret      2,3,3
barret      2,4,4
barret      2,5,5
barret      2,10,10
barret      2,11,11
barret      2,12,12
barret      2,6,6
barret      2,7,7
barret      2,8,8
barret      2,13,13
barret      2,14,14
barret      2,15,15
#store
vmovdqa		%ymm3,(%rdi)
vmovdqa		%ymm10,32(%rdi)
vmovdqa		%ymm6,64(%rdi)
vmovdqa		%ymm13,96(%rdi)

# radix 3 ntt   
vmovdqu		1596(%r8),%ymm1
vmovdqu		2620(%r8),%ymm2

fqmul       1,2,4,3
vmovdqu		1660(%r8),%ymm1
vmovdqu		2684(%r8),%ymm2
fqmul       1,2,11,10
vmovdqu		1724(%r8),%ymm1
vmovdqu		2748(%r8),%ymm2
fqmul       1,2,7,6
vmovdqu		1788(%r8),%ymm1
vmovdqu		2812(%r8),%ymm2
fqmul       1,2,14,13

vmovdqu		1628(%r8),%ymm1
vmovdqu		2652(%r8),%ymm2

fqmul       1,2,5,4
vmovdqu		1692(%r8),%ymm1
vmovdqu		2716(%r8),%ymm2
fqmul       1,2,12,11
vmovdqu		1756(%r8),%ymm1
vmovdqu		2780(%r8),%ymm2
fqmul       1,2,8,7
vmovdqu		1820(%r8),%ymm1
vmovdqu		2844(%r8),%ymm2
fqmul       1,2,15,14
# tb-tc
vpsubw      %ymm4,%ymm3,%ymm5
vpsubw      %ymm11,%ymm10,%ymm12
vpsubw      %ymm7,%ymm6,%ymm8
vpsubw      %ymm14,%ymm13,%ymm15


vmovdqu		_16xroot3qinv(%rip),%ymm1
vmovdqu		_16xroot3(%rip),%ymm2
fqmul       1,2,5,9
fqmul       1,2,12,5
fqmul       1,2,8,12
fqmul       1,2,15,8

#update
update3     0,3,4,9,3,4,9
update3     32,10,11,5,10,11,5
update3     64,6,7,12,6,7,12
update3     96,13,14,8,13,14,8

# load v

#shuffle
shuffle1    4,3,2,3
shuffle1    10,9,4,9
shuffle1    5,11,10,11
shuffle1    7,6,5,6
shuffle1    13,12,7,12
shuffle1    8,14,13,14

#store
vmovdqa		%ymm2,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm4,64(%rdi)
vmovdqa		%ymm9,96(%rdi)
vmovdqa		%ymm10,128(%rdi)
vmovdqa		%ymm11,160(%rdi)
vmovdqa		%ymm5,192(%rdi)
vmovdqa		%ymm6,224(%rdi)
vmovdqa		%ymm7,256(%rdi)
vmovdqa		%ymm12,288(%rdi)
vmovdqa		%ymm13,320(%rdi)
vmovdqa		%ymm14,352(%rdi)

add     $64,%rdx
add		$384,%rdi
add     $256,%r8
add		$8,%rax
cmp		$32,%rax
jb		_looptop_level3t5
ret