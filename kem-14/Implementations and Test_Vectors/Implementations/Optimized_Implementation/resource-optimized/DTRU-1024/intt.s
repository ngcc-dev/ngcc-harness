#include "consts.h"
.include "shuffle.inc"

.macro butterflya l,r,z
vpsrld $0,%ymm\l,%ymm12
//t+ajl
vpaddw %ymm\r,%ymm12,%ymm\l
//t-ajl
vpsubw %ymm\r,%ymm12,%ymm\r
//**********************
//a*zeta*qinv
vpmullw %ymm0,%ymm\r,%ymm12
//a*zeta
vpmulhw %ymm\z,%ymm\r,%ymm13
//a*zeta*qinv*q
vpmulhw %ymm1,%ymm12,%ymm12
vpsubw  %ymm12,%ymm13,%ymm\r
.endm


.macro butterflyb l,r,z
vpsrld $0,%ymm\l,%ymm12
//t+ajl
vpaddw      %ymm\r,%ymm12,%ymm\l
vpmulhw     %ymm2,%ymm\l,%ymm14
vpaddw      %ymm15,%ymm14,%ymm14
vpsraw      $10,%ymm14,%ymm14
vpmullw     %ymm1,%ymm14,%ymm14
vpsubw      %ymm14,%ymm\l,%ymm\l
//t-ajl
vpsubw      %ymm\r,%ymm12,%ymm\r
vpmulhw     %ymm2,%ymm\r,%ymm14
vpaddw      %ymm15,%ymm14,%ymm14
vpsraw      $10,%ymm14,%ymm14
vpmullw     %ymm1,%ymm14,%ymm14
vpsubw      %ymm14,%ymm\r,%ymm\r


//**********************
//a*zeta*qinv
vpmullw %ymm0,%ymm\r,%ymm12
//a*zeta
vpmulhw %ymm\z,%ymm\r,%ymm13
//a*zeta*qinv*q
vpmulhw %ymm1,%ymm12,%ymm12
vpsubw  %ymm12,%ymm13,%ymm\r

.endm
//*************************************************************************
.macro levelsix l0,r0 
//t = a[j]-a[j+512]
vpsubw %ymm\r0,%ymm\l0,%ymm12
//t*zeta*qinv
vpmullw %ymm0,%ymm12,%ymm13
//t*zeta
vpmulhw %ymm3,%ymm12,%ymm14
//t*zeta*qinv*q
vpmulhw %ymm1,%ymm13,%ymm13
vpsubw  %ymm13,%ymm14,%ymm12

//a[j]=a[j]+a[j+512]
vpaddw %ymm\r0,%ymm\l0,%ymm\l0
//a[j]=a[j]-t
vpsubw %ymm12,%ymm\l0,%ymm\l0
//a[j]=f*a[j]
vmovdqa 256(%rdx),%ymm3
vmovdqa 256(%r8),%ymm0
//a*f*qinv
vpmullw %ymm0,%ymm\l0,%ymm13
//a*f
vpmulhw %ymm3,%ymm\l0,%ymm14
//a*f*qinv*q
vpmulhw %ymm1,%ymm13,%ymm13
vpsubw  %ymm13,%ymm14,%ymm\l0
//a[j+256]=f2*t
//f
vmovdqa 288(%rdx),%ymm3
vmovdqa 288(%r8),%ymm0
//t*f2*qinv
vpmullw %ymm0,%ymm12,%ymm13
//t*f2
vpmulhw %ymm3,%ymm12,%ymm14
//t*f2*qinv*q
vpmulhw %ymm1,%ymm13,%ymm13
vpsubw  %ymm13,%ymm14,%ymm\r0
.endm

.macro barret l
vpmulhw     %ymm2,%ymm\l,%ymm12
vpsraw      $10,%ymm12,%ymm12
vpmullw     %ymm1,%ymm12,%ymm12
vpsubw      %ymm12,%ymm\l,%ymm\l
.endm

//*******************************************************************
.text

.global invntt_avx
invntt_avx:

vmovdqa		32(%rsi),%ymm1
vmovdqa		64(%rsi),%ymm2
vmovdqa		20064(%rsi),%ymm15
mov		%rsi,%rcx
lea		2400(%rsi),%rdx
lea		12992(%rsi),%r8
lea		12992(%rsi),%r9
#levels0t3

vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11

shuffle1   4,5
shuffle1   6,7
shuffle1   8,9
shuffle1   10,11

shuffle2   4,6
shuffle2   8,10
shuffle2   5,7
shuffle2   9,11

shuffle4	4,8,3,8
shuffle4	5,9,4,9
shuffle4	6,10,5,10
shuffle4	7,11,6,11

//level0: //8
vmovdqa     (%rdx),%ymm7
vmovdqa     (%r8),%ymm0
butterflyb	3,4,7
vmovdqa     32(%rdx),%ymm7
vmovdqa     32(%r8),%ymm0
butterflyb	5,6,7
vmovdqa     64(%rdx),%ymm7
vmovdqa     64(%r8),%ymm0
butterflyb	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa     96(%r8),%ymm0
butterflyb	10,11,7

//level1://16

vmovdqa     128(%rdx),%ymm7
vmovdqa     128(%r8),%ymm0
butterflya	3,5,7
butterflya	4,6,7
vmovdqa     160(%rdx),%ymm7
vmovdqa     160(%r8),%ymm0
butterflya	8,10,7
butterflya	9,11,7

//level3://32
vmovdqa     192(%rdx),%ymm7
vmovdqa     192(%r8),%ymm0
butterflya	3,8,7
butterflya	4,9,7
butterflya	5,10,7
butterflya	6,11,7


shuffle8	3,4,7,4
shuffle8	5,6,3,6
shuffle8	8,9,5,9
shuffle8	10,11,8,11

//level4://64
vmovdqa     224(%rdx),%ymm10
vmovdqa     224(%r8),%ymm0
butterflyb	7,4,10
butterflyb	3,6,10
butterflyb	5,9,10
butterflyb	8,11,10

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm8,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
add		$256,%rdi
add		$256,%rdx
add		$256,%r8
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11

shuffle1   4,5
shuffle1   6,7
shuffle1   8,9
shuffle1   10,11

shuffle2   4,6
shuffle2   8,10
shuffle2   5,7
shuffle2   9,11

shuffle4	4,8,3,8
shuffle4	5,9,4,9
shuffle4	6,10,5,10
shuffle4	7,11,6,11

//level0: //8
vmovdqa     (%rdx),%ymm7
vmovdqa     (%r8),%ymm0
butterflyb	3,4,7
vmovdqa     32(%rdx),%ymm7
vmovdqa     32(%r8),%ymm0
butterflyb	5,6,7
vmovdqa     64(%rdx),%ymm7
vmovdqa     64(%r8),%ymm0
butterflyb	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa     96(%r8),%ymm0
butterflyb	10,11,7

//level1://16

vmovdqa     128(%rdx),%ymm7
vmovdqa     128(%r8),%ymm0
butterflya	3,5,7
butterflya	4,6,7

vmovdqa     160(%rdx),%ymm7
vmovdqa     160(%r8),%ymm0
butterflya	8,10,7
butterflya	9,11,7

//level3://32
vmovdqa     192(%rdx),%ymm7
vmovdqa     192(%r8),%ymm0
butterflya	3,8,7
butterflya	4,9,7
butterflya	5,10,7
butterflya	6,11,7


shuffle8	3,4,7,4
shuffle8	5,6,3,6
shuffle8	8,9,5,9
shuffle8	10,11,8,11

//level4://64
vmovdqa     224(%rdx),%ymm10
vmovdqa     224(%r8),%ymm0
barret 7
butterflyb	7,4,10
butterflyb	3,6,10
butterflyb	5,9,10
butterflyb	8,11,10

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm8,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
add		$256,%rdi
add		$256,%rdx
add		$256,%r8
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11

shuffle1   4,5
shuffle1   6,7
shuffle1   8,9
shuffle1   10,11

shuffle2   4,6
shuffle2   8,10
shuffle2   5,7
shuffle2   9,11

shuffle4	4,8,3,8
shuffle4	5,9,4,9
shuffle4	6,10,5,10
shuffle4	7,11,6,11

//level0: //8
vmovdqa     (%rdx),%ymm7
vmovdqa     (%r8),%ymm0
butterflyb	3,4,7
vmovdqa     32(%rdx),%ymm7
vmovdqa     32(%r8),%ymm0
butterflyb	5,6,7
vmovdqa     64(%rdx),%ymm7
vmovdqa     64(%r8),%ymm0
butterflyb	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa     96(%r8),%ymm0
butterflyb	10,11,7

//level1://16

vmovdqa     128(%rdx),%ymm7
vmovdqa     128(%r8),%ymm0
butterflya	3,5,7
butterflya	4,6,7

vmovdqa     160(%rdx),%ymm7
vmovdqa     160(%r8),%ymm0
butterflya	8,10,7
butterflya	9,11,7

//level3://32
vmovdqa     192(%rdx),%ymm7
vmovdqa     192(%r8),%ymm0
butterflya	3,8,7
butterflya	4,9,7
butterflya	5,10,7
butterflya	6,11,7


shuffle8	3,4,7,4
shuffle8	5,6,3,6
shuffle8	8,9,5,9
shuffle8	10,11,8,11

//level4://64
vmovdqa     224(%rdx),%ymm10
vmovdqa     224(%r8),%ymm0
butterflyb	7,4,10
butterflyb	3,6,10
butterflyb	5,9,10
butterflyb	8,11,10

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm8,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
add		$256,%rdi
add		$256,%rdx
add		$256,%r8
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11

shuffle1   4,5
shuffle1   6,7
shuffle1   8,9
shuffle1   10,11

shuffle2   4,6
shuffle2   8,10
shuffle2   5,7
shuffle2   9,11

shuffle4	4,8,3,8
shuffle4	5,9,4,9
shuffle4	6,10,5,10
shuffle4	7,11,6,11

//level0: //8
vmovdqa     (%rdx),%ymm7
vmovdqa     (%r8),%ymm0
butterflyb	3,4,7
vmovdqa     32(%rdx),%ymm7
vmovdqa     32(%r8),%ymm0
butterflyb	5,6,7
vmovdqa     64(%rdx),%ymm7
vmovdqa     64(%r8),%ymm0
butterflyb	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa     96(%r8),%ymm0
butterflyb	10,11,7

//level1://16

vmovdqa     128(%rdx),%ymm7
vmovdqa     128(%r8),%ymm0
butterflya	3,5,7
butterflya	4,6,7

vmovdqa     160(%rdx),%ymm7
vmovdqa     160(%r8),%ymm0
butterflya	8,10,7
butterflya	9,11,7

//level3://32
vmovdqa     192(%rdx),%ymm7
vmovdqa     192(%r8),%ymm0
butterflya	3,8,7
butterflya	4,9,7
butterflya	5,10,7
butterflya	6,11,7


shuffle8	3,4,7,4
shuffle8	5,6,3,6
shuffle8	8,9,5,9
shuffle8	10,11,8,11

//level4://64
vmovdqa     224(%rdx),%ymm10
vmovdqa     224(%r8),%ymm0
barret 7
butterflyb	7,4,10
butterflyb	3,6,10
butterflyb	5,9,10
butterflyb	8,11,10

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm8,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
add		$256,%rdi
add		$256,%rdx
add		$256,%r8
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11

shuffle1   4,5
shuffle1   6,7
shuffle1   8,9
shuffle1   10,11

shuffle2   4,6
shuffle2   8,10
shuffle2   5,7
shuffle2   9,11

shuffle4	4,8,3,8
shuffle4	5,9,4,9
shuffle4	6,10,5,10
shuffle4	7,11,6,11

//level0: //8
vmovdqa     (%rdx),%ymm7
vmovdqa     (%r8),%ymm0
butterflyb	3,4,7
vmovdqa     32(%rdx),%ymm7
vmovdqa     32(%r8),%ymm0
butterflyb	5,6,7
vmovdqa     64(%rdx),%ymm7
vmovdqa     64(%r8),%ymm0
butterflyb	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa     96(%r8),%ymm0
butterflyb	10,11,7

//level1://16

vmovdqa     128(%rdx),%ymm7
vmovdqa     128(%r8),%ymm0
butterflya	3,5,7
butterflya	4,6,7

vmovdqa     160(%rdx),%ymm7
vmovdqa     160(%r8),%ymm0
butterflya	8,10,7
butterflya	9,11,7

//level3://32
vmovdqa     192(%rdx),%ymm7
vmovdqa     192(%r8),%ymm0
butterflya	3,8,7
butterflya	4,9,7
butterflya	5,10,7
butterflya	6,11,7


shuffle8	3,4,7,4
shuffle8	5,6,3,6
shuffle8	8,9,5,9
shuffle8	10,11,8,11

//level4://64
vmovdqa     224(%rdx),%ymm10
vmovdqa     224(%r8),%ymm0
butterflyb	7,4,10
butterflyb	3,6,10
butterflyb	5,9,10
butterflyb	8,11,10

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm8,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
add		$256,%rdi
add		$256,%rdx
add		$256,%r8
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11

shuffle1   4,5
shuffle1   6,7
shuffle1   8,9
shuffle1   10,11

shuffle2   4,6
shuffle2   8,10
shuffle2   5,7
shuffle2   9,11

shuffle4	4,8,3,8
shuffle4	5,9,4,9
shuffle4	6,10,5,10
shuffle4	7,11,6,11

//level0: //8
vmovdqa     (%rdx),%ymm7
vmovdqa     (%r8),%ymm0
butterflyb	3,4,7
vmovdqa     32(%rdx),%ymm7
vmovdqa     32(%r8),%ymm0
butterflyb	5,6,7
vmovdqa     64(%rdx),%ymm7
vmovdqa     64(%r8),%ymm0
butterflyb	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa     96(%r8),%ymm0
butterflyb	10,11,7

//level1://16

vmovdqa     128(%rdx),%ymm7
vmovdqa     128(%r8),%ymm0
butterflya	3,5,7
butterflya	4,6,7

vmovdqa     160(%rdx),%ymm7
vmovdqa     160(%r8),%ymm0
butterflya	8,10,7
butterflya	9,11,7

//level3://32
vmovdqa     192(%rdx),%ymm7
vmovdqa     192(%r8),%ymm0
butterflya	3,8,7
butterflya	4,9,7
butterflya	5,10,7
butterflya	6,11,7


shuffle8	3,4,7,4
shuffle8	5,6,3,6
shuffle8	8,9,5,9
shuffle8	10,11,8,11

//level4://64
vmovdqa     224(%rdx),%ymm10
vmovdqa     224(%r8),%ymm0
barret 7
butterflyb	7,4,10
butterflyb	3,6,10
butterflyb	5,9,10
butterflyb	8,11,10

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm8,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
add		$256,%rdi
add		$256,%rdx
add		$256,%r8
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11

shuffle1   4,5
shuffle1   6,7
shuffle1   8,9
shuffle1   10,11

shuffle2   4,6
shuffle2   8,10
shuffle2   5,7
shuffle2   9,11

shuffle4	4,8,3,8
shuffle4	5,9,4,9
shuffle4	6,10,5,10
shuffle4	7,11,6,11

//level0: //8
vmovdqa     (%rdx),%ymm7
vmovdqa     (%r8),%ymm0
butterflyb	3,4,7
vmovdqa     32(%rdx),%ymm7
vmovdqa     32(%r8),%ymm0
butterflyb	5,6,7
vmovdqa     64(%rdx),%ymm7
vmovdqa     64(%r8),%ymm0
butterflyb	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa     96(%r8),%ymm0
butterflyb	10,11,7

//level1://16

vmovdqa     128(%rdx),%ymm7
vmovdqa     128(%r8),%ymm0
butterflya	3,5,7
butterflya	4,6,7

vmovdqa     160(%rdx),%ymm7
vmovdqa     160(%r8),%ymm0
butterflya	8,10,7
butterflya	9,11,7

//level3://32
vmovdqa     192(%rdx),%ymm7
vmovdqa     192(%r8),%ymm0
butterflya	3,8,7
butterflya	4,9,7
butterflya	5,10,7
butterflya	6,11,7


shuffle8	3,4,7,4
shuffle8	5,6,3,6
shuffle8	8,9,5,9
shuffle8	10,11,8,11

//level4://64
vmovdqa     224(%rdx),%ymm10
vmovdqa     224(%r8),%ymm0
butterflyb	7,4,10
butterflyb	3,6,10
butterflyb	5,9,10
butterflyb	8,11,10

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm8,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm11,224(%rdi)
add		$256,%rdi
add		$256,%rdx
add		$256,%r8
vmovdqa		(%rdi),%ymm4
vmovdqa		32(%rdi),%ymm5
vmovdqa		64(%rdi),%ymm6
vmovdqa		96(%rdi),%ymm7
vmovdqa		128(%rdi),%ymm8
vmovdqa		160(%rdi),%ymm9
vmovdqa		192(%rdi),%ymm10
vmovdqa		224(%rdi),%ymm11

shuffle1   4,5
shuffle1   6,7
shuffle1   8,9
shuffle1   10,11

shuffle2   4,6
shuffle2   8,10
shuffle2   5,7
shuffle2   9,11

shuffle4	4,8,3,8
shuffle4	5,9,4,9
shuffle4	6,10,5,10
shuffle4	7,11,6,11

//level0: //8
vmovdqa     (%rdx),%ymm7
vmovdqa     (%r8),%ymm0
butterflyb	3,4,7
vmovdqa     32(%rdx),%ymm7
vmovdqa     32(%r8),%ymm0
butterflyb	5,6,7
vmovdqa     64(%rdx),%ymm7
vmovdqa     64(%r8),%ymm0
butterflyb	8,9,7
vmovdqa     96(%rdx),%ymm7
vmovdqa     96(%r8),%ymm0
butterflyb	10,11,7

//level1://16

vmovdqa     128(%rdx),%ymm7
vmovdqa     128(%r8),%ymm0
butterflya	3,5,7
butterflya	4,6,7

vmovdqa     160(%rdx),%ymm7
vmovdqa     160(%r8),%ymm0
butterflya	8,10,7
butterflya	9,11,7

//level3://32
vmovdqa     192(%rdx),%ymm7
vmovdqa     192(%r8),%ymm0
butterflya	3,8,7
butterflya	4,9,7
butterflya	5,10,7
butterflya	6,11,7


shuffle8	3,4,7,4
shuffle8	5,6,3,6
shuffle8	8,9,5,9
shuffle8	10,11,8,11

//level4://64
vmovdqa     224(%rdx),%ymm10
vmovdqa     224(%r8),%ymm0
barret 7
butterflyb	7,4,10
butterflyb	3,6,10
butterflyb	5,9,10
butterflyb	8,11,10

vmovdqa		%ymm7,(%rdi)
vmovdqa		%ymm3,32(%rdi)
vmovdqa		%ymm5,64(%rdi)
vmovdqa		%ymm8,96(%rdi)
vmovdqa		%ymm4,128(%rdi)
vmovdqa		%ymm6,160(%rdi)
vmovdqa		%ymm9,192(%rdi)
vmovdqa		%ymm11,224(%rdi)

sub		$1792,%rdi
lea		4448(%rcx),%rdx
lea		2048(%r9),%r8

vmovdqa		(%rdi),%ymm4
vmovdqa		256(%rdi),%ymm5
vmovdqa		512(%rdi),%ymm6
vmovdqa		768(%rdi),%ymm7
vmovdqa		1024(%rdi),%ymm8
vmovdqa		1280(%rdi),%ymm9
vmovdqa		1536(%rdi),%ymm10
vmovdqa		1792(%rdi),%ymm11

//level5://128
vmovdqa     (%rdx),%ymm3
vmovdqa     (%r8),%ymm0
barret 4
butterflya   4,5,3
vmovdqa     32(%rdx),%ymm3
vmovdqa     32(%r8),%ymm0
barret 6
butterflya   6,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa     64(%r8),%ymm0
barret 8
butterflya   8,9,3
vmovdqa     96(%rdx),%ymm3
vmovdqa     96(%r8),%ymm0
barret 10
butterflya   10,11,3

//level6://256
vmovdqa     128(%rdx),%ymm3
vmovdqa     128(%r8),%ymm0
butterflya   4,6,3
butterflya   5,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa     160(%r8),%ymm0
butterflya   8,10,3
butterflya   9,11,3

//level7://512

vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 4,8
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 5,9
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 6,10
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 7,11

vmovdqa         %ymm4,(%rdi)
vmovdqa         %ymm5,256(%rdi)
vmovdqa         %ymm6,512(%rdi)
vmovdqa         %ymm7,768(%rdi)
vmovdqa         %ymm8,1024(%rdi)
vmovdqa         %ymm9,1280(%rdi)
vmovdqa         %ymm10,1536(%rdi)
vmovdqa         %ymm11,1792(%rdi)
add		$32,%rdi
add		$32,%eax
vmovdqa		(%rdi),%ymm4
vmovdqa		256(%rdi),%ymm5
vmovdqa		512(%rdi),%ymm6
vmovdqa		768(%rdi),%ymm7
vmovdqa		1024(%rdi),%ymm8
vmovdqa		1280(%rdi),%ymm9
vmovdqa		1536(%rdi),%ymm10
vmovdqa		1792(%rdi),%ymm11

//level5://128
vmovdqa     (%rdx),%ymm3
vmovdqa     (%r8),%ymm0
butterflya   4,5,3
vmovdqa     32(%rdx),%ymm3
vmovdqa     32(%r8),%ymm0
barret 6
butterflya   6,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa     64(%r8),%ymm0
butterflya   8,9,3
vmovdqa     96(%rdx),%ymm3
vmovdqa     96(%r8),%ymm0
barret 10
butterflya   10,11,3

//level6://256
vmovdqa     128(%rdx),%ymm3
vmovdqa     128(%r8),%ymm0
barret 4
butterflya   4,6,3
butterflya   5,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa     160(%r8),%ymm0
barret 8
butterflya   8,10,3
butterflya   9,11,3

//level7://512

vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 4,8
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 5,9
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 6,10
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 7,11

vmovdqa         %ymm4,(%rdi)
vmovdqa         %ymm5,256(%rdi)
vmovdqa         %ymm6,512(%rdi)
vmovdqa         %ymm7,768(%rdi)
vmovdqa         %ymm8,1024(%rdi)
vmovdqa         %ymm9,1280(%rdi)
vmovdqa         %ymm10,1536(%rdi)
vmovdqa         %ymm11,1792(%rdi)
add		$32,%rdi
add		$32,%eax

vmovdqa		(%rdi),%ymm4
vmovdqa		256(%rdi),%ymm5
vmovdqa		512(%rdi),%ymm6
vmovdqa		768(%rdi),%ymm7
vmovdqa		1024(%rdi),%ymm8
vmovdqa		1280(%rdi),%ymm9
vmovdqa		1536(%rdi),%ymm10
vmovdqa		1792(%rdi),%ymm11

//level5://128
vmovdqa     (%rdx),%ymm3
vmovdqa     (%r8),%ymm0
butterflya   4,5,3
vmovdqa     32(%rdx),%ymm3
vmovdqa     32(%r8),%ymm0
butterflya   6,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa     64(%r8),%ymm0
butterflya   8,9,3
vmovdqa     96(%rdx),%ymm3
vmovdqa     96(%r8),%ymm0
butterflya   10,11,3

//level6://256
vmovdqa     128(%rdx),%ymm3
vmovdqa     128(%r8),%ymm0
barret 4
butterflya   4,6,3
butterflya   5,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa     160(%r8),%ymm0
barret 8
butterflya   8,10,3
butterflya   9,11,3

//level7://512

vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 4,8
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 5,9
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 6,10
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 7,11

vmovdqa         %ymm4,(%rdi)
vmovdqa         %ymm5,256(%rdi)
vmovdqa         %ymm6,512(%rdi)
vmovdqa         %ymm7,768(%rdi)
vmovdqa         %ymm8,1024(%rdi)
vmovdqa         %ymm9,1280(%rdi)
vmovdqa         %ymm10,1536(%rdi)
vmovdqa         %ymm11,1792(%rdi)
add		$32,%rdi
add		$32,%eax

vmovdqa		(%rdi),%ymm4
vmovdqa		256(%rdi),%ymm5
vmovdqa		512(%rdi),%ymm6
vmovdqa		768(%rdi),%ymm7
vmovdqa		1024(%rdi),%ymm8
vmovdqa		1280(%rdi),%ymm9
vmovdqa		1536(%rdi),%ymm10
vmovdqa		1792(%rdi),%ymm11

//level5://128
vmovdqa     (%rdx),%ymm3
vmovdqa     (%r8),%ymm0
butterflya   4,5,3
vmovdqa     32(%rdx),%ymm3
vmovdqa     32(%r8),%ymm0
butterflya   6,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa     64(%r8),%ymm0
butterflya   8,9,3
vmovdqa     96(%rdx),%ymm3
vmovdqa     96(%r8),%ymm0
butterflya   10,11,3

//level6://256
vmovdqa     128(%rdx),%ymm3
vmovdqa     128(%r8),%ymm0
barret 4
butterflya   4,6,3
butterflya   5,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa     160(%r8),%ymm0
butterflya   8,10,3
butterflya   9,11,3

//level7://512

vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 4,8
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 5,9
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 6,10
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 7,11

vmovdqa         %ymm4,(%rdi)
vmovdqa         %ymm5,256(%rdi)
vmovdqa         %ymm6,512(%rdi)
vmovdqa         %ymm7,768(%rdi)
vmovdqa         %ymm8,1024(%rdi)
vmovdqa         %ymm9,1280(%rdi)
vmovdqa         %ymm10,1536(%rdi)
vmovdqa         %ymm11,1792(%rdi)
add		$32,%rdi
add		$32,%eax

vmovdqa		(%rdi),%ymm4
vmovdqa		256(%rdi),%ymm5
vmovdqa		512(%rdi),%ymm6
vmovdqa		768(%rdi),%ymm7
vmovdqa		1024(%rdi),%ymm8
vmovdqa		1280(%rdi),%ymm9
vmovdqa		1536(%rdi),%ymm10
vmovdqa		1792(%rdi),%ymm11

//level5://128
vmovdqa     (%rdx),%ymm3
vmovdqa     (%r8),%ymm0
butterflya   4,5,3
vmovdqa     32(%rdx),%ymm3
vmovdqa     32(%r8),%ymm0
butterflya   6,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa     64(%r8),%ymm0
butterflya   8,9,3
vmovdqa     96(%rdx),%ymm3
vmovdqa     96(%r8),%ymm0
butterflya   10,11,3

//level6://256
vmovdqa     128(%rdx),%ymm3
vmovdqa     128(%r8),%ymm0
butterflya   4,6,3
butterflya   5,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa     160(%r8),%ymm0
butterflya   8,10,3
butterflya   9,11,3

//level7://512

vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 4,8
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 5,9
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 6,10
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 7,11

vmovdqa         %ymm4,(%rdi)
vmovdqa         %ymm5,256(%rdi)
vmovdqa         %ymm6,512(%rdi)
vmovdqa         %ymm7,768(%rdi)
vmovdqa         %ymm8,1024(%rdi)
vmovdqa         %ymm9,1280(%rdi)
vmovdqa         %ymm10,1536(%rdi)
vmovdqa         %ymm11,1792(%rdi)
add		$32,%rdi
add		$32,%eax

vmovdqa		(%rdi),%ymm4
vmovdqa		256(%rdi),%ymm5
vmovdqa		512(%rdi),%ymm6
vmovdqa		768(%rdi),%ymm7
vmovdqa		1024(%rdi),%ymm8
vmovdqa		1280(%rdi),%ymm9
vmovdqa		1536(%rdi),%ymm10
vmovdqa		1792(%rdi),%ymm11

//level5://128
vmovdqa     (%rdx),%ymm3
vmovdqa     (%r8),%ymm0
butterflya   4,5,3
vmovdqa     32(%rdx),%ymm3
vmovdqa     32(%r8),%ymm0
butterflya   6,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa     64(%r8),%ymm0
butterflya   8,9,3
vmovdqa     96(%rdx),%ymm3
vmovdqa     96(%r8),%ymm0
butterflya   10,11,3

//level6://256
vmovdqa     128(%rdx),%ymm3
vmovdqa     128(%r8),%ymm0
butterflya   4,6,3
butterflya   5,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa     160(%r8),%ymm0
butterflya   8,10,3
butterflya   9,11,3

//level7://512

vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 4,8
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 5,9
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 6,10
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 7,11

vmovdqa         %ymm4,(%rdi)
vmovdqa         %ymm5,256(%rdi)
vmovdqa         %ymm6,512(%rdi)
vmovdqa         %ymm7,768(%rdi)
vmovdqa         %ymm8,1024(%rdi)
vmovdqa         %ymm9,1280(%rdi)
vmovdqa         %ymm10,1536(%rdi)
vmovdqa         %ymm11,1792(%rdi)
add		$32,%rdi
add		$32,%eax

vmovdqa		(%rdi),%ymm4
vmovdqa		256(%rdi),%ymm5
vmovdqa		512(%rdi),%ymm6
vmovdqa		768(%rdi),%ymm7
vmovdqa		1024(%rdi),%ymm8
vmovdqa		1280(%rdi),%ymm9
vmovdqa		1536(%rdi),%ymm10
vmovdqa		1792(%rdi),%ymm11

//level5://128
vmovdqa     (%rdx),%ymm3
vmovdqa     (%r8),%ymm0
butterflya   4,5,3
vmovdqa     32(%rdx),%ymm3
vmovdqa     32(%r8),%ymm0
butterflya   6,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa     64(%r8),%ymm0
butterflya   8,9,3
vmovdqa     96(%rdx),%ymm3
vmovdqa     96(%r8),%ymm0
butterflya   10,11,3

//level6://256
vmovdqa     128(%rdx),%ymm3
vmovdqa     128(%r8),%ymm0
butterflya   4,6,3
butterflya   5,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa     160(%r8),%ymm0
butterflya   8,10,3
butterflya   9,11,3

//level7://512

vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 4,8
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 5,9
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 6,10
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 7,11

vmovdqa         %ymm4,(%rdi)
vmovdqa         %ymm5,256(%rdi)
vmovdqa         %ymm6,512(%rdi)
vmovdqa         %ymm7,768(%rdi)
vmovdqa         %ymm8,1024(%rdi)
vmovdqa         %ymm9,1280(%rdi)
vmovdqa         %ymm10,1536(%rdi)
vmovdqa         %ymm11,1792(%rdi)
add		$32,%rdi
add		$32,%eax

vmovdqa		(%rdi),%ymm4
vmovdqa		256(%rdi),%ymm5
vmovdqa		512(%rdi),%ymm6
vmovdqa		768(%rdi),%ymm7
vmovdqa		1024(%rdi),%ymm8
vmovdqa		1280(%rdi),%ymm9
vmovdqa		1536(%rdi),%ymm10
vmovdqa		1792(%rdi),%ymm11

//level5://128
vmovdqa     (%rdx),%ymm3
vmovdqa     (%r8),%ymm0
butterflya   4,5,3
vmovdqa     32(%rdx),%ymm3
vmovdqa     32(%r8),%ymm0
butterflya   6,7,3
vmovdqa     64(%rdx),%ymm3
vmovdqa     64(%r8),%ymm0
butterflya   8,9,3
vmovdqa     96(%rdx),%ymm3
vmovdqa     96(%r8),%ymm0
butterflya   10,11,3

//level6://256
vmovdqa     128(%rdx),%ymm3
vmovdqa     128(%r8),%ymm0
butterflya   4,6,3
butterflya   5,7,3
vmovdqa     160(%rdx),%ymm3
vmovdqa     160(%r8),%ymm0
butterflya   8,10,3
butterflya   9,11,3

//level7://512

vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 4,8
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 5,9
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 6,10
vmovdqa     224(%rdx),%ymm3
vmovdqa     224(%r8),%ymm0
levelsix 7,11

vmovdqa         %ymm4,(%rdi)
vmovdqa         %ymm5,256(%rdi)
vmovdqa         %ymm6,512(%rdi)
vmovdqa         %ymm7,768(%rdi)
vmovdqa         %ymm8,1024(%rdi)
vmovdqa         %ymm9,1280(%rdi)
vmovdqa         %ymm10,1536(%rdi)
vmovdqa         %ymm11,1792(%rdi)

ret
