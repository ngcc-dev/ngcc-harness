#include "consts.h"
.macro update2 off,l,h,rh0
vmovdqa     (\off)(%rdi),%ymm\rh0
vpsubw      %ymm\l,%ymm\rh0,%ymm\rh0
vpsubw      %ymm\h,%ymm\rh0,%ymm\rh0
.endm
.macro update1 off,h,z
vmovdqa     (\off)(%rdi),%ymm1
vpsubw      %ymm\h,%ymm1,%ymm\z
.endm
.macro butterfly l,h,z=12,zl0=1,zl1=2,zh0=0
vpsubw      %ymm\h,%ymm\l,%ymm\z
vpaddw      %ymm\l,%ymm\h,%ymm\l
#mul
vpmullw		%ymm\zl0,%ymm\z,%ymm\h
vpmulhw		%ymm\zl1,%ymm\z,%ymm\z
#reduce
vpmulhw		%ymm\zh0,%ymm\h,%ymm\h
vpsubw		%ymm\h,%ymm\z,%ymm\h
.endm
 
.macro fqmul l1,l2,h,r
#mul
vpmullw		%ymm\l1,%ymm\h,%ymm\r
vpmulhw		%ymm\l2,%ymm\h,%ymm\h
#reduce
vpmulhw		%ymm0,%ymm\r,%ymm\r
vpsubw		%ymm\r,%ymm\h,%ymm\r
.endm
 .macro centralbarret l
vpmulhw     %ymm2,%ymm\l,%ymm1
vpaddw      %ymm9,%ymm1,%ymm1
vpsraw      $10,%ymm1,%ymm1
vpmullw     %ymm0,%ymm1,%ymm1
vpsubw      %ymm1,%ymm\l,%ymm\l
.endm
.macro barret v,l,r
vpmulhw     %ymm\v,%ymm\l,%ymm1
vpsraw      $10,%ymm1,%ymm1
vpmullw     %ymm0,%ymm1,%ymm1
vpsubw      %ymm1,%ymm\l,%ymm\r
.endm

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

.macro level3t5_1 y,xh1,xh2,xh3,off
vmovdqa    (\y+  0+32*\off)(%rdi),%ymm4
vmovdqa    (\y+ 96+32*\off)(%rdi),%ymm5
vmovdqa    (\y+192+32*\off)(%rdi),%ymm6
vmovdqa    (\y+288+32*\off)(%rdi),%ymm7
vmovdqa    (\y+384+32*\off)(%rdi),%ymm8
vmovdqa    (\y+480+32*\off)(%rdi),%ymm9
vmovdqa    (\y+576+32*\off)(%rdi),%ymm10
vmovdqa    (\y+672+32*\off)(%rdi),%ymm11
vmovdqa		_16xv(%rip),%ymm3
vpbroadcastw   (28+\xh3)(%rdx),%ymm1
vpbroadcastw   (30+\xh3)(%rdx),%ymm2
butterfly  4,5

barret     3,6,6
vpbroadcastw   (32+\xh3)(%rdx),%ymm1
vpbroadcastw   (34+\xh3)(%rdx),%ymm2
butterfly  6,7
vpbroadcastw   (36+\xh3)(%rdx),%ymm1
vpbroadcastw   (38+\xh3)(%rdx),%ymm2
butterfly  8,9

barret     3,10,10
vpbroadcastw   (40+\xh3)(%rdx),%ymm1
vpbroadcastw   (42+\xh3)(%rdx),%ymm2
butterfly  10,11

barret     3,4,4
vpbroadcastw   (12+\xh2)(%rdx),%ymm1  
vpbroadcastw   (14+\xh2)(%rdx),%ymm2  
butterfly  4,6 
butterfly  5,7

barret     3,8,8
vpbroadcastw   (16+\xh2)(%rdx),%ymm1   
vpbroadcastw   (18+\xh2)(%rdx),%ymm2 
butterfly  8,10
butterfly  9,11

barret     3,4,4
vpbroadcastw   (4+\xh1)(%rdx),%ymm1 
vpbroadcastw   (6+\xh1)(%rdx),%ymm2  
butterfly  4,8
butterfly  5,9
butterfly  6,10
butterfly  7,11

vmovdqa		%ymm4,(\y+  0+32*\off)(%rdi)
vmovdqa		%ymm5,(\y+ 96+32*\off)(%rdi)
vmovdqa		%ymm6,(\y+192+32*\off)(%rdi)
vmovdqa		%ymm7,(\y+288+32*\off)(%rdi)
vmovdqa		%ymm8,(\y+384+32*\off)(%rdi)
vmovdqa		%ymm9,(\y+480+32*\off)(%rdi)
vmovdqa		%ymm10,(\y+576+32*\off)(%rdi)
vmovdqa		%ymm11,(\y+672+32*\off)(%rdi)
.endm


.macro level3t5_2 y,xh1,xh2,xh3,off
vmovdqa    (\y+  0+32*\off)(%rdi),%ymm4
vmovdqa    (\y+ 96+32*\off)(%rdi),%ymm5
vmovdqa    (\y+192+32*\off)(%rdi),%ymm6
vmovdqa    (\y+288+32*\off)(%rdi),%ymm7
vmovdqa    (\y+384+32*\off)(%rdi),%ymm8
vmovdqa    (\y+480+32*\off)(%rdi),%ymm9
vmovdqa    (\y+576+32*\off)(%rdi),%ymm10
vmovdqa    (\y+672+32*\off)(%rdi),%ymm11
vmovdqa		_16xv(%rip),%ymm3
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

vpbroadcastw   (12+\xh2)(%rdx),%ymm1  
vpbroadcastw   (14+\xh2)(%rdx),%ymm2 

butterfly  4,6 
butterfly  5,7

barret     3,8,8
vpbroadcastw   (16+\xh2)(%rdx),%ymm1   
vpbroadcastw   (18+\xh2)(%rdx),%ymm2 
butterfly  8,10
butterfly  9,11

barret     3,4,4 
vpbroadcastw   (4+\xh1)(%rdx),%ymm1 
vpbroadcastw   (6+\xh1)(%rdx),%ymm2  
butterfly  4,8
butterfly  5,9
butterfly  6,10
butterfly  7,11

vmovdqa		%ymm4,(\y+  0+32*\off)(%rdi)
vmovdqa		%ymm5,(\y+ 96+32*\off)(%rdi)
vmovdqa		%ymm6,(\y+192+32*\off)(%rdi)
vmovdqa		%ymm7,(\y+288+32*\off)(%rdi)
vmovdqa		%ymm8,(\y+384+32*\off)(%rdi)
vmovdqa		%ymm9,(\y+480+32*\off)(%rdi)
vmovdqa		%ymm10,(\y+576+32*\off)(%rdi)
vmovdqa		%ymm11,(\y+672+32*\off)(%rdi)
.endm


.macro level3t5_3 y,xh1,xh2,xh3,off
vmovdqa    (\y+  0+32*\off)(%rdi),%ymm4
vmovdqa    (\y+ 96+32*\off)(%rdi),%ymm5
vmovdqa    (\y+192+32*\off)(%rdi),%ymm6
vmovdqa    (\y+288+32*\off)(%rdi),%ymm7
vmovdqa    (\y+384+32*\off)(%rdi),%ymm8
vmovdqa    (\y+480+32*\off)(%rdi),%ymm9
vmovdqa    (\y+576+32*\off)(%rdi),%ymm10
vmovdqa    (\y+672+32*\off)(%rdi),%ymm11
vmovdqa		_16xv(%rip),%ymm3
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

vpbroadcastw   (12+\xh2)(%rdx),%ymm1  
vpbroadcastw   (14+\xh2)(%rdx),%ymm2  
butterfly  4,6 
butterfly  5,7
vpbroadcastw   (16+\xh2)(%rdx),%ymm1   
vpbroadcastw   (18+\xh2)(%rdx),%ymm2 
butterfly  8,10
butterfly  9,11

barret     3,4,4 
vpbroadcastw   (4+\xh1)(%rdx),%ymm1 
vpbroadcastw   (6+\xh1)(%rdx),%ymm2  
butterfly  4,8
butterfly  5,9
butterfly  6,10
butterfly  7,11

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
.global cdecl(invntt_avx)
cdecl(invntt_avx):

lea		zetas_inv_exp(%rip),%r8
vmovdqa		_16xq(%rip),%ymm0
lea		zetas_inv_exp(%rip),%rdx
xor     %rax,%rax
_looptop_level0t2:
vmovdqa    (%rdi),%ymm15
vmovdqa    32(%rdi),%ymm3
vmovdqa    64(%rdi),%ymm2
vmovdqa    96(%rdi),%ymm11
vmovdqa    128(%rdi),%ymm4
vmovdqa    160(%rdi),%ymm10
vmovdqa    192(%rdi),%ymm5
vmovdqa    224(%rdi),%ymm6
vmovdqa    256(%rdi),%ymm12
vmovdqa    288(%rdi),%ymm14
vmovdqa    320(%rdi),%ymm7
vmovdqa    352(%rdi),%ymm13

#shuffle
shuffle1    3,15,8,15
shuffle1    11,2,3,2
shuffle1    10,4,11,4
shuffle1    6,5,10,5
shuffle1    14,12,6,12
shuffle1    13,7,14,7

vmovdqa     %ymm2,%ymm13
//1
#store
vmovdqa		%ymm8,(%rdi)
vmovdqa		%ymm13,32(%rdi)
vmovdqa		%ymm10,64(%rdi)
vmovdqa		%ymm12,96(%rdi)



vpsubw      %ymm3,%ymm15,%ymm8
vpsubw      %ymm4,%ymm11,%ymm13
vpsubw      %ymm6,%ymm5,%ymm10
vpsubw      %ymm7,%ymm14,%ymm12
vmovdqu		_16xroot3qinv(%rip),%ymm1
vmovdqu		_16xroot3(%rip),%ymm2

fqmul       1,2,8,9
fqmul       1,2,13,8
fqmul       1,2,10,13
fqmul       1,2,12,10

vmovdqa     %ymm9,%ymm12


update2     0,15,12,0
update2     32,11,8,1
update2     64,5,13,2
update2     96,14,10,9

vpaddw      (%rdi),%ymm12,%ymm12
vpsubw      %ymm3,%ymm12,%ymm12

vpaddw      32(%rdi),%ymm8,%ymm8
vpsubw      %ymm4,%ymm8,%ymm8

vpaddw      64(%rdi),%ymm13,%ymm13
vpsubw      %ymm6,%ymm13,%ymm13

vpaddw      96(%rdi),%ymm10,%ymm10
vpsubw      %ymm7,%ymm10,%ymm10

vpaddw      (%rdi),%ymm15,%ymm15
vpaddw      %ymm15,%ymm3,%ymm3

vpaddw      32(%rdi),%ymm11,%ymm11
vpaddw      %ymm11,%ymm4,%ymm4

vpaddw      64(%rdi),%ymm5,%ymm5
vpaddw      %ymm5,%ymm6,%ymm6

vpaddw      96(%rdi),%ymm14,%ymm14
vpaddw      %ymm14,%ymm7,%ymm7

vmovdqa     %ymm0,%ymm15
vmovdqa     %ymm1,%ymm11
vmovdqa     %ymm2,%ymm5
vmovdqa     %ymm9,%ymm14

vmovdqa		_16xq(%rip),%ymm0
vmovdqu		1596(%r8),%ymm1
vmovdqu		2620(%r8),%ymm2
fqmul       1,2,15,9
vmovdqu		1660(%r8),%ymm1
vmovdqu		2684(%r8),%ymm2
fqmul       1,2,11,15
vmovdqu		1724(%r8),%ymm1
vmovdqu		2748(%r8),%ymm2
fqmul       1,2,5,11
vmovdqu		1788(%r8),%ymm1
vmovdqu		2812(%r8),%ymm2
fqmul       1,2,14,5

vmovdqu		1628(%r8),%ymm1
vmovdqu		2652(%r8),%ymm2
fqmul       1,2,12,14
vmovdqu		1692(%r8),%ymm1
vmovdqu		2716(%r8),%ymm2
fqmul       1,2,8,12
vmovdqu		1756(%r8),%ymm1
vmovdqu		2780(%r8),%ymm2
fqmul       1,2,13,8
vmovdqu		1820(%r8),%ymm1
vmovdqu		2844(%r8),%ymm2
fqmul       1,2,10,13


vmovdqa     %ymm9,%ymm10

//2
#store
vmovdqa		%ymm3,(%rdi)
vmovdqa		%ymm10,32(%rdi)
vmovdqa		%ymm14,64(%rdi)
vmovdqa		%ymm6,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
vmovdqa		%ymm8,256(%rdi)


vpaddw      (%rdi),%ymm4,%ymm3
vpaddw      32(%rdi),%ymm15,%ymm10
vpaddw      64(%rdi),%ymm12,%ymm14
vpaddw      192(%rdi),%ymm7,%ymm6
vpaddw      224(%rdi),%ymm5,%ymm11
vpaddw      256(%rdi),%ymm13,%ymm8

#centralbarret_reduce
vmovdqa		_16xv(%rip),%ymm2
vmovdqa		_16x512(%rip),%ymm9


centralbarret      3
centralbarret      10
centralbarret      14
centralbarret      6
centralbarret      11
centralbarret      8

#store
//3
vmovdqa		%ymm3,96(%rdi)
vmovdqa		%ymm10,128(%rdi)
vmovdqa		%ymm14,160(%rdi)
vmovdqa		%ymm6,288(%rdi)
vmovdqa		%ymm11,320(%rdi)
vmovdqa		%ymm8,352(%rdi)


update1     0,4,4
update1     32,15,15
update1     64,12,12
update1     192,7,7
update1     224,5,5
update1     256,13,13


vmovdqu		1084(%rdx),%ymm1
vmovdqu		1116(%rdx),%ymm9
vmovdqu		1340(%rdx),%ymm2
vmovdqu		1372(%rdx),%ymm3

#mul
fqmul       1,2,4,10
fqmul       1,2,15,4
fqmul       1,2,12,15
fqmul       9,3,7,12
fqmul       9,3,5,7
fqmul       9,3,13,5

#load
vmovdqa		96(%rdi ),%ymm3 
vmovdqa		128(%rdi),%ymm6
vmovdqa		160(%rdi),%ymm8
vmovdqa		288(%rdi),%ymm11
vmovdqa		320(%rdi),%ymm13
vmovdqa		352(%rdi),%ymm14

shuffle2    6,3,9,3
shuffle2    10,8,6,8
shuffle2    15,4,10,4
shuffle2    13,11,15,11
shuffle2    12,14,13,14
shuffle2    5,7,12,7

#store
vmovdqa		%ymm9,0(%rdi)
vmovdqa		%ymm6,32(%rdi)
vmovdqa		%ymm10,64(%rdi)
vmovdqa		%ymm15,192(%rdi)
vmovdqa		%ymm13,224(%rdi)
vmovdqa		%ymm12,256(%rdi)

  
vpaddw      (%rdi),%ymm3,%ymm9
vpaddw      32(%rdi),%ymm8,%ymm6
vpaddw      64(%rdi),%ymm4,%ymm10
vpaddw      192(%rdi),%ymm11,%ymm15
vpaddw      224(%rdi),%ymm14,%ymm13
vpaddw      256(%rdi),%ymm7,%ymm12
 
update1     0,3,3
update1     32,8,8
update1     64,4,4
update1     192,11,11
update1     224,14,14
update1     256,7,7
 
vmovdqa		%ymm9,(%rdi)
vmovdqa		%ymm6,32(%rdi)
vmovdqa		%ymm10,64(%rdi)
vmovdqa		%ymm15,192(%rdi)
vmovdqa		%ymm13,224(%rdi)
vmovdqa		%ymm12,256(%rdi)

vmovdqu		572(%rdx),%ymm12
vmovdqu		604(%rdx),%ymm15
vmovdqu		828(%rdx),%ymm2
vmovdqu		860(%rdx),%ymm1

#mul
fqmul       12,2,3,9
fqmul       12,2,8,3
fqmul       12,2,4,8
fqmul       15,1,11,4
fqmul       15,1,14,11
fqmul       15,1,7,14

#load
vmovdqa		(%rdi),%ymm5
vmovdqa		32(%rdi),%ymm6
vmovdqa		64(%rdi),%ymm7
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm12
vmovdqa		256(%rdi),%ymm13


shuffle4    6,5,15,5
shuffle4    9,7,6,7
shuffle4    8,3,9,3
shuffle4    12,10,8,10
shuffle4    4,13,12,13
shuffle4    14,11,4,11

vmovdqa		%ymm15,(%rdi)
vmovdqa		%ymm6,32(%rdi)
vmovdqa		%ymm9,64(%rdi)
vmovdqa		%ymm8,192(%rdi)
vmovdqa		%ymm12,224(%rdi)
vmovdqa		%ymm4,256(%rdi)

vpaddw      (%rdi),%ymm5,%ymm15
vpaddw      32(%rdi),%ymm7,%ymm6
vpaddw      64(%rdi),%ymm3,%ymm9
vpaddw      192(%rdi),%ymm10,%ymm8
vpaddw      224(%rdi),%ymm13,%ymm12
vpaddw      256(%rdi),%ymm11,%ymm4

update1     0,5,5
update1     32,7,7
update1     64,3,3
update1     192,10,10
update1     224,13,13
update1     256,11,11

vmovdqa		%ymm15,(%rdi)
vmovdqa		%ymm6,32(%rdi)
vmovdqa		%ymm9,64(%rdi)
vmovdqa		%ymm8,192(%rdi)
vmovdqa		%ymm12,224(%rdi)
vmovdqa		%ymm4,256(%rdi)

vmovdqu		60(%rdx),%ymm12
vmovdqu		92(%rdx),%ymm14
vmovdqu		316(%rdx),%ymm2
vmovdqu		348(%rdx),%ymm4

#mul
fqmul       12,2,5,8
fqmul       12,2,7,5
fqmul       12,2,3,7
fqmul       14,4,10,3
fqmul       14,4,13,10
fqmul       14,4,11,13

#load
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm6
vmovdqa		64(%rdi),%ymm11
vmovdqa		192(%rdi),%ymm12
vmovdqa		224(%rdi),%ymm14
vmovdqa		256(%rdi),%ymm15

shuffle8    6,4,9,4
shuffle8    8,11,6,11
shuffle8    7,5,8,5
shuffle8    14,12,7,12
shuffle8    3,15,14,15
shuffle8    13,10,3,10

vmovdqa		%ymm9,(%rdi)
vmovdqa		%ymm6,32(%rdi)
vmovdqa		%ymm8,64(%rdi)
vmovdqa		%ymm4,96(%rdi)
vmovdqa		%ymm11,128(%rdi)
vmovdqa		%ymm5,160(%rdi)
vmovdqa		%ymm7,192(%rdi)
vmovdqa		%ymm14,224(%rdi)
vmovdqa		%ymm3,256(%rdi)
vmovdqa		%ymm12,288(%rdi)
vmovdqa		%ymm15,320(%rdi)
vmovdqa		%ymm10,352(%rdi)

add     $64,%rdx
add		$384,%rdi
add     $256,%r8
add		$8,%rax
cmp		$32,%rax
jb		_looptop_level0t2

sub		$1536,%rdi
sub     $256,%rdx


level3t5_1    0,0,0,0,0
level3t5_2    0,0,0,0,1
level3t5_3    0,0,0,0,2
level3t5_1    768,4,8,16,0
level3t5_2    768,4,8,16,1
level3t5_3    768,4,8,16,2


#zetas



xor		%rax,%rax
.p2align 5
_loop6:
#load
vmovdqa		768(%rdi),%ymm4
vmovdqa		800(%rdi),%ymm5
vmovdqa		832(%rdi),%ymm6
vmovdqa		864(%rdi),%ymm7
vmovdqa		896(%rdi),%ymm8
vmovdqa		928(%rdi),%ymm9

update1     0,4,10
update1     32,5,11
update1     64,6,12
update1     96,7,13
update1     128,8,14
update1     160,9,15

vpbroadcastw	(%rdx),%ymm1
vpbroadcastw	2(%rdx),%ymm2

fqmul   1,2,10,3
fqmul   1,2,11,10
fqmul   1,2,12,11
fqmul   1,2,13,12
fqmul   1,2,14,13
fqmul   1,2,15,14


vpaddw      (%rdi),%ymm4,%ymm4
vpaddw      32(%rdi),%ymm5,%ymm5
vpaddw      64(%rdi),%ymm6,%ymm6
vpaddw      96(%rdi),%ymm7,%ymm7
vpaddw      128(%rdi),%ymm8,%ymm8
vpaddw      160(%rdi),%ymm9,%ymm9

vpsubw      %ymm3,%ymm4,%ymm4
vpsubw      %ymm10,%ymm5,%ymm5
vpsubw      %ymm11,%ymm6,%ymm6
vpsubw      %ymm12,%ymm7,%ymm7
vpsubw      %ymm13,%ymm8,%ymm8
vpsubw      %ymm14,%ymm9,%ymm9

vmovdqa		_16xn1qinv(%rip),%ymm1
vmovdqa		_16xn1(%rip),%ymm2

fqmul   1,2,4,15
fqmul   1,2,5,4
fqmul   1,2,6,5
fqmul   1,2,7,6
fqmul   1,2,8,7
fqmul   1,2,9,8

vmovdqa		_16xn2qinv(%rip),%ymm1
vmovdqa		_16xn2(%rip),%ymm2

fqmul   1,2,3,9
fqmul   1,2,10,3
fqmul   1,2,11,10
fqmul   1,2,12,11
fqmul   1,2,13,12
fqmul   1,2,14,13


#store
vmovdqa		%ymm15,(%rdi)
vmovdqa		%ymm4,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm6,96(%rdi)
vmovdqa		%ymm7,128(%rdi)
vmovdqa		%ymm8,160(%rdi)
vmovdqa		%ymm9,768(%rdi)
vmovdqa		%ymm3,800(%rdi)
vmovdqa		%ymm10,832(%rdi)
vmovdqa		%ymm11,864(%rdi)
vmovdqa		%ymm12,896(%rdi)
vmovdqa		%ymm13,928(%rdi)


add		$192,%rdi
add		$192,%rax
cmp		$768,%rax
jb		_loop6

ret