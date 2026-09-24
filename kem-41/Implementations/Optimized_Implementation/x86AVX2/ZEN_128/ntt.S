#Copyright (c) 2026 Yu Zhang.
#Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
#              School of Cyber Security, University of Chinese Academy of Sciences  
#File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.

.macro shuffle8 r0,r1,r2,r3
vperm2i128	$0x20,%ymm\r1,%ymm\r0,%ymm\r2
vperm2i128	$0x31,%ymm\r1,%ymm\r0,%ymm\r3
.endm

.macro shuffle4 r0,r1,r2,r3
vpunpcklqdq	%ymm\r1,%ymm\r0,%ymm\r2
vpunpckhqdq	%ymm\r1,%ymm\r0,%ymm\r3
.endm

.macro shuffle2 r0,r1,r2,r3
#vpsllq		$32,%ymm\r1,%ymm\r2
vmovsldup	%ymm\r1,%ymm\r2
vpblendd	$0xAA,%ymm\r2,%ymm\r0,%ymm\r2
vpsrlq		$32,%ymm\r0,%ymm\r0
#vmovshdup	%ymm\r0,%ymm\r0
vpblendd	$0xAA,%ymm\r1,%ymm\r0,%ymm\r3
.endm

.macro shuffle1 r0,r1,r2,r3
vpslld		$16,%ymm\r1,%ymm\r2
vpblendw	$0xAA,%ymm\r2,%ymm\r0,%ymm\r2
vpsrld		$16,%ymm\r0,%ymm\r0
vpblendw	$0xAA,%ymm\r1,%ymm\r0,%ymm\r3
.endm

.macro caddq r
vpsraw      $15,%ymm\r,%ymm15
vpand       %ymm0,%ymm15,%ymm15
vpaddw      %ymm15,%ymm\r,%ymm\r
.endm

.macro update rln,rl0,rl1,rl2,rl3,rh0,rh1,rh2,rh3
vpmullw		%ymm15,%ymm\rh0,%ymm12
vpmulhw		%ymm2,%ymm\rh0,%ymm\rh0
vpmullw		%ymm15,%ymm\rh1,%ymm13
vpmulhw		%ymm2,%ymm\rh1,%ymm\rh1
vpmullw		%ymm15,%ymm\rh2,%ymm14
vpmulhw		%ymm2,%ymm\rh2,%ymm\rh2
vpmullw		%ymm15,%ymm\rh3,%ymm15
vpmulhw		%ymm2,%ymm\rh3,%ymm\rh3

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13
vpmulhw		%ymm0,%ymm14,%ymm14
vpmulhw		%ymm0,%ymm15,%ymm15

vpsubw		%ymm12,%ymm\rh0,%ymm\rh0
vpsubw		%ymm13,%ymm\rh1,%ymm\rh1
vpsubw		%ymm14,%ymm\rh2,%ymm\rh2
vpsubw		%ymm15,%ymm\rh3,%ymm\rh3

vpaddw		%ymm\rh0,%ymm\rl0,%ymm\rln
vpsubw		%ymm\rh0,%ymm\rl0,%ymm\rh0
vpaddw		%ymm\rh1,%ymm\rl1,%ymm\rl0
vpsubw		%ymm\rh1,%ymm\rl1,%ymm\rh1
vpaddw		%ymm\rh2,%ymm\rl2,%ymm\rl1
vpsubw		%ymm\rh2,%ymm\rl2,%ymm\rh2
vpaddw		%ymm\rh3,%ymm\rl3,%ymm\rl2
vpsubw		%ymm\rh3,%ymm\rl3,%ymm\rh3
.endm

.macro nttlevel0 off
/*level 0*/
vmovdqa   (16)*2(%rsi),%ymm15
vmovdqa   (64*\off+256)*2(%rdi),%ymm8
vmovdqa   (64*\off+272)*2(%rdi),%ymm9
vmovdqa   (64*\off+288)*2(%rdi),%ymm10
vmovdqa   (64*\off+304)*2(%rdi),%ymm11
vmovdqa   (32)*2(%rsi),%ymm2

vmovdqa   (64*\off+0)*2(%rdi),%ymm4
vmovdqa   (64*\off+16)*2(%rdi),%ymm5
vmovdqa   (64*\off+32)*2(%rdi),%ymm6
vmovdqa   (64*\off+48)*2(%rdi),%ymm7

update      3,4,5,6,7,8,9,10,11

vmovdqa   %ymm3,(64*\off+0)*2(%rdi)
vmovdqa   %ymm4,(64*\off+16)*2(%rdi)
vmovdqa   %ymm5,(64*\off+32)*2(%rdi)
vmovdqa   %ymm6,(64*\off+48)*2(%rdi)
vmovdqa   %ymm8,(64*\off+256)*2(%rdi)
vmovdqa   %ymm9,(64*\off+272)*2(%rdi)
vmovdqa   %ymm10,(64*\off+288)*2(%rdi)
vmovdqa   %ymm11,(64*\off+304)*2(%rdi)
.endm

.macro nttlevel1 off
vmovdqa   (32*\off+48)*2(%rsi),%ymm15
vmovdqa   (256*\off+128)*2(%rdi),%ymm8
vmovdqa   (256*\off+144)*2(%rdi),%ymm9
vmovdqa   (256*\off+160)*2(%rdi),%ymm10
vmovdqa   (256*\off+176)*2(%rdi),%ymm11
vmovdqa   (32*\off+64)*2(%rsi),%ymm2

vmovdqa   (256*\off+0)*2(%rdi),%ymm4
vmovdqa   (256*\off+16)*2(%rdi),%ymm5
vmovdqa   (256*\off+32)*2(%rdi),%ymm6
vmovdqa   (256*\off+48)*2(%rdi),%ymm7

update      3,4,5,6,7,8,9,10,11

vmovdqa   %ymm3,(256*\off+0)*2(%rdi)
vmovdqa   %ymm4,(256*\off+16)*2(%rdi)
vmovdqa   %ymm5,(256*\off+32)*2(%rdi)
vmovdqa   %ymm6,(256*\off+48)*2(%rdi)
vmovdqa   %ymm8,(256*\off+128)*2(%rdi)
vmovdqa   %ymm9,(256*\off+144)*2(%rdi)
vmovdqa   %ymm10,(256*\off+160)*2(%rdi)
vmovdqa   %ymm11,(256*\off+176)*2(%rdi)

vmovdqa   (32*\off+48)*2(%rsi),%ymm15
vmovdqa   (256*\off+192)*2(%rdi),%ymm8
vmovdqa   (256*\off+208)*2(%rdi),%ymm9
vmovdqa   (256*\off+224)*2(%rdi),%ymm10
vmovdqa   (256*\off+240)*2(%rdi),%ymm11
vmovdqa   (32*\off+64)*2(%rsi),%ymm2

vmovdqa   (256*\off+64)*2(%rdi),%ymm4
vmovdqa   (256*\off+80)*2(%rdi),%ymm5
vmovdqa   (256*\off+96)*2(%rdi),%ymm6
vmovdqa   (256*\off+112)*2(%rdi),%ymm7

update      3,4,5,6,7,8,9,10,11

vmovdqa   %ymm3,(256*\off+64)*2(%rdi)
vmovdqa   %ymm4,(256*\off+80)*2(%rdi)
vmovdqa   %ymm5,(256*\off+96)*2(%rdi)
vmovdqa   %ymm6,(256*\off+112)*2(%rdi)
vmovdqa   %ymm8,(256*\off+192)*2(%rdi)
vmovdqa   %ymm9,(256*\off+208)*2(%rdi)
vmovdqa   %ymm10,(256*\off+224)*2(%rdi)
vmovdqa   %ymm11,(256*\off+240)*2(%rdi)
.endm

.macro nttlevel2t6 off
/*level 2*/
vmovdqa   (32*\off+112)*2(%rsi),%ymm15
vmovdqa   (128*\off+64)*2(%rdi),%ymm8
vmovdqa   (128*\off+80)*2(%rdi),%ymm9
vmovdqa   (128*\off+96)*2(%rdi),%ymm10
vmovdqa   (128*\off+112)*2(%rdi),%ymm11
vmovdqa   (32*\off+128)*2(%rsi),%ymm2

vmovdqa   (128*\off+0)*2(%rdi),%ymm4
vmovdqa   (128*\off+16)*2(%rdi),%ymm5
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm7

update		3,4,5,6,7,8,9,10,11

/*level 3*/
shuffle8	5,10,7,10
shuffle8	6,11,5,11
shuffle8	3,8,6,8
shuffle8	4,9,3,9

vmovdqa   (32*\off+240)*2(%rsi),%ymm15
vmovdqa   (32*\off+256)*2(%rsi),%ymm2

update		4,6,8,3,9,7,10,5,11

/*level  4*/
shuffle4	8,5,9,5
shuffle4	3,11,8,11
shuffle4	4,7,3,7
shuffle4	6,10,4,10

vmovdqa   (32*\off+368)*2(%rsi),%ymm15
vmovdqa   (32*\off+384)*2(%rsi),%ymm2

update		6,3,7,4,10,9,5,8,11

/*level 5*/
shuffle2	7,8,10,8
shuffle2	4,11,7,11
shuffle2	6,9,4,9
shuffle2	3,5,6,5

vmovdqa   (32*\off+496)*2(%rsi),%ymm15
vmovdqa   (32*\off+512)*2(%rsi),%ymm2

update		3,4,9,6,5,10,8,7,11

/*level 6*/
shuffle1	9,7,5,7
shuffle1	6,11,9,11
shuffle1	3,10,6,10
shuffle1	4,8,3,8

vmovdqa   (32*\off+624)*2(%rsi),%ymm15
vmovdqa   (32*\off+640)*2(%rsi),%ymm2

update		4,6,10,3,8,5,7,9,11

vmovdqa   %ymm4,(128*\off+0)*2(%rdi)
vmovdqa   %ymm6,(128*\off+16)*2(%rdi)
vmovdqa   %ymm10,(128*\off+32)*2(%rdi)
vmovdqa   %ymm3,(128*\off+48)*2(%rdi)
vmovdqa   %ymm5,(128*\off+64)*2(%rdi)
vmovdqa   %ymm7,(128*\off+80)*2(%rdi)
vmovdqa   %ymm9,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm

.macro nttlevel2t6_mq off
/*level 2*/
vmovdqa   (32*\off+112)*2(%rsi),%ymm15
vmovdqa   (128*\off+64)*2(%rdi),%ymm8
vmovdqa   (128*\off+80)*2(%rdi),%ymm9
vmovdqa   (128*\off+96)*2(%rdi),%ymm10
vmovdqa   (128*\off+112)*2(%rdi),%ymm11
vmovdqa   (32*\off+128)*2(%rsi),%ymm2

vmovdqa   (128*\off+0)*2(%rdi),%ymm4
vmovdqa   (128*\off+16)*2(%rdi),%ymm5
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm7

update		3,4,5,6,7,8,9,10,11

/*level 3*/
shuffle8	5,10,7,10
shuffle8	6,11,5,11
shuffle8	3,8,6,8
shuffle8	4,9,3,9

vmovdqa   (32*\off+240)*2(%rsi),%ymm15
vmovdqa   (32*\off+256)*2(%rsi),%ymm2

update		4,6,8,3,9,7,10,5,11

/*level  4*/
shuffle4	8,5,9,5
shuffle4	3,11,8,11
shuffle4	4,7,3,7
shuffle4	6,10,4,10

vmovdqa   (32*\off+368)*2(%rsi),%ymm15
vmovdqa   (32*\off+384)*2(%rsi),%ymm2

update		6,3,7,4,10,9,5,8,11

/*level 5*/
shuffle2	7,8,10,8
shuffle2	4,11,7,11
shuffle2	6,9,4,9
shuffle2	3,5,6,5

vmovdqa   (32*\off+496)*2(%rsi),%ymm15
vmovdqa   (32*\off+512)*2(%rsi),%ymm2

update		3,4,9,6,5,10,8,7,11

/*level 6*/
shuffle1	9,7,5,7
shuffle1	6,11,9,11
shuffle1	3,10,6,10
shuffle1	4,8,3,8

vmovdqa   (32*\off+624)*2(%rsi),%ymm15
vmovdqa   (32*\off+640)*2(%rsi),%ymm2

update		4,6,10,3,8,5,7,9,11

vmovdqa   (752)*2(%rsi),%ymm1             #MONT_QINV
vmovdqa   (768)*2(%rsi),%ymm2             #MONT

vpmullw		%ymm1,%ymm4,%ymm12
vpmullw		%ymm1,%ymm6,%ymm13
vpmullw		%ymm1,%ymm10,%ymm14
vpmullw		%ymm1,%ymm3,%ymm15

vpmulhw		%ymm2,%ymm4,%ymm4
vpmulhw		%ymm2,%ymm6,%ymm6
vpmulhw		%ymm2,%ymm10,%ymm10
vpmulhw		%ymm2,%ymm3,%ymm3

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13
vpmulhw		%ymm0,%ymm14,%ymm14
vpmulhw		%ymm0,%ymm15,%ymm15

vpsubw		%ymm12,%ymm4,%ymm4
vpsubw		%ymm13,%ymm6,%ymm6
vpsubw		%ymm14,%ymm10,%ymm10
vpsubw		%ymm15,%ymm3,%ymm3

vpmullw		%ymm1,%ymm5,%ymm12
vpmullw		%ymm1,%ymm7,%ymm13
vpmullw		%ymm1,%ymm9,%ymm14
vpmullw		%ymm1,%ymm11,%ymm15

vpmulhw		%ymm2,%ymm5,%ymm5
vpmulhw		%ymm2,%ymm7,%ymm7
vpmulhw		%ymm2,%ymm9,%ymm9
vpmulhw		%ymm2,%ymm11,%ymm11

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13
vpmulhw		%ymm0,%ymm14,%ymm14
vpmulhw		%ymm0,%ymm15,%ymm15

vpsubw		%ymm12,%ymm5,%ymm5
vpsubw		%ymm13,%ymm7,%ymm7
vpsubw		%ymm14,%ymm9,%ymm9
vpsubw		%ymm15,%ymm11,%ymm11

caddq       4
caddq       6
caddq       10
caddq       3
caddq       5
caddq       7
caddq       9
caddq       11

vmovdqa   %ymm4,(128*\off+0)*2(%rdi)
vmovdqa   %ymm6,(128*\off+16)*2(%rdi)
vmovdqa   %ymm10,(128*\off+32)*2(%rdi)
vmovdqa   %ymm3,(128*\off+48)*2(%rdi)
vmovdqa   %ymm5,(128*\off+64)*2(%rdi)
vmovdqa   %ymm7,(128*\off+80)*2(%rdi)
vmovdqa   %ymm9,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm

.text
.global poly_ntt
poly_ntt:
vmovdqa    0*2(%rsi),%ymm0

nttlevel0 0
nttlevel0 1
nttlevel0 2
nttlevel0 3

nttlevel1 0
nttlevel1 1

nttlevel2t6 0
nttlevel2t6 1
nttlevel2t6 2
nttlevel2t6 3

ret

.text
.global poly_ntt_mq
poly_ntt_mq:
vmovdqa    0*2(%rsi),%ymm0
vmovdqa   (752)*2(%rsi),%ymm1

nttlevel0 0
nttlevel0 1
nttlevel0 2
nttlevel0 3

nttlevel1 0
nttlevel1 1

nttlevel2t6_mq 0
nttlevel2t6_mq 1
nttlevel2t6_mq 2
nttlevel2t6_mq 3

ret

.macro fqmulprecomp1 al,ah,b,c
vpmullw		%ymm\al,%ymm\b,%ymm15
vpmulhw		%ymm\ah,%ymm\b,%ymm\c
vpmulhw		%ymm0,%ymm15,%ymm15
vpsubw		%ymm15,%ymm\c,%ymm\c
.endm

.macro fqmulprecomp2 al,ah,b,c
vpmullw		%ymm\al,%ymm\b,%ymm15
vpmulhw		%ymm\ah,%ymm\b,%ymm\c
vpmulhw		%ymm0,%ymm15,%ymm15
vpsubw		%ymm\c,%ymm15,%ymm\c
.endm

.macro schoolbook1 off
vmovdqa   (128*\off+0)*2(%rsi),%ymm2   #a0
vmovdqa   (128*\off+16)*2(%rsi),%ymm3  #a1
vmovdqa   (128*\off+32)*2(%rsi),%ymm4  #a2
vmovdqa   (128*\off+48)*2(%rsi),%ymm5  #a3

vmovdqa   (128*\off+0)*2(%rdx),%ymm6    #b0
vpmullw     %ymm1,%ymm6,%ymm7            #b0.QINV.lo
fqmulprecomp1    7,6,2,8                 #a0b0
fqmulprecomp1    7,6,3,9                 #a1b0
fqmulprecomp1    7,6,4,10                #a2b0
fqmulprecomp1    7,6,5,11                #a3b0

vmovdqa   (128*\off+16)*2(%rdx),%ymm6   #b1
vpmullw     %ymm1,%ymm6,%ymm7            #b1.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b1
vpaddw		     %ymm14,%ymm9,%ymm9      #a0b1+a1b0
fqmulprecomp1    7,6,3,14                #a1b1
vpaddw		     %ymm14,%ymm10,%ymm10    #a1b1+a2b0
fqmulprecomp1    7,6,4,14                #a2b1
vpaddw		     %ymm14,%ymm11,%ymm11    #a2b1+a3b0
fqmulprecomp1    7,6,5,12                #-a3b1

vmovdqa   (128*\off+32)*2(%rdx),%ymm6   #b2
vpmullw     %ymm1,%ymm6,%ymm7            #b2.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b2
vpaddw		     %ymm14,%ymm10,%ymm10    #a0b2+a1b1+a2b0
fqmulprecomp1    7,6,3,14                #a1b2
vpaddw		     %ymm14,%ymm11,%ymm11    #a1b2+a2b1+a3b0
fqmulprecomp1    7,6,4,14                #-a2b2
vpaddw		     %ymm14,%ymm12,%ymm12    #-a3b1-a2b2
fqmulprecomp1    7,6,5,13                #-a3b2

vmovdqa   (128*\off+48)*2(%rdx),%ymm6   #b3
vpmullw     %ymm1,%ymm6,%ymm7            #b3.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b3
vpaddw		     %ymm14,%ymm11,%ymm11    #a0b3+a1b2+a2b1+a3b0
fqmulprecomp1    7,6,3,14                #-a1b3
vpaddw		     %ymm14,%ymm12,%ymm12    #-a1b3-a3b1-a2b2
fqmulprecomp1    7,6,4,14                #-a2b3
vpaddw		     %ymm14,%ymm13,%ymm13    #-a2b3+-a3b2
fqmulprecomp1    7,6,5,14                #-a3b3

vmovdqa   (32*\off+32)*2(%rcx),%ymm7   #zeta.QINV
vmovdqa   (32*\off+48)*2(%rcx),%ymm6   #zeta
fqmulprecomp1    7,6,12,12               #-a1b3-a3b1-a2b2.zeta
fqmulprecomp1    7,6,13,13               #-a2b3+-a3b2.zeta
fqmulprecomp1    7,6,14,14               #-a3b3.zeta

vpaddw		     %ymm12,%ymm8,%ymm8
vpaddw		     %ymm13,%ymm9,%ymm9
vpaddw		     %ymm14,%ymm10,%ymm10

vmovdqa   %ymm8,(128*\off+0)*2(%rdi)
vmovdqa   %ymm9,(128*\off+16)*2(%rdi)
vmovdqa   %ymm10,(128*\off+32)*2(%rdi)
vmovdqa   %ymm11,(128*\off+48)*2(%rdi)
.endm

.macro schoolbook2 off
vmovdqa   (128*\off+64)*2(%rsi),%ymm2   #a0
vmovdqa   (128*\off+80)*2(%rsi),%ymm3  #a1
vmovdqa   (128*\off+96)*2(%rsi),%ymm4  #a2
vmovdqa   (128*\off+112)*2(%rsi),%ymm5  #a3

vmovdqa   (128*\off+64)*2(%rdx),%ymm6    #b0
vpmullw     %ymm1,%ymm6,%ymm7            #b0.QINV.lo
fqmulprecomp1    7,6,2,8                 #a0b0
fqmulprecomp1    7,6,3,9                 #a1b0
fqmulprecomp1    7,6,4,10                #a2b0
fqmulprecomp1    7,6,5,11                #a3b0

vmovdqa   (128*\off+80)*2(%rdx),%ymm6   #b1
vpmullw     %ymm1,%ymm6,%ymm7            #b1.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b1
vpaddw		     %ymm14,%ymm9,%ymm9      #a0b1+a1b0
fqmulprecomp1    7,6,3,14                #a1b1
vpaddw		     %ymm14,%ymm10,%ymm10    #a1b1+a2b0
fqmulprecomp1    7,6,4,14                #a2b1
vpaddw		     %ymm14,%ymm11,%ymm11    #a2b1+a3b0
fqmulprecomp2    7,6,5,12                #-a3b1

vmovdqa   (128*\off+96)*2(%rdx),%ymm6   #b2
vpmullw     %ymm1,%ymm6,%ymm7            #b2.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b2
vpaddw		     %ymm14,%ymm10,%ymm10    #a0b2+a1b1+a2b0
fqmulprecomp1    7,6,3,14                #a1b2
vpaddw		     %ymm14,%ymm11,%ymm11    #a1b2+a2b1+a3b0
fqmulprecomp2    7,6,4,14                #-a2b2
vpaddw		     %ymm14,%ymm12,%ymm12    #-a3b1-a2b2
fqmulprecomp2    7,6,5,13                #-a3b2

vmovdqa   (128*\off+112)*2(%rdx),%ymm6   #b3
vpmullw     %ymm1,%ymm6,%ymm7            #b3.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b3
vpaddw		     %ymm14,%ymm11,%ymm11    #a0b3+a1b2+a2b1+a3b0
fqmulprecomp2    7,6,3,14                #-a1b3
vpaddw		     %ymm14,%ymm12,%ymm12    #-a1b3-a3b1-a2b2
fqmulprecomp2    7,6,4,14                #-a2b3
vpaddw		     %ymm14,%ymm13,%ymm13    #-a2b3+-a3b2
fqmulprecomp2    7,6,5,14                #-a3b3

vmovdqa   (32*\off+32)*2(%rcx),%ymm7   #zeta.QINV
vmovdqa   (32*\off+48)*2(%rcx),%ymm6   #zeta
fqmulprecomp1    7,6,12,12               #-a1b3-a3b1-a2b2.zeta
fqmulprecomp1    7,6,13,13               #-a2b3+-a3b2.zeta
fqmulprecomp1    7,6,14,14               #-a3b3.zeta

vpaddw		     %ymm12,%ymm8,%ymm8
vpaddw		     %ymm13,%ymm9,%ymm9
vpaddw		     %ymm14,%ymm10,%ymm10

vmovdqa   %ymm8,(128*\off+64)*2(%rdi)
vmovdqa   %ymm9,(128*\off+80)*2(%rdi)
vmovdqa   %ymm10,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm


.text
.global poly_basemul_ntt
poly_basemul_ntt:

vmovdqa   (0)*2(%rcx),%ymm0             #Q
vmovdqa   (16)*2(%rcx),%ymm1            #QINV

schoolbook1 0
schoolbook1 1
schoolbook1 2
schoolbook1 3
schoolbook2 0
schoolbook2 1
schoolbook2 2
schoolbook2 3

ret

.macro schoolbook1_mq off
vmovdqa   (128*\off+0)*2(%rsi),%ymm2   #a0
vmovdqa   (128*\off+16)*2(%rsi),%ymm3  #a1
vmovdqa   (128*\off+32)*2(%rsi),%ymm4  #a2
vmovdqa   (128*\off+48)*2(%rsi),%ymm5  #a3

vmovdqa   (128*\off+0)*2(%rdx),%ymm6    #b0
vpmullw     %ymm1,%ymm6,%ymm7            #b0.QINV.lo
fqmulprecomp1    7,6,2,8                 #a0b0
fqmulprecomp1    7,6,3,9                 #a1b0
fqmulprecomp1    7,6,4,10                #a2b0
fqmulprecomp1    7,6,5,11                #a3b0

vmovdqa   (128*\off+16)*2(%rdx),%ymm6   #b1
vpmullw     %ymm1,%ymm6,%ymm7            #b1.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b1
vpaddw		     %ymm14,%ymm9,%ymm9      #a0b1+a1b0
fqmulprecomp1    7,6,3,14                #a1b1
vpaddw		     %ymm14,%ymm10,%ymm10    #a1b1+a2b0
fqmulprecomp1    7,6,4,14                #a2b1
vpaddw		     %ymm14,%ymm11,%ymm11    #a2b1+a3b0
fqmulprecomp1    7,6,5,12                #-a3b1

vmovdqa   (128*\off+32)*2(%rdx),%ymm6   #b2
vpmullw     %ymm1,%ymm6,%ymm7            #b2.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b2
vpaddw		     %ymm14,%ymm10,%ymm10    #a0b2+a1b1+a2b0
fqmulprecomp1    7,6,3,14                #a1b2
vpaddw		     %ymm14,%ymm11,%ymm11    #a1b2+a2b1+a3b0
fqmulprecomp1    7,6,4,14                #-a2b2
vpaddw		     %ymm14,%ymm12,%ymm12    #-a3b1-a2b2
fqmulprecomp1    7,6,5,13                #-a3b2

vmovdqa   (128*\off+48)*2(%rdx),%ymm6   #b3
vpmullw     %ymm1,%ymm6,%ymm7            #b3.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b3
vpaddw		     %ymm14,%ymm11,%ymm11    #a0b3+a1b2+a2b1+a3b0
fqmulprecomp1    7,6,3,14                #-a1b3
vpaddw		     %ymm14,%ymm12,%ymm12    #-a1b3-a3b1-a2b2
fqmulprecomp1    7,6,4,14                #-a2b3
vpaddw		     %ymm14,%ymm13,%ymm13    #-a2b3+-a3b2
fqmulprecomp1    7,6,5,14                #-a3b3

vmovdqa   (32*\off+32)*2(%rcx),%ymm7   #zeta.QINV
vmovdqa   (32*\off+48)*2(%rcx),%ymm6   #zeta
fqmulprecomp1    7,6,12,12               #-a1b3-a3b1-a2b2.zeta
fqmulprecomp1    7,6,13,13               #-a2b3+-a3b2.zeta
fqmulprecomp1    7,6,14,14               #-a3b3.zeta

vpaddw		     %ymm12,%ymm8,%ymm8
vpaddw		     %ymm13,%ymm9,%ymm9
vpaddw		     %ymm14,%ymm10,%ymm10

vmovdqa   (160)*2(%rcx),%ymm2             #MONT_QINV
vmovdqa   (176)*2(%rcx),%ymm3             #MONT

vpmullw		%ymm2,%ymm8,%ymm12
vpmullw		%ymm2,%ymm9,%ymm13
vpmullw		%ymm2,%ymm10,%ymm14
vpmullw		%ymm2,%ymm11,%ymm15

vpmulhw		%ymm3,%ymm8,%ymm4
vpmulhw		%ymm3,%ymm9,%ymm5
vpmulhw		%ymm3,%ymm10,%ymm6
vpmulhw		%ymm3,%ymm11,%ymm7

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13
vpmulhw		%ymm0,%ymm14,%ymm14
vpmulhw		%ymm0,%ymm15,%ymm15

vpsubw		%ymm12,%ymm4,%ymm8
vpsubw		%ymm13,%ymm5,%ymm9
vpsubw		%ymm14,%ymm6,%ymm10
vpsubw		%ymm15,%ymm7,%ymm11

caddq       8
caddq       9
caddq       10
caddq       11

vmovdqa   %ymm8,(128*\off+0)*2(%rdi)
vmovdqa   %ymm9,(128*\off+16)*2(%rdi)
vmovdqa   %ymm10,(128*\off+32)*2(%rdi)
vmovdqa   %ymm11,(128*\off+48)*2(%rdi)
.endm

.macro schoolbook2_mq off
vmovdqa   (128*\off+64)*2(%rsi),%ymm2   #a0
vmovdqa   (128*\off+80)*2(%rsi),%ymm3  #a1
vmovdqa   (128*\off+96)*2(%rsi),%ymm4  #a2
vmovdqa   (128*\off+112)*2(%rsi),%ymm5  #a3

vmovdqa   (128*\off+64)*2(%rdx),%ymm6    #b0
vpmullw     %ymm1,%ymm6,%ymm7            #b0.QINV.lo
fqmulprecomp1    7,6,2,8                 #a0b0
fqmulprecomp1    7,6,3,9                 #a1b0
fqmulprecomp1    7,6,4,10                #a2b0
fqmulprecomp1    7,6,5,11                #a3b0

vmovdqa   (128*\off+80)*2(%rdx),%ymm6   #b1
vpmullw     %ymm1,%ymm6,%ymm7            #b1.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b1
vpaddw		     %ymm14,%ymm9,%ymm9      #a0b1+a1b0
fqmulprecomp1    7,6,3,14                #a1b1
vpaddw		     %ymm14,%ymm10,%ymm10    #a1b1+a2b0
fqmulprecomp1    7,6,4,14                #a2b1
vpaddw		     %ymm14,%ymm11,%ymm11    #a2b1+a3b0
fqmulprecomp2    7,6,5,12                #-a3b1

vmovdqa   (128*\off+96)*2(%rdx),%ymm6   #b2
vpmullw     %ymm1,%ymm6,%ymm7            #b2.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b2
vpaddw		     %ymm14,%ymm10,%ymm10    #a0b2+a1b1+a2b0
fqmulprecomp1    7,6,3,14                #a1b2
vpaddw		     %ymm14,%ymm11,%ymm11    #a1b2+a2b1+a3b0
fqmulprecomp2    7,6,4,14                #-a2b2
vpaddw		     %ymm14,%ymm12,%ymm12    #-a3b1-a2b2
fqmulprecomp2    7,6,5,13                #-a3b2

vmovdqa   (128*\off+112)*2(%rdx),%ymm6   #b3
vpmullw     %ymm1,%ymm6,%ymm7            #b3.QINV.lo
fqmulprecomp1    7,6,2,14                #a0b3
vpaddw		     %ymm14,%ymm11,%ymm11    #a0b3+a1b2+a2b1+a3b0
fqmulprecomp2    7,6,3,14                #-a1b3
vpaddw		     %ymm14,%ymm12,%ymm12    #-a1b3-a3b1-a2b2
fqmulprecomp2    7,6,4,14                #-a2b3
vpaddw		     %ymm14,%ymm13,%ymm13    #-a2b3+-a3b2
fqmulprecomp2    7,6,5,14                #-a3b3

vmovdqa   (32*\off+32)*2(%rcx),%ymm7   #zeta.QINV
vmovdqa   (32*\off+48)*2(%rcx),%ymm6   #zeta
fqmulprecomp1    7,6,12,12               #-a1b3-a3b1-a2b2.zeta
fqmulprecomp1    7,6,13,13               #-a2b3+-a3b2.zeta
fqmulprecomp1    7,6,14,14               #-a3b3.zeta

vpaddw		     %ymm12,%ymm8,%ymm8
vpaddw		     %ymm13,%ymm9,%ymm9
vpaddw		     %ymm14,%ymm10,%ymm10

vmovdqa   (160)*2(%rcx),%ymm2             #MONT_QINV
vmovdqa   (176)*2(%rcx),%ymm3             #MONT

vpmullw		%ymm2,%ymm8,%ymm12
vpmullw		%ymm2,%ymm9,%ymm13
vpmullw		%ymm2,%ymm10,%ymm14
vpmullw		%ymm2,%ymm11,%ymm15

vpmulhw		%ymm3,%ymm8,%ymm4
vpmulhw		%ymm3,%ymm9,%ymm5
vpmulhw		%ymm3,%ymm10,%ymm6
vpmulhw		%ymm3,%ymm11,%ymm7

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13
vpmulhw		%ymm0,%ymm14,%ymm14
vpmulhw		%ymm0,%ymm15,%ymm15

vpsubw		%ymm12,%ymm4,%ymm8
vpsubw		%ymm13,%ymm5,%ymm9
vpsubw		%ymm14,%ymm6,%ymm10
vpsubw		%ymm15,%ymm7,%ymm11

caddq       8
caddq       9
caddq       10
caddq       11

vmovdqa   %ymm8,(128*\off+64)*2(%rdi)
vmovdqa   %ymm9,(128*\off+80)*2(%rdi)
vmovdqa   %ymm10,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm


.text
.global poly_basemul_ntt_mq
poly_basemul_ntt_mq:

vmovdqa   (0)*2(%rcx),%ymm0             #Q
vmovdqa   (16)*2(%rcx),%ymm1            #QINV

schoolbook1_mq 0
schoolbook1_mq 1
schoolbook1_mq 2
schoolbook1_mq 3
schoolbook2_mq 0
schoolbook2_mq 1
schoolbook2_mq 2
schoolbook2_mq 3

ret


.macro butterfly rl0,rl1,rl2,rl3,rh0,rh1,rh2,rh3,zl0=2,zl1=2,zh0=3,zh1=3
vpsubw		%ymm\rh0,%ymm\rl0,%ymm12
vpaddw		%ymm\rh0,%ymm\rl0,%ymm\rl0
vpsubw		%ymm\rh1,%ymm\rl1,%ymm13

vpmullw		%ymm\zl0,%ymm12,%ymm\rh0
vpaddw		%ymm\rh1,%ymm\rl1,%ymm\rl1
vpsubw		%ymm\rh2,%ymm\rl2,%ymm14

vpmullw		%ymm\zl0,%ymm13,%ymm\rh1
vpaddw		%ymm\rh2,%ymm\rl2,%ymm\rl2
vpsubw		%ymm\rh3,%ymm\rl3,%ymm15

vpmullw		%ymm\zl1,%ymm14,%ymm\rh2
vpaddw		%ymm\rh3,%ymm\rl3,%ymm\rl3
vpmullw		%ymm\zl1,%ymm15,%ymm\rh3

vpmulhw		%ymm\zh0,%ymm12,%ymm12
vpmulhw		%ymm\zh0,%ymm13,%ymm13

vpmulhw		%ymm\zh1,%ymm14,%ymm14
vpmulhw		%ymm\zh1,%ymm15,%ymm15

vpmulhw		%ymm0,%ymm\rh0,%ymm\rh0

vpmulhw		%ymm0,%ymm\rh1,%ymm\rh1

vpmulhw		%ymm0,%ymm\rh2,%ymm\rh2
vpmulhw		%ymm0,%ymm\rh3,%ymm\rh3

vpsubw		%ymm\rh0,%ymm12,%ymm\rh0

vpsubw		%ymm\rh1,%ymm13,%ymm\rh1

vpsubw		%ymm\rh2,%ymm14,%ymm\rh2
vpsubw		%ymm\rh3,%ymm15,%ymm\rh3
.endm

.macro inttlevel2t6 off
vmovdqa   (128*\off+0)*2(%rdi),%ymm4
vmovdqa   (128*\off+16)*2(%rdi),%ymm5
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm7
vmovdqa   (128*\off+64)*2(%rdi),%ymm8
vmovdqa   (128*\off+80)*2(%rdi),%ymm9
vmovdqa   (128*\off+96)*2(%rdi),%ymm10
vmovdqa   (128*\off+112)*2(%rdi),%ymm11

/*level 6*/
vmovdqa   (32*\off+16)*2(%rsi),%ymm2
vmovdqa   (32*\off+32)*2(%rsi),%ymm3

butterfly	4,5,6,7,8,9,10,11,2,2,3,3

shuffle1	4,5,3,5
shuffle1	6,7,4,7
shuffle1	8,9,6,9
shuffle1	10,11,8,11

/*level 5*/
vmovdqa   (32*\off+144)*2(%rsi),%ymm2
vmovdqa   (32*\off+160)*2(%rsi),%ymm10

butterfly	3,4,6,8,5,7,9,11,2,2,10,10

shuffle2	3,4,10,4
shuffle2	6,8,3,8
shuffle2	5,7,6,7
shuffle2	9,11,5,11

/*level 4*/
vmovdqa   (32*\off+272)*2(%rsi),%ymm2
vmovdqa   (32*\off+288)*2(%rsi),%ymm9

butterfly	10,3,6,5,4,8,7,11,2,2,9,9

shuffle4	10,3,9,3
shuffle4	6,5,10,5
shuffle4	4,8,6,8
shuffle4	7,11,4,11

/*level 3*/
vmovdqa   (32*\off+400)*2(%rsi),%ymm2
vmovdqa   (32*\off+416)*2(%rsi),%ymm7

butterfly	9,10,6,4,3,5,8,11,2,2,7,7

shuffle8	9,10,7,10
shuffle8	6,4,9,4
shuffle8	3,5,6,5
shuffle8	8,11,3,11

/*level 2*/
vmovdqa   (32*\off+528)*2(%rsi),%ymm2
vmovdqa   (32*\off+544)*2(%rsi),%ymm8

butterfly	7,9,6,3,10,4,5,11,2,2,8,8

vmovdqa   %ymm7,(128*\off+0)*2(%rdi)
vmovdqa   %ymm9,(128*\off+16)*2(%rdi)
vmovdqa   %ymm6,(128*\off+32)*2(%rdi)
vmovdqa   %ymm3,(128*\off+48)*2(%rdi)
vmovdqa   %ymm10,(128*\off+64)*2(%rdi)
vmovdqa   %ymm4,(128*\off+80)*2(%rdi)
vmovdqa   %ymm5,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm


.macro inttlevel1 off

vmovdqa   (256*\off+128)*2(%rdi),%ymm8
vmovdqa   (256*\off+144)*2(%rdi),%ymm9
vmovdqa   (256*\off+160)*2(%rdi),%ymm10
vmovdqa   (256*\off+176)*2(%rdi),%ymm11
vmovdqa   (32*\off+656)*2(%rsi),%ymm2
vmovdqa   (32*\off+672)*2(%rsi),%ymm3

vmovdqa   (256*\off+0)*2(%rdi),%ymm4
vmovdqa   (256*\off+16)*2(%rdi),%ymm5
vmovdqa   (256*\off+32)*2(%rdi),%ymm6
vmovdqa   (256*\off+48)*2(%rdi),%ymm7

butterfly	4,5,6,7,8,9,10,11

vmovdqa   %ymm4,(256*\off+0)*2(%rdi)
vmovdqa   %ymm5,(256*\off+16)*2(%rdi)
vmovdqa   %ymm6,(256*\off+32)*2(%rdi)
vmovdqa   %ymm7,(256*\off+48)*2(%rdi)
vmovdqa   %ymm8,(256*\off+128)*2(%rdi)
vmovdqa   %ymm9,(256*\off+144)*2(%rdi)
vmovdqa   %ymm10,(256*\off+160)*2(%rdi)
vmovdqa   %ymm11,(256*\off+176)*2(%rdi)

vmovdqa   (256*\off+192)*2(%rdi),%ymm8
vmovdqa   (256*\off+208)*2(%rdi),%ymm9
vmovdqa   (256*\off+224)*2(%rdi),%ymm10
vmovdqa   (256*\off+240)*2(%rdi),%ymm11
vmovdqa   (32*\off+656)*2(%rsi),%ymm2
vmovdqa   (32*\off+672)*2(%rsi),%ymm3

vmovdqa   (256*\off+64)*2(%rdi),%ymm4
vmovdqa   (256*\off+80)*2(%rdi),%ymm5
vmovdqa   (256*\off+96)*2(%rdi),%ymm6
vmovdqa   (256*\off+112)*2(%rdi),%ymm7

butterfly	4,5,6,7,8,9,10,11

vmovdqa   %ymm4,(256*\off+64)*2(%rdi)
vmovdqa   %ymm5,(256*\off+80)*2(%rdi)
vmovdqa   %ymm6,(256*\off+96)*2(%rdi)
vmovdqa   %ymm7,(256*\off+112)*2(%rdi)
vmovdqa   %ymm8,(256*\off+192)*2(%rdi)
vmovdqa   %ymm9,(256*\off+208)*2(%rdi)
vmovdqa   %ymm10,(256*\off+224)*2(%rdi)
vmovdqa   %ymm11,(256*\off+240)*2(%rdi)
.endm

.macro inttlevel0 off
vmovdqa   (64*\off+0)*2(%rdi),%ymm4
vmovdqa   (64*\off+16)*2(%rdi),%ymm5
vmovdqa   (64*\off+32)*2(%rdi),%ymm6
vmovdqa   (64*\off+48)*2(%rdi),%ymm7
vmovdqa   (64*\off+256)*2(%rdi),%ymm8
vmovdqa   (64*\off+272)*2(%rdi),%ymm9
vmovdqa   (64*\off+288)*2(%rdi),%ymm10
vmovdqa   (64*\off+304)*2(%rdi),%ymm11
vmovdqa   (720)*2(%rsi),%ymm2
vmovdqa   (736)*2(%rsi),%ymm3

butterfly	4,5,6,7,8,9,10,11,2,2,3,3

vmovdqa   (752)*2(%rsi),%ymm2
vmovdqa   (768)*2(%rsi),%ymm3

vpmullw		%ymm2,%ymm4,%ymm12
vpmullw		%ymm2,%ymm5,%ymm13
vpmullw		%ymm2,%ymm6,%ymm14
vpmullw		%ymm2,%ymm7,%ymm15

vpmulhw		%ymm3,%ymm4,%ymm4
vpmulhw		%ymm3,%ymm5,%ymm5
vpmulhw		%ymm3,%ymm6,%ymm6
vpmulhw		%ymm3,%ymm7,%ymm7

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13
vpmulhw		%ymm0,%ymm14,%ymm14
vpmulhw		%ymm0,%ymm15,%ymm15

vpsubw		%ymm12,%ymm4,%ymm4
vpsubw		%ymm13,%ymm5,%ymm5
vpsubw		%ymm14,%ymm6,%ymm6
vpsubw		%ymm15,%ymm7,%ymm7

vmovdqa   %ymm4,(64*\off+0)*2(%rdi)
vmovdqa   %ymm5,(64*\off+16)*2(%rdi)
vmovdqa   %ymm6,(64*\off+32)*2(%rdi)
vmovdqa   %ymm7,(64*\off+48)*2(%rdi)
vmovdqa   %ymm8,(64*\off+256)*2(%rdi)
vmovdqa   %ymm9,(64*\off+272)*2(%rdi)
vmovdqa   %ymm10,(64*\off+288)*2(%rdi)
vmovdqa   %ymm11,(64*\off+304)*2(%rdi)
.endm

.text
.global poly_intt
poly_intt:
vmovdqa   (0)*2(%rsi),%ymm0

inttlevel2t6 0
inttlevel2t6 1
inttlevel2t6 2
inttlevel2t6 3

inttlevel1 0
inttlevel1 1

inttlevel0 0
inttlevel0 1
inttlevel0 2
inttlevel0 3
ret

.macro shuffled2bitreversed off
vmovdqa   (128*\off+0)*2(%rdi),%ymm4
vmovdqa   (128*\off+16)*2(%rdi),%ymm5
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm7
vmovdqa   (128*\off+64)*2(%rdi),%ymm8
vmovdqa   (128*\off+80)*2(%rdi),%ymm9
vmovdqa   (128*\off+96)*2(%rdi),%ymm10
vmovdqa   (128*\off+112)*2(%rdi),%ymm11

shuffle1	4,5,3,5
shuffle1	6,7,4,7
shuffle1	8,9,6,9
shuffle1	10,11,8,11

shuffle2	3,4,10,4
shuffle2	6,8,3,8
shuffle2	5,7,6,7
shuffle2	9,11,5,11

shuffle4	10,3,9,3
shuffle4	6,5,10,5
shuffle4	4,8,6,8
shuffle4	7,11,4,11

shuffle8	9,10,7,10
shuffle8	6,4,9,4
shuffle8	3,5,6,5
shuffle8	8,11,3,11

vmovdqa   %ymm7,(128*\off+0)*2(%rdi)
vmovdqa   %ymm9,(128*\off+16)*2(%rdi)
vmovdqa   %ymm6,(128*\off+32)*2(%rdi)
vmovdqa   %ymm3,(128*\off+48)*2(%rdi)
vmovdqa   %ymm10,(128*\off+64)*2(%rdi)
vmovdqa   %ymm4,(128*\off+80)*2(%rdi)
vmovdqa   %ymm5,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm


.text
.global shuffled_order_2_bitreversed_order
shuffled_order_2_bitreversed_order:

shuffled2bitreversed 0
shuffled2bitreversed 1
shuffled2bitreversed 2
shuffled2bitreversed 3

ret


.macro bitreversed2shuffled off
vmovdqa   (128*\off+0)*2(%rdi),%ymm4
vmovdqa   (128*\off+16)*2(%rdi),%ymm5
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm7
vmovdqa   (128*\off+64)*2(%rdi),%ymm8
vmovdqa   (128*\off+80)*2(%rdi),%ymm9
vmovdqa   (128*\off+96)*2(%rdi),%ymm10
vmovdqa   (128*\off+112)*2(%rdi),%ymm11

shuffle8	4,8,3,8
shuffle8	5,9,4,9
shuffle8	6,10,5,10
shuffle8	7,11,6,11

shuffle4	3,5,7,5
shuffle4	8,10,3,10
shuffle4	4,6,8,6
shuffle4	9,11,4,11

shuffle2	7,8,9,8
shuffle2	5,6,7,6
shuffle2	3,4,5,4
shuffle2	10,11,3,11

shuffle1	9,5,10,5
shuffle1	8,4,9,4
shuffle1	7,3,8,3
shuffle1	6,11,7,11

vmovdqa   %ymm10,(128*\off+0)*2(%rdi)
vmovdqa   %ymm5,(128*\off+16)*2(%rdi)
vmovdqa   %ymm9,(128*\off+32)*2(%rdi)
vmovdqa   %ymm4,(128*\off+48)*2(%rdi)
vmovdqa   %ymm8,(128*\off+64)*2(%rdi)
vmovdqa   %ymm3,(128*\off+80)*2(%rdi)
vmovdqa   %ymm7,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm


.text
.global bitreversed_order_2_shuffled_order
bitreversed_order_2_shuffled_order:

bitreversed2shuffled 0
bitreversed2shuffled 1
bitreversed2shuffled 2
bitreversed2shuffled 3

ret