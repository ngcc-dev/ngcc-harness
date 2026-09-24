
.include "shuffle.inc"
#include "consts.h"

.macro shuffle_to1 off

vmovdqa	  (128*\off + 0)*2(%rsi),%ymm4
vmovdqa	  (128*\off + 16)*2(%rsi),%ymm5
vmovdqa	  (128*\off + 32)*2(%rsi),%ymm6
vmovdqa	  (128*\off + 48)*2(%rsi),%ymm7
vmovdqa	  (128*\off + 64)*2(%rsi),%ymm8
vmovdqa	  (128*\off + 80)*2(%rsi),%ymm9
vmovdqa	  (128*\off + 96)*2(%rsi),%ymm10
vmovdqa	  (128*\off + 112)*2(%rsi),%ymm11

shuffle8	4,8,3,8
shuffle8	5,9,4,9
shuffle8	6,10,5,10
shuffle8	7,11,6,11

shuffle4	3,5,7,5
shuffle4	8,10,3,10
shuffle4	4,6,8,6
shuffle4	9,11,4,11

shuffle2    7,8
shuffle2    5,6
shuffle2    3,4
shuffle2    10,11

shuffle1   7,3
shuffle1   8,4
shuffle1   5,10
shuffle1   6,11


vmovdqa		%ymm7,(128*\off + 0)*2(%rsi)
vmovdqa		%ymm3,(128*\off + 16)*2(%rsi)
vmovdqa		%ymm8,(128*\off + 32)*2(%rsi)
vmovdqa		%ymm4,(128*\off + 48)*2(%rsi)
vmovdqa		%ymm5,(128*\off + 64)*2(%rsi)
vmovdqa		%ymm10,(128*\off + 80)*2(%rsi)
vmovdqa		%ymm6,(128*\off + 96)*2(%rsi)
vmovdqa		%ymm11,(128*\off + 112)*2(%rsi)

.endm

.macro shuffle_to2 off

vmovdqa	  (128*\off + 0)*2(%rdx),%ymm4
vmovdqa	  (128*\off + 16)*2(%rdx),%ymm5
vmovdqa	  (128*\off + 32)*2(%rdx),%ymm6
vmovdqa	  (128*\off + 48)*2(%rdx),%ymm7
vmovdqa	  (128*\off + 64)*2(%rdx),%ymm8
vmovdqa	  (128*\off + 80)*2(%rdx),%ymm9
vmovdqa	  (128*\off + 96)*2(%rdx),%ymm10
vmovdqa	  (128*\off + 112)*2(%rdx),%ymm11

shuffle8	4,8,3,8
shuffle8	5,9,4,9
shuffle8	6,10,5,10
shuffle8	7,11,6,11

shuffle4	3,5,7,5
shuffle4	8,10,3,10
shuffle4	4,6,8,6
shuffle4	9,11,4,11

shuffle2    7,8
shuffle2    5,6
shuffle2    3,4
shuffle2    10,11

shuffle1   7,3
shuffle1   8,4
shuffle1   5,10
shuffle1   6,11


vmovdqa		%ymm7,(128*\off + 0)*2(%rdx)
vmovdqa		%ymm3,(128*\off + 16)*2(%rdx)
vmovdqa		%ymm8,(128*\off + 32)*2(%rdx)
vmovdqa		%ymm4,(128*\off + 48)*2(%rdx)
vmovdqa		%ymm5,(128*\off + 64)*2(%rdx)
vmovdqa		%ymm10,(128*\off + 80)*2(%rdx)
vmovdqa		%ymm6,(128*\off + 96)*2(%rdx)
vmovdqa		%ymm11,(128*\off + 112)*2(%rdx)

.endm

.macro shuffle_back1 off

vmovdqa	  (128*\off + 0)*2(%rdi),%ymm7
vmovdqa	  (128*\off + 16)*2(%rdi),%ymm3
vmovdqa	  (128*\off + 32)*2(%rdi),%ymm8
vmovdqa	  (128*\off + 48)*2(%rdi),%ymm4
vmovdqa	  (128*\off + 64)*2(%rdi),%ymm5
vmovdqa	  (128*\off + 80)*2(%rdi),%ymm10
vmovdqa	  (128*\off + 96)*2(%rdi),%ymm6
vmovdqa	  (128*\off + 112)*2(%rdi),%ymm11

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

.endm


.macro shuffle_back2 off

vmovdqa	  (128*\off + 0)*2(%rdx),%ymm7
vmovdqa	  (128*\off + 16)*2(%rdx),%ymm3
vmovdqa	  (128*\off + 32)*2(%rdx),%ymm8
vmovdqa	  (128*\off + 48)*2(%rdx),%ymm4
vmovdqa	  (128*\off + 64)*2(%rdx),%ymm5
vmovdqa	  (128*\off + 80)*2(%rdx),%ymm10
vmovdqa	  (128*\off + 96)*2(%rdx),%ymm6
vmovdqa	  (128*\off + 112)*2(%rdx),%ymm11

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

vmovdqa		%ymm4,(128*\off + 0)*2(%rdx)
vmovdqa		%ymm5,(128*\off + 16)*2(%rdx)
vmovdqa		%ymm6,(128*\off + 32)*2(%rdx)
vmovdqa		%ymm7,(128*\off + 48)*2(%rdx)
vmovdqa		%ymm8,(128*\off + 64)*2(%rdx)
vmovdqa		%ymm9,(128*\off + 80)*2(%rdx)
vmovdqa		%ymm10,(128*\off + 96)*2(%rdx)
vmovdqa		%ymm11,(128*\off + 112)*2(%rdx)

.endm


.macro fqmul l,h,r
vpmulhw   %ymm\l,%ymm\h,%ymm12
vpmullw   %ymm\l,%ymm\h,%ymm13
vpmullw %ymm0,%ymm13,%ymm14
vpmulhw %ymm1,%ymm14,%ymm14
vpsubw  %ymm14,%ymm12,%ymm\r
.endm

.macro barret a
vpmulhw     %ymm2,%ymm\a,%ymm13
vpaddw      %ymm15,%ymm13,%ymm13
vpsraw      $10,%ymm13,%ymm13
vpmullw     %ymm1,%ymm13,%ymm13
vpsubw      %ymm13,%ymm\a,%ymm\a
.endm


.macro calc_d l,r
//al + ar
vmovdqa    (32*\l)(%r8),%ymm4
vmovdqa    (32*\r)(%r8),%ymm5
vpaddw     %ymm4,%ymm5,%ymm6
//bl + br
vmovdqa    (32*\l+128)(%r8),%ymm4
vmovdqa    (32*\r+128)(%r8),%ymm5
vpaddw     %ymm4,%ymm5,%ymm7
//dl + dr
vmovdqa     (32*\l+256)(%r8),%ymm4
vmovdqa     (32*\r+256)(%r8),%ymm5
vpaddw     %ymm4,%ymm5,%ymm10
// (al + ar)(bl + br) - (dl + dr)
fqmul      6,7,4
vpsubw     %ymm10,%ymm4,%ymm4
.endm

.macro basemul off
//a[0]
vmovdqa   (64*\off + 0)*2(%rsi),%ymm4
//a[1]
vmovdqa   (64*\off + 16)*2(%rsi),%ymm5
vmovdqa   %ymm4,0(%r8)
vmovdqa   %ymm5,32(%r8)
//b[0]
vmovdqa   (64*\off + 0)*2(%rdx),%ymm6
//b[1]
vmovdqa   (64*\off + 16)*2(%rdx),%ymm7
vmovdqa   %ymm6,128(%r8)
vmovdqa   %ymm7,160(%r8)
//d0
fqmul     4,6,3
vmovdqa   %ymm3,256(%r8)
//d1
fqmul     5,7,3
vmovdqa   %ymm3,288(%r8)
//a[2]
vmovdqa   (64*\off + 32)*2(%rsi),%ymm4
//a[3]
vmovdqa   (64*\off + 48)*2(%rsi),%ymm5
vmovdqa   %ymm4,64(%r8)
vmovdqa   %ymm5,96(%r8)
//b[2]
vmovdqa   (64*\off + 32)*2(%rdx),%ymm6
//b[3]
vmovdqa   (64*\off + 48)*2(%rdx),%ymm7
vmovdqa   %ymm6,192(%r8)
vmovdqa   %ymm7,224(%r8)
//d2
fqmul     4,6,3
vmovdqa   %ymm3,320(%r8)
//d3
fqmul     5,7,3
vmovdqa   %ymm3,352(%r8)

//c0:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 1,3
vpaddw   %ymm4,%ymm3,%ymm3

vmovdqa  (256+32*2)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3
vmovdqa   (_ZETAS_BASEMUL+16*\off)*2(%rcx),%ymm8
fqmul     3,8,3
vmovdqa  (256+32*0)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3
barret 3
vmovdqa  %ymm3,(64*\off+0)*2(%rdi)
//c1:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 2,3
vpaddw   %ymm4,%ymm3,%ymm3
fqmul     3,8,3
calc_d 0,1
vpaddw   %ymm4,%ymm3,%ymm3
barret 3
vmovdqa  %ymm3,(64*\off+16)*2(%rdi)
//c2:
vpxor     %ymm3,%ymm3,%ymm3
vmovdqa  (256+32*3)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3
fqmul     3,8,3
vmovdqa  (256+32*1)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3
calc_d 0,2
vpaddw   %ymm4,%ymm3,%ymm3
barret 3
vmovdqa  %ymm3,(64*\off+32)*2(%rdi)
//c3:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 0,3
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 1,2
vpaddw   %ymm4,%ymm3,%ymm3
barret 3
vmovdqa  %ymm3,(64*\off+48)*2(%rdi)
.endm

.text
.global  cdecl(basemul_avx)
cdecl(basemul_avx):


shuffle_to1 0
shuffle_to1 1
shuffle_to1 2
shuffle_to1 3
shuffle_to2 0
shuffle_to2 1
shuffle_to2 2
shuffle_to2 3

mov		%rsp,%r10
mov		%rsp,%rax
and		$31,%rax
sub		%rax,%rsp
sub		$1024,%rsp
mov     %rsp,%r8
//qinv
vmovdqa   _16XQINV*2(%rcx),%ymm0
//q
vmovdqa   _16XQ*2(%rcx),%ymm1
//v
vmovdqa   _16XV*2(%rcx),%ymm2
vmovdqa   _16XBARRETCONST*2(%rcx),%ymm15

basemul 0
basemul 1
basemul 2
basemul 3
basemul 4
basemul 5
basemul 6
basemul 7

shuffle_back1 0
shuffle_back1 1
shuffle_back1 2
shuffle_back1 3

shuffle_back2 0
shuffle_back2 1
shuffle_back2 2
shuffle_back2 3

mov		%r10,%rsp
ret