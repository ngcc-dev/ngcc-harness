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
vpsraw		$15,%ymm\r,%ymm12
vpand		%ymm0,%ymm12,%ymm12
vpaddw		%ymm12,%ymm\r,%ymm\r
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
vmovdqa   (64*\off+512)*2(%rdi),%ymm8
vmovdqa   (64*\off+528)*2(%rdi),%ymm9
vmovdqa   (64*\off+544)*2(%rdi),%ymm10
vmovdqa   (64*\off+560)*2(%rdi),%ymm11
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
vmovdqa   %ymm8,(64*\off+512)*2(%rdi)
vmovdqa   %ymm9,(64*\off+528)*2(%rdi)
vmovdqa   %ymm10,(64*\off+544)*2(%rdi)
vmovdqa   %ymm11,(64*\off+560)*2(%rdi)
.endm

.macro nttlevel1 off
vmovdqa   (48)*2(%rsi),%ymm15
vmovdqa   (64*\off+256)*2(%rdi),%ymm8
vmovdqa   (64*\off+272)*2(%rdi),%ymm9
vmovdqa   (64*\off+288)*2(%rdi),%ymm10
vmovdqa   (64*\off+304)*2(%rdi),%ymm11
vmovdqa   (64)*2(%rsi),%ymm2

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

vmovdqa   (80)*2(%rsi),%ymm15
vmovdqa   (64*\off+768)*2(%rdi),%ymm8
vmovdqa   (64*\off+784)*2(%rdi),%ymm9
vmovdqa   (64*\off+800)*2(%rdi),%ymm10
vmovdqa   (64*\off+816)*2(%rdi),%ymm11
vmovdqa   (96)*2(%rsi),%ymm2

vmovdqa   (64*\off+512)*2(%rdi),%ymm4
vmovdqa   (64*\off+528)*2(%rdi),%ymm5
vmovdqa   (64*\off+544)*2(%rdi),%ymm6
vmovdqa   (64*\off+560)*2(%rdi),%ymm7

update      3,4,5,6,7,8,9,10,11

vmovdqa   %ymm3,(64*\off+512)*2(%rdi)
vmovdqa   %ymm4,(64*\off+528)*2(%rdi)
vmovdqa   %ymm5,(64*\off+544)*2(%rdi)
vmovdqa   %ymm6,(64*\off+560)*2(%rdi)
vmovdqa   %ymm8,(64*\off+768)*2(%rdi)
vmovdqa   %ymm9,(64*\off+784)*2(%rdi)
vmovdqa   %ymm10,(64*\off+800)*2(%rdi)
vmovdqa   %ymm11,(64*\off+816)*2(%rdi)
.endm

.macro nttlevel2 off
vmovdqa   (112)*2(%rsi),%ymm15
vmovdqa   (64*\off+128)*2(%rdi),%ymm8
vmovdqa   (64*\off+144)*2(%rdi),%ymm9
vmovdqa   (64*\off+160)*2(%rdi),%ymm10
vmovdqa   (64*\off+176)*2(%rdi),%ymm11
vmovdqa   (128)*2(%rsi),%ymm2

vmovdqa   (64*\off+0)*2(%rdi),%ymm4
vmovdqa   (64*\off+16)*2(%rdi),%ymm5
vmovdqa   (64*\off+32)*2(%rdi),%ymm6
vmovdqa   (64*\off+48)*2(%rdi),%ymm7

update      3,4,5,6,7,8,9,10,11

vmovdqa   %ymm3,(64*\off+0)*2(%rdi)
vmovdqa   %ymm4,(64*\off+16)*2(%rdi)
vmovdqa   %ymm5,(64*\off+32)*2(%rdi)
vmovdqa   %ymm6,(64*\off+48)*2(%rdi)
vmovdqa   %ymm8,(64*\off+128)*2(%rdi)
vmovdqa   %ymm9,(64*\off+144)*2(%rdi)
vmovdqa   %ymm10,(64*\off+160)*2(%rdi)
vmovdqa   %ymm11,(64*\off+176)*2(%rdi)

vmovdqa   (144)*2(%rsi),%ymm15
vmovdqa   (64*\off+384)*2(%rdi),%ymm8
vmovdqa   (64*\off+400)*2(%rdi),%ymm9
vmovdqa   (64*\off+416)*2(%rdi),%ymm10
vmovdqa   (64*\off+432)*2(%rdi),%ymm11
vmovdqa   (160)*2(%rsi),%ymm2

vmovdqa   (64*\off+256)*2(%rdi),%ymm4
vmovdqa   (64*\off+272)*2(%rdi),%ymm5
vmovdqa   (64*\off+288)*2(%rdi),%ymm6
vmovdqa   (64*\off+304)*2(%rdi),%ymm7

update      3,4,5,6,7,8,9,10,11

vmovdqa   %ymm3,(64*\off+256)*2(%rdi)
vmovdqa   %ymm4,(64*\off+272)*2(%rdi)
vmovdqa   %ymm5,(64*\off+288)*2(%rdi)
vmovdqa   %ymm6,(64*\off+304)*2(%rdi)
vmovdqa   %ymm8,(64*\off+384)*2(%rdi)
vmovdqa   %ymm9,(64*\off+400)*2(%rdi)
vmovdqa   %ymm10,(64*\off+416)*2(%rdi)
vmovdqa   %ymm11,(64*\off+432)*2(%rdi)

vmovdqa   (176)*2(%rsi),%ymm15
vmovdqa   (64*\off+640)*2(%rdi),%ymm8
vmovdqa   (64*\off+656)*2(%rdi),%ymm9
vmovdqa   (64*\off+672)*2(%rdi),%ymm10
vmovdqa   (64*\off+688)*2(%rdi),%ymm11
vmovdqa   (192)*2(%rsi),%ymm2

vmovdqa   (64*\off+512)*2(%rdi),%ymm4
vmovdqa   (64*\off+528)*2(%rdi),%ymm5
vmovdqa   (64*\off+544)*2(%rdi),%ymm6
vmovdqa   (64*\off+560)*2(%rdi),%ymm7

update      3,4,5,6,7,8,9,10,11

vmovdqa   %ymm3,(64*\off+512)*2(%rdi)
vmovdqa   %ymm4,(64*\off+528)*2(%rdi)
vmovdqa   %ymm5,(64*\off+544)*2(%rdi)
vmovdqa   %ymm6,(64*\off+560)*2(%rdi)
vmovdqa   %ymm8,(64*\off+640)*2(%rdi)
vmovdqa   %ymm9,(64*\off+656)*2(%rdi)
vmovdqa   %ymm10,(64*\off+672)*2(%rdi)
vmovdqa   %ymm11,(64*\off+688)*2(%rdi)

vmovdqa   (208)*2(%rsi),%ymm15
vmovdqa   (64*\off+896)*2(%rdi),%ymm8
vmovdqa   (64*\off+912)*2(%rdi),%ymm9
vmovdqa   (64*\off+928)*2(%rdi),%ymm10
vmovdqa   (64*\off+944)*2(%rdi),%ymm11
vmovdqa   (224)*2(%rsi),%ymm2

vmovdqa   (64*\off+768)*2(%rdi),%ymm4
vmovdqa   (64*\off+784)*2(%rdi),%ymm5
vmovdqa   (64*\off+800)*2(%rdi),%ymm6
vmovdqa   (64*\off+816)*2(%rdi),%ymm7

update      3,4,5,6,7,8,9,10,11

vmovdqa   %ymm3,(64*\off+768)*2(%rdi)
vmovdqa   %ymm4,(64*\off+784)*2(%rdi)
vmovdqa   %ymm5,(64*\off+800)*2(%rdi)
vmovdqa   %ymm6,(64*\off+816)*2(%rdi)
vmovdqa   %ymm8,(64*\off+896)*2(%rdi)
vmovdqa   %ymm9,(64*\off+912)*2(%rdi)
vmovdqa   %ymm10,(64*\off+928)*2(%rdi)
vmovdqa   %ymm11,(64*\off+944)*2(%rdi)
.endm

.macro nttlevel3t6 off
/*level 3*/
vmovdqa   (32*\off+240)*2(%rsi),%ymm15
vmovdqa   (128*\off+64)*2(%rdi),%ymm8
vmovdqa   (128*\off+80)*2(%rdi),%ymm9
vmovdqa   (128*\off+96)*2(%rdi),%ymm10
vmovdqa   (128*\off+112)*2(%rdi),%ymm11
vmovdqa   (32*\off+256)*2(%rsi),%ymm2

vmovdqa   (128*\off+0)*2(%rdi),%ymm4
vmovdqa   (128*\off+16)*2(%rdi),%ymm5
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm7

update		3,4,5,6,7,8,9,10,11

/*level 4*/
shuffle8	5,10,7,10
shuffle8	6,11,5,11
shuffle8	3,8,6,8
shuffle8	4,9,3,9

vmovdqa   (32*\off+496)*2(%rsi),%ymm15
vmovdqa   (32*\off+512)*2(%rsi),%ymm2

update		4,6,8,3,9,7,10,5,11

/*level  5*/
shuffle4	8,5,9,5
shuffle4	3,11,8,11
shuffle4	4,7,3,7
shuffle4	6,10,4,10

vmovdqa   (32*\off+752)*2(%rsi),%ymm15
vmovdqa   (32*\off+768)*2(%rsi),%ymm2

update		6,3,7,4,10,9,5,8,11

/*level 6*/
shuffle2	7,8,10,8
shuffle2	4,11,7,11
shuffle2	6,9,4,9
shuffle2	3,5,6,5

vmovdqa   (32*\off+1008)*2(%rsi),%ymm15
vmovdqa   (32*\off+1024)*2(%rsi),%ymm2

update		3,4,9,6,5,10,8,7,11

vmovdqa   %ymm3,(128*\off+0)*2(%rdi)
vmovdqa   %ymm4,(128*\off+16)*2(%rdi)
vmovdqa   %ymm9,(128*\off+32)*2(%rdi)
vmovdqa   %ymm6,(128*\off+48)*2(%rdi)
vmovdqa   %ymm10,(128*\off+64)*2(%rdi)
vmovdqa   %ymm8,(128*\off+80)*2(%rdi)
vmovdqa   %ymm7,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm

.macro nttlevel3t6_mq off
/*level 3*/
vmovdqa   (32*\off+240)*2(%rsi),%ymm15
vmovdqa   (128*\off+64)*2(%rdi),%ymm8
vmovdqa   (128*\off+80)*2(%rdi),%ymm9
vmovdqa   (128*\off+96)*2(%rdi),%ymm10
vmovdqa   (128*\off+112)*2(%rdi),%ymm11
vmovdqa   (32*\off+256)*2(%rsi),%ymm2

vmovdqa   (128*\off+0)*2(%rdi),%ymm4
vmovdqa   (128*\off+16)*2(%rdi),%ymm5
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm7

update		3,4,5,6,7,8,9,10,11

/*level 4*/
shuffle8	5,10,7,10
shuffle8	6,11,5,11
shuffle8	3,8,6,8
shuffle8	4,9,3,9

vmovdqa   (32*\off+496)*2(%rsi),%ymm15
vmovdqa   (32*\off+512)*2(%rsi),%ymm2

update		4,6,8,3,9,7,10,5,11

/*level  5*/
shuffle4	8,5,9,5
shuffle4	3,11,8,11
shuffle4	4,7,3,7
shuffle4	6,10,4,10

vmovdqa   (32*\off+752)*2(%rsi),%ymm15
vmovdqa   (32*\off+768)*2(%rsi),%ymm2

update		6,3,7,4,10,9,5,8,11

/*level 6*/
shuffle2	7,8,10,8
shuffle2	4,11,7,11
shuffle2	6,9,4,9
shuffle2	3,5,6,5

vmovdqa   (32*\off+1008)*2(%rsi),%ymm15
vmovdqa   (32*\off+1024)*2(%rsi),%ymm2

update		3,4,9,6,5,10,8,7,11

vmovdqa   (1264)*2(%rsi),%ymm1             #MONT_QINV
vmovdqa   (1280)*2(%rsi),%ymm2             #MONT

vpmullw		%ymm1,%ymm3,%ymm12
vpmullw		%ymm1,%ymm4,%ymm13
vpmullw		%ymm1,%ymm9,%ymm14
vpmullw		%ymm1,%ymm6,%ymm15

vpmulhw		%ymm2,%ymm3,%ymm3
vpmulhw		%ymm2,%ymm4,%ymm4
vpmulhw		%ymm2,%ymm9,%ymm9
vpmulhw		%ymm2,%ymm6,%ymm6

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13
vpmulhw		%ymm0,%ymm14,%ymm14
vpmulhw		%ymm0,%ymm15,%ymm15

vpsubw		%ymm12,%ymm3,%ymm3
vpsubw		%ymm13,%ymm4,%ymm4
vpsubw		%ymm14,%ymm9,%ymm9
vpsubw		%ymm15,%ymm6,%ymm6

vpmullw		%ymm1,%ymm10,%ymm12
vpmullw		%ymm1,%ymm8,%ymm13
vpmullw		%ymm1,%ymm7,%ymm14
vpmullw		%ymm1,%ymm11,%ymm15

vpmulhw		%ymm2,%ymm10,%ymm10
vpmulhw		%ymm2,%ymm8,%ymm8
vpmulhw		%ymm2,%ymm7,%ymm7
vpmulhw		%ymm2,%ymm11,%ymm11

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13
vpmulhw		%ymm0,%ymm14,%ymm14
vpmulhw		%ymm0,%ymm15,%ymm15

vpsubw		%ymm12,%ymm10,%ymm10
vpsubw		%ymm13,%ymm8,%ymm8
vpsubw		%ymm14,%ymm7,%ymm7
vpsubw		%ymm15,%ymm11,%ymm11

caddq   3
caddq   4
caddq   9
caddq   6
caddq   10
caddq   8
caddq   7
caddq   11

vmovdqa   %ymm3,(128*\off+0)*2(%rdi)
vmovdqa   %ymm4,(128*\off+16)*2(%rdi)
vmovdqa   %ymm9,(128*\off+32)*2(%rdi)
vmovdqa   %ymm6,(128*\off+48)*2(%rdi)
vmovdqa   %ymm10,(128*\off+64)*2(%rdi)
vmovdqa   %ymm8,(128*\off+80)*2(%rdi)
vmovdqa   %ymm7,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)
.endm

.macro nttpermute off
vmovdqa   (256*\off+128)*2(%rdi),%ymm8
vmovdqa   (256*\off+144)*2(%rdi),%ymm9
vmovdqa   (256*\off+160)*2(%rdi),%ymm10
vmovdqa   (256*\off+176)*2(%rdi),%ymm11

vmovdqa   (256*\off+0)*2(%rdi),%ymm4
vmovdqa   (256*\off+16)*2(%rdi),%ymm5
vmovdqa   (256*\off+32)*2(%rdi),%ymm6
vmovdqa   (256*\off+48)*2(%rdi),%ymm7

shuffle1	6,10,3,10
shuffle1	7,11,6,11
shuffle1	4,8,7,8
shuffle1	5,9,4,9

vmovdqa   %ymm7,(256*\off+0)*2(%rdi)
vmovdqa   %ymm8,(256*\off+16)*2(%rdi)
vmovdqa   %ymm4,(256*\off+32)*2(%rdi)
vmovdqa   %ymm9,(256*\off+48)*2(%rdi)
vmovdqa   %ymm3,(256*\off+128)*2(%rdi)
vmovdqa   %ymm10,(256*\off+144)*2(%rdi)
vmovdqa   %ymm6,(256*\off+160)*2(%rdi)
vmovdqa   %ymm11,(256*\off+176)*2(%rdi)

vmovdqa   (256*\off+192)*2(%rdi),%ymm8
vmovdqa   (256*\off+208)*2(%rdi),%ymm9
vmovdqa   (256*\off+224)*2(%rdi),%ymm10
vmovdqa   (256*\off+240)*2(%rdi),%ymm11

vmovdqa   (256*\off+64)*2(%rdi),%ymm4
vmovdqa   (256*\off+80)*2(%rdi),%ymm5
vmovdqa   (256*\off+96)*2(%rdi),%ymm6
vmovdqa   (256*\off+112)*2(%rdi),%ymm7

shuffle1	6,10,3,10
shuffle1	7,11,6,11
shuffle1	4,8,7,8
shuffle1	5,9,4,9

vmovdqa   %ymm7,(256*\off+64)*2(%rdi)
vmovdqa   %ymm8,(256*\off+80)*2(%rdi)
vmovdqa   %ymm4,(256*\off+96)*2(%rdi)
vmovdqa   %ymm9,(256*\off+112)*2(%rdi)
vmovdqa   %ymm3,(256*\off+192)*2(%rdi)
vmovdqa   %ymm10,(256*\off+208)*2(%rdi)
vmovdqa   %ymm6,(256*\off+224)*2(%rdi)
vmovdqa   %ymm11,(256*\off+240)*2(%rdi)

.endm

.text
.global poly_ntt
poly_ntt:
vmovdqa    0*2(%rsi),%ymm0

nttlevel0 0
nttlevel0 1
nttlevel0 2
nttlevel0 3
nttlevel0 4
nttlevel0 5
nttlevel0 6
nttlevel0 7

nttlevel1 0
nttlevel1 1
nttlevel1 2
nttlevel1 3

nttlevel2 0
nttlevel2 1

nttlevel3t6 0
nttlevel3t6 1
nttlevel3t6 2
nttlevel3t6 3
nttlevel3t6 4
nttlevel3t6 5
nttlevel3t6 6
nttlevel3t6 7

nttpermute 0
nttpermute 1
nttpermute 2
nttpermute 3

ret

.text
.global poly_ntt_mq
poly_ntt_mq:
vmovdqa    0*2(%rsi),%ymm0

nttlevel0 0
nttlevel0 1
nttlevel0 2
nttlevel0 3
nttlevel0 4
nttlevel0 5
nttlevel0 6
nttlevel0 7

nttlevel1 0
nttlevel1 1
nttlevel1 2
nttlevel1 3

nttlevel2 0
nttlevel2 1

nttlevel3t6_mq 0
nttlevel3t6_mq 1
nttlevel3t6_mq 2
nttlevel3t6_mq 3
nttlevel3t6_mq 4
nttlevel3t6_mq 5
nttlevel3t6_mq 6
nttlevel3t6_mq 7

nttpermute 0
nttpermute 1
nttpermute 2
nttpermute 3

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

.macro inttpermute off
vmovdqa   (256*\off+128)*2(%rdi),%ymm8
vmovdqa   (256*\off+144)*2(%rdi),%ymm9
vmovdqa   (256*\off+160)*2(%rdi),%ymm10
vmovdqa   (256*\off+176)*2(%rdi),%ymm11

vmovdqa   (256*\off+0)*2(%rdi),%ymm4
vmovdqa   (256*\off+16)*2(%rdi),%ymm5
vmovdqa   (256*\off+32)*2(%rdi),%ymm6
vmovdqa   (256*\off+48)*2(%rdi),%ymm7

shuffle1	6,10,3,10
shuffle1	7,11,6,11
shuffle1	4,8,7,8
shuffle1	5,9,4,9

vmovdqa   %ymm7,(256*\off+0)*2(%rdi)
vmovdqa   %ymm4,(256*\off+16)*2(%rdi)
vmovdqa   %ymm3,(256*\off+32)*2(%rdi)
vmovdqa   %ymm6,(256*\off+48)*2(%rdi)
vmovdqa   %ymm8,(256*\off+128)*2(%rdi)
vmovdqa   %ymm9,(256*\off+144)*2(%rdi)
vmovdqa   %ymm10,(256*\off+160)*2(%rdi)
vmovdqa   %ymm11,(256*\off+176)*2(%rdi)

vmovdqa   (256*\off+192)*2(%rdi),%ymm8
vmovdqa   (256*\off+208)*2(%rdi),%ymm9
vmovdqa   (256*\off+224)*2(%rdi),%ymm10
vmovdqa   (256*\off+240)*2(%rdi),%ymm11

vmovdqa   (256*\off+64)*2(%rdi),%ymm4
vmovdqa   (256*\off+80)*2(%rdi),%ymm5
vmovdqa   (256*\off+96)*2(%rdi),%ymm6
vmovdqa   (256*\off+112)*2(%rdi),%ymm7

shuffle1	6,10,3,10
shuffle1	7,11,6,11
shuffle1	4,8,7,8
shuffle1	5,9,4,9

vmovdqa   %ymm7,(256*\off+64)*2(%rdi)
vmovdqa   %ymm4,(256*\off+80)*2(%rdi)
vmovdqa   %ymm3,(256*\off+96)*2(%rdi)
vmovdqa   %ymm6,(256*\off+112)*2(%rdi)
vmovdqa   %ymm8,(256*\off+192)*2(%rdi)
vmovdqa   %ymm9,(256*\off+208)*2(%rdi)
vmovdqa   %ymm10,(256*\off+224)*2(%rdi)
vmovdqa   %ymm11,(256*\off+240)*2(%rdi)

.endm

.macro inttlevel3t6 off
vmovdqa   (128*\off+0)*2(%rdi),%ymm3
vmovdqa   (128*\off+16)*2(%rdi),%ymm4
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm8
vmovdqa   (128*\off+64)*2(%rdi),%ymm5
vmovdqa   (128*\off+80)*2(%rdi),%ymm7
vmovdqa   (128*\off+96)*2(%rdi),%ymm9
vmovdqa   (128*\off+112)*2(%rdi),%ymm11

/*level 6*/
vmovdqa   (32*\off+16)*2(%rsi),%ymm2
vmovdqa   (32*\off+32)*2(%rsi),%ymm10

butterfly	3,4,6,8,5,7,9,11,2,2,10,10

shuffle2	3,4,10,4
shuffle2	6,8,3,8
shuffle2	5,7,6,7
shuffle2	9,11,5,11

/*level 5*/
vmovdqa   (32*\off+272)*2(%rsi),%ymm2
vmovdqa   (32*\off+288)*2(%rsi),%ymm9

butterfly	10,3,6,5,4,8,7,11,2,2,9,9

shuffle4	10,3,9,3
shuffle4	6,5,10,5
shuffle4	4,8,6,8
shuffle4	7,11,4,11

/*level 4*/
vmovdqa   (32*\off+528)*2(%rsi),%ymm2
vmovdqa   (32*\off+544)*2(%rsi),%ymm7

butterfly	9,10,6,4,3,5,8,11,2,2,7,7

shuffle8	9,10,7,10
shuffle8	6,4,9,4
shuffle8	3,5,6,5
shuffle8	8,11,3,11

/*level 3*/
vmovdqa   (32*\off+784)*2(%rsi),%ymm2
vmovdqa   (32*\off+800)*2(%rsi),%ymm8

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

.macro inttlevel2 off
vmovdqa   (1040)*2(%rsi),%ymm2
vmovdqa   (64*\off+128)*2(%rdi),%ymm8
vmovdqa   (64*\off+144)*2(%rdi),%ymm9
vmovdqa   (64*\off+160)*2(%rdi),%ymm10
vmovdqa   (64*\off+176)*2(%rdi),%ymm11
vmovdqa   (1056)*2(%rsi),%ymm3

vmovdqa   (64*\off+0)*2(%rdi),%ymm4
vmovdqa   (64*\off+16)*2(%rdi),%ymm5
vmovdqa   (64*\off+32)*2(%rdi),%ymm6
vmovdqa   (64*\off+48)*2(%rdi),%ymm7

butterfly   4,5,6,7,8,9,10,11,2,2,3,3

vmovdqa   %ymm4,(64*\off+0)*2(%rdi)
vmovdqa   %ymm5,(64*\off+16)*2(%rdi)
vmovdqa   %ymm6,(64*\off+32)*2(%rdi)
vmovdqa   %ymm7,(64*\off+48)*2(%rdi)
vmovdqa   %ymm8,(64*\off+128)*2(%rdi)
vmovdqa   %ymm9,(64*\off+144)*2(%rdi)
vmovdqa   %ymm10,(64*\off+160)*2(%rdi)
vmovdqa   %ymm11,(64*\off+176)*2(%rdi)

vmovdqa   (1072)*2(%rsi),%ymm2
vmovdqa   (64*\off+384)*2(%rdi),%ymm8
vmovdqa   (64*\off+400)*2(%rdi),%ymm9
vmovdqa   (64*\off+416)*2(%rdi),%ymm10
vmovdqa   (64*\off+432)*2(%rdi),%ymm11
vmovdqa   (1088)*2(%rsi),%ymm3

vmovdqa   (64*\off+256)*2(%rdi),%ymm4
vmovdqa   (64*\off+272)*2(%rdi),%ymm5
vmovdqa   (64*\off+288)*2(%rdi),%ymm6
vmovdqa   (64*\off+304)*2(%rdi),%ymm7

butterfly   4,5,6,7,8,9,10,11,2,2,3,3

vmovdqa   %ymm4,(64*\off+256)*2(%rdi)
vmovdqa   %ymm5,(64*\off+272)*2(%rdi)
vmovdqa   %ymm6,(64*\off+288)*2(%rdi)
vmovdqa   %ymm7,(64*\off+304)*2(%rdi)
vmovdqa   %ymm8,(64*\off+384)*2(%rdi)
vmovdqa   %ymm9,(64*\off+400)*2(%rdi)
vmovdqa   %ymm10,(64*\off+416)*2(%rdi)
vmovdqa   %ymm11,(64*\off+432)*2(%rdi)

vmovdqa   (1104)*2(%rsi),%ymm2
vmovdqa   (64*\off+640)*2(%rdi),%ymm8
vmovdqa   (64*\off+656)*2(%rdi),%ymm9
vmovdqa   (64*\off+672)*2(%rdi),%ymm10
vmovdqa   (64*\off+688)*2(%rdi),%ymm11
vmovdqa   (1120)*2(%rsi),%ymm3

vmovdqa   (64*\off+512)*2(%rdi),%ymm4
vmovdqa   (64*\off+528)*2(%rdi),%ymm5
vmovdqa   (64*\off+544)*2(%rdi),%ymm6
vmovdqa   (64*\off+560)*2(%rdi),%ymm7

butterfly   4,5,6,7,8,9,10,11,2,2,3,3

vmovdqa   %ymm4,(64*\off+512)*2(%rdi)
vmovdqa   %ymm5,(64*\off+528)*2(%rdi)
vmovdqa   %ymm6,(64*\off+544)*2(%rdi)
vmovdqa   %ymm7,(64*\off+560)*2(%rdi)
vmovdqa   %ymm8,(64*\off+640)*2(%rdi)
vmovdqa   %ymm9,(64*\off+656)*2(%rdi)
vmovdqa   %ymm10,(64*\off+672)*2(%rdi)
vmovdqa   %ymm11,(64*\off+688)*2(%rdi)

vmovdqa   (1136)*2(%rsi),%ymm2
vmovdqa   (64*\off+896)*2(%rdi),%ymm8
vmovdqa   (64*\off+912)*2(%rdi),%ymm9
vmovdqa   (64*\off+928)*2(%rdi),%ymm10
vmovdqa   (64*\off+944)*2(%rdi),%ymm11
vmovdqa   (1152)*2(%rsi),%ymm3

vmovdqa   (64*\off+768)*2(%rdi),%ymm4
vmovdqa   (64*\off+784)*2(%rdi),%ymm5
vmovdqa   (64*\off+800)*2(%rdi),%ymm6
vmovdqa   (64*\off+816)*2(%rdi),%ymm7

butterfly   4,5,6,7,8,9,10,11,2,2,3,3

vmovdqa   %ymm4,(64*\off+768)*2(%rdi)
vmovdqa   %ymm5,(64*\off+784)*2(%rdi)
vmovdqa   %ymm6,(64*\off+800)*2(%rdi)
vmovdqa   %ymm7,(64*\off+816)*2(%rdi)
vmovdqa   %ymm8,(64*\off+896)*2(%rdi)
vmovdqa   %ymm9,(64*\off+912)*2(%rdi)
vmovdqa   %ymm10,(64*\off+928)*2(%rdi)
vmovdqa   %ymm11,(64*\off+944)*2(%rdi)
.endm

.macro inttlevel1 off
vmovdqa   (1168)*2(%rsi),%ymm2
vmovdqa   (64*\off+256)*2(%rdi),%ymm8
vmovdqa   (64*\off+272)*2(%rdi),%ymm9
vmovdqa   (64*\off+288)*2(%rdi),%ymm10
vmovdqa   (64*\off+304)*2(%rdi),%ymm11
vmovdqa   (1184)*2(%rsi),%ymm3

vmovdqa   (64*\off+0)*2(%rdi),%ymm4
vmovdqa   (64*\off+16)*2(%rdi),%ymm5
vmovdqa   (64*\off+32)*2(%rdi),%ymm6
vmovdqa   (64*\off+48)*2(%rdi),%ymm7

butterfly   4,5,6,7,8,9,10,11,2,2,3,3

vmovdqa   %ymm4,(64*\off+0)*2(%rdi)
vmovdqa   %ymm5,(64*\off+16)*2(%rdi)
vmovdqa   %ymm6,(64*\off+32)*2(%rdi)
vmovdqa   %ymm7,(64*\off+48)*2(%rdi)
vmovdqa   %ymm8,(64*\off+256)*2(%rdi)
vmovdqa   %ymm9,(64*\off+272)*2(%rdi)
vmovdqa   %ymm10,(64*\off+288)*2(%rdi)
vmovdqa   %ymm11,(64*\off+304)*2(%rdi)

vmovdqa   (1200)*2(%rsi),%ymm2
vmovdqa   (64*\off+768)*2(%rdi),%ymm8
vmovdqa   (64*\off+784)*2(%rdi),%ymm9
vmovdqa   (64*\off+800)*2(%rdi),%ymm10
vmovdqa   (64*\off+816)*2(%rdi),%ymm11
vmovdqa   (1216)*2(%rsi),%ymm3

vmovdqa   (64*\off+512)*2(%rdi),%ymm4
vmovdqa   (64*\off+528)*2(%rdi),%ymm5
vmovdqa   (64*\off+544)*2(%rdi),%ymm6
vmovdqa   (64*\off+560)*2(%rdi),%ymm7

butterfly   4,5,6,7,8,9,10,11,2,2,3,3

vmovdqa   %ymm4,(64*\off+512)*2(%rdi)
vmovdqa   %ymm5,(64*\off+528)*2(%rdi)
vmovdqa   %ymm6,(64*\off+544)*2(%rdi)
vmovdqa   %ymm7,(64*\off+560)*2(%rdi)
vmovdqa   %ymm8,(64*\off+768)*2(%rdi)
vmovdqa   %ymm9,(64*\off+784)*2(%rdi)
vmovdqa   %ymm10,(64*\off+800)*2(%rdi)
vmovdqa   %ymm11,(64*\off+816)*2(%rdi)
.endm

.macro inttlevel0 off
/*level 0*/
vmovdqa   (1232)*2(%rsi),%ymm2
vmovdqa   (64*\off+512)*2(%rdi),%ymm8
vmovdqa   (64*\off+528)*2(%rdi),%ymm9
vmovdqa   (64*\off+544)*2(%rdi),%ymm10
vmovdqa   (64*\off+560)*2(%rdi),%ymm11
vmovdqa   (1248)*2(%rsi),%ymm3

vmovdqa   (64*\off+0)*2(%rdi),%ymm4
vmovdqa   (64*\off+16)*2(%rdi),%ymm5
vmovdqa   (64*\off+32)*2(%rdi),%ymm6
vmovdqa   (64*\off+48)*2(%rdi),%ymm7

butterfly   4,5,6,7,8,9,10,11,2,2,3,3

vmovdqa   (1264)*2(%rsi),%ymm2
vmovdqa   (1280)*2(%rsi),%ymm3

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
vmovdqa   %ymm8,(64*\off+512)*2(%rdi)
vmovdqa   %ymm9,(64*\off+528)*2(%rdi)
vmovdqa   %ymm10,(64*\off+544)*2(%rdi)
vmovdqa   %ymm11,(64*\off+560)*2(%rdi)
.endm

.text
.global poly_intt
poly_intt:
vmovdqa   (0)*2(%rsi),%ymm0

inttpermute 0
inttpermute 1
inttpermute 2
inttpermute 3

inttlevel3t6 0
inttlevel3t6 1
inttlevel3t6 2
inttlevel3t6 3
inttlevel3t6 4
inttlevel3t6 5
inttlevel3t6 6
inttlevel3t6 7

inttlevel2 0
inttlevel2 1

inttlevel1 0
inttlevel1 1
inttlevel1 2
inttlevel1 3

inttlevel0 0
inttlevel0 1
inttlevel0 2
inttlevel0 3
inttlevel0 4
inttlevel0 5
inttlevel0 6
inttlevel0 7

ret

.macro shuffled2bitreversed0 off
vmovdqa   (256*\off+0)*2(%rdi),%ymm4
vmovdqa   (256*\off+16)*2(%rdi),%ymm5
vmovdqa   (256*\off+32)*2(%rdi),%ymm6
vmovdqa   (256*\off+48)*2(%rdi),%ymm7
vmovdqa   (256*\off+128)*2(%rdi),%ymm8
vmovdqa   (256*\off+144)*2(%rdi),%ymm9
vmovdqa   (256*\off+160)*2(%rdi),%ymm10
vmovdqa   (256*\off+176)*2(%rdi),%ymm11


vmovdqa   %ymm4,(256*\off+0)*2(%rdi)
vmovdqa   %ymm6,(256*\off+16)*2(%rdi)
vmovdqa   %ymm8,(256*\off+32)*2(%rdi)
vmovdqa   %ymm10,(256*\off+48)*2(%rdi)
vmovdqa   %ymm5,(256*\off+128)*2(%rdi)
vmovdqa   %ymm7,(256*\off+144)*2(%rdi)
vmovdqa   %ymm9,(256*\off+160)*2(%rdi)
vmovdqa   %ymm11,(256*\off+176)*2(%rdi)


vmovdqa   (256*\off+64)*2(%rdi),%ymm4
vmovdqa   (256*\off+80)*2(%rdi),%ymm5
vmovdqa   (256*\off+96)*2(%rdi),%ymm6
vmovdqa   (256*\off+112)*2(%rdi),%ymm7
vmovdqa   (256*\off+192)*2(%rdi),%ymm8
vmovdqa   (256*\off+208)*2(%rdi),%ymm9
vmovdqa   (256*\off+224)*2(%rdi),%ymm10
vmovdqa   (256*\off+240)*2(%rdi),%ymm11

vmovdqa   %ymm4,(256*\off+64)*2(%rdi)
vmovdqa   %ymm6,(256*\off+80)*2(%rdi)
vmovdqa   %ymm8,(256*\off+96)*2(%rdi)
vmovdqa   %ymm10,(256*\off+112)*2(%rdi)
vmovdqa   %ymm5,(256*\off+192)*2(%rdi)
vmovdqa   %ymm7,(256*\off+208)*2(%rdi)
vmovdqa   %ymm9,(256*\off+224)*2(%rdi)
vmovdqa   %ymm11,(256*\off+240)*2(%rdi)
.endm

.macro shuffled2bitreversed1 off
vmovdqa   (128*\off+0)*2(%rdi),%ymm3
vmovdqa   (128*\off+16)*2(%rdi),%ymm4
vmovdqa   (128*\off+32)*2(%rdi),%ymm6
vmovdqa   (128*\off+48)*2(%rdi),%ymm8
vmovdqa   (128*\off+64)*2(%rdi),%ymm5
vmovdqa   (128*\off+80)*2(%rdi),%ymm7
vmovdqa   (128*\off+96)*2(%rdi),%ymm9
vmovdqa   (128*\off+112)*2(%rdi),%ymm11

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

shuffled2bitreversed0 0
shuffled2bitreversed0 1
shuffled2bitreversed0 2
shuffled2bitreversed0 3

inttpermute 0
inttpermute 1
inttpermute 2
inttpermute 3

shuffled2bitreversed1 0
shuffled2bitreversed1 1
shuffled2bitreversed1 2
shuffled2bitreversed1 3
shuffled2bitreversed1 4
shuffled2bitreversed1 5
shuffled2bitreversed1 6
shuffled2bitreversed1 7

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

vmovdqa   %ymm9,(128*\off+0)*2(%rdi)
vmovdqa   %ymm8,(128*\off+16)*2(%rdi)
vmovdqa   %ymm7,(128*\off+32)*2(%rdi)
vmovdqa   %ymm6,(128*\off+48)*2(%rdi)
vmovdqa   %ymm5,(128*\off+64)*2(%rdi)
vmovdqa   %ymm4,(128*\off+80)*2(%rdi)
vmovdqa   %ymm3,(128*\off+96)*2(%rdi)
vmovdqa   %ymm11,(128*\off+112)*2(%rdi)

.endm

.text
.global bitreversed_order_2_shuffled_order
bitreversed_order_2_shuffled_order:

bitreversed2shuffled 0
bitreversed2shuffled 1
bitreversed2shuffled 2
bitreversed2shuffled 3
bitreversed2shuffled 4
bitreversed2shuffled 5
bitreversed2shuffled 6
bitreversed2shuffled 7

nttpermute 0
nttpermute 1
nttpermute 2
nttpermute 3

ret
