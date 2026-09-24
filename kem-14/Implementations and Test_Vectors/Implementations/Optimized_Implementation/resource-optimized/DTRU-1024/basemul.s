
.macro fqmul l,h,r

vpmulhw   %ymm\l,%ymm\h,%ymm12
vpmullw   %ymm\l,%ymm\h,%ymm13
vpmullw %ymm0,%ymm13,%ymm14
vpmulhw %ymm1,%ymm14,%ymm14
vpsubw  %ymm14,%ymm12,%ymm\r

.endm

.macro barret h,l,r
vpmulhw     %ymm\h,%ymm\l,%ymm12
vpsraw      $10,%ymm12,%ymm12
vpmullw     %ymm11,%ymm12,%ymm12
vpsubw      %ymm12,%ymm\l,%ymm\r
.endm

.macro calc_d l,r
//a
vmovdqa    (32*\l)(%r8),%ymm4
vmovdqa    (32*\r)(%r8),%ymm5
vpaddw     %ymm4,%ymm5,%ymm6

//b
vmovdqa    (32*\l+256)(%r8),%ymm4
vmovdqa    (32*\r+256)(%r8),%ymm5
vpaddw     %ymm4,%ymm5,%ymm7

//d

vmovdqa     (32*\l+512)(%r8),%ymm4
vmovdqa     (32*\r+512)(%r8),%ymm5
vpaddw     %ymm4,%ymm5,%ymm10

fqmul      6,7,4
vpsubw     %ymm10,%ymm4,%ymm4
.endm

.text
mul:
//a[0]
vmovdqa   0(%rsi),%ymm4
//a[1]
vmovdqa   32(%rsi),%ymm5
vmovdqa   %ymm4,0(%r8)
vmovdqa   %ymm5,32(%r8)
//b[0]
vmovdqa   0(%rdx),%ymm6
//b[1]
vmovdqa   32(%rdx),%ymm7
vmovdqa   %ymm6,256(%r8)
vmovdqa   %ymm7,288(%r8)

//d
fqmul     4,6,3
vmovdqa   %ymm3,512(%r8)
fqmul     5,7,3
vmovdqa   %ymm3,544(%r8)

//a[2]
vmovdqa   64(%rsi),%ymm4
//a[3]
vmovdqa   96(%rsi),%ymm5
vmovdqa   %ymm4,64(%r8)
vmovdqa   %ymm5,96(%r8)
//b[2]
vmovdqa   64(%rdx),%ymm6
//b[3]
vmovdqa   96(%rdx),%ymm7
vmovdqa   %ymm6,320(%r8)
vmovdqa   %ymm7,352(%r8)
fqmul     4,6,3
vmovdqa   %ymm3,576(%r8)
fqmul     5,7,3
vmovdqa   %ymm3,608(%r8)

//a[4]
vmovdqa   128(%rsi),%ymm4
//a[5]
vmovdqa   160(%rsi),%ymm5
vmovdqa   %ymm4,128(%r8)
vmovdqa   %ymm5,160(%r8)
//b[4]
vmovdqa   128(%rdx),%ymm6
//b[5]
vmovdqa   160(%rdx),%ymm7
vmovdqa   %ymm6,384(%r8)
vmovdqa   %ymm7,416(%r8)
fqmul     4,6,3
vmovdqa   %ymm3,640(%r8)
fqmul     5,7,3
vmovdqa   %ymm3,672(%r8)

//a[6]
vmovdqa   192(%rsi),%ymm4
//a[7]
vmovdqa   224(%rsi),%ymm5
vmovdqa   %ymm4,192(%r8)
vmovdqa   %ymm5,224(%r8)
//b[6]
vmovdqa   192(%rdx),%ymm6
//b[7]
vmovdqa   224(%rdx),%ymm7
//b
vmovdqa   %ymm6,448(%r8)
vmovdqa   %ymm7,480(%r8)
fqmul     4,6,3
vmovdqa   %ymm3,704(%r8)
fqmul     5,7,3
vmovdqa   %ymm3,736(%r8)


//c0:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 1,7
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 2,6
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 3,5
vpaddw   %ymm4,%ymm3,%ymm3
vmovdqa  (512+32*4)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3
vmovdqa   (%rcx),%ymm8
fqmul     3,8,3

vmovdqa  (512+32*0)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3

vmovdqa  %ymm3,0(%rdi)
//c1:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 2,7
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 3,6
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 4,5
vpaddw   %ymm4,%ymm3,%ymm3
vmovdqa   (%rcx),%ymm8
fqmul     3,8,3
calc_d 0,1
vpaddw   %ymm4,%ymm3,%ymm3
barret    2,3,3
vmovdqa  %ymm3,32(%rdi)
//c2:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 3,7
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 4,6
vpaddw   %ymm4,%ymm3,%ymm3
vmovdqa  (512+32*5)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3
vmovdqa   (%rcx),%ymm8
fqmul     3,8,3
calc_d 0,2
vpaddw   %ymm4,%ymm3,%ymm3
vmovdqa  (512+32*1)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3

barret    2,3,3
vmovdqa  %ymm3,64(%rdi)
//c3:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 4,7
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 5,6
vpaddw   %ymm4,%ymm3,%ymm3
vmovdqa   (%rcx),%ymm8
fqmul     3,8,3
calc_d 0,3
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 1,2
vpaddw   %ymm4,%ymm3,%ymm3
barret    2,3,3
vmovdqa  %ymm3,96(%rdi)
//c4:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 5,7
vpaddw   %ymm4,%ymm3,%ymm3
vmovdqa  (512+32*6)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3
vmovdqa   (%rcx),%ymm8
fqmul     3,8,3
vmovdqa  (512+32*2)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3

calc_d 0,4
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 1,3
vpaddw   %ymm4,%ymm3,%ymm3
barret    2,3,3
vmovdqa  %ymm3,128(%rdi)
//c5:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 6,7
vpaddw   %ymm4,%ymm3,%ymm3
vmovdqa   (%rcx),%ymm8
fqmul     3,8,3
vpxor     %ymm9,%ymm9,%ymm9
calc_d 0,5
vpaddw   %ymm4,%ymm9,%ymm9
calc_d 1,4
vpaddw   %ymm4,%ymm9,%ymm9
calc_d 2,3
vpaddw   %ymm4,%ymm9,%ymm9
barret    2,9,9
vpaddw   %ymm9,%ymm3,%ymm3
vmovdqa  %ymm3,160(%rdi)
//c6:
vpxor     %ymm3,%ymm3,%ymm3
vmovdqa  (512+32*7)(%r8),%ymm3
vmovdqa   (%rcx),%ymm8
fqmul     3,8,3
vmovdqa  (512+32*3)(%r8),%ymm5
vpaddw   %ymm5,%ymm3,%ymm3

vpxor     %ymm9,%ymm9,%ymm9
calc_d 0,6
vpaddw   %ymm4,%ymm9,%ymm9
calc_d 1,5
vpaddw   %ymm4,%ymm9,%ymm9
calc_d 2,4
vpaddw   %ymm4,%ymm9,%ymm9
barret    2,9,9
vpaddw   %ymm9,%ymm3,%ymm3
vmovdqa  %ymm3,192(%rdi)
//c7:
vpxor     %ymm3,%ymm3,%ymm3
calc_d 0,7
vpaddw   %ymm4,%ymm3,%ymm3
calc_d 1,6
vpaddw   %ymm4,%ymm3,%ymm3
barret    2,3,3
vpxor     %ymm9,%ymm9,%ymm9
calc_d 2,5
vpaddw   %ymm4,%ymm9,%ymm9
calc_d 3,4
vpaddw   %ymm4,%ymm9,%ymm9
barret    2,9,9
vpaddw   %ymm9,%ymm3,%ymm3
vmovdqa  %ymm3,224(%rdi)
ret

.global  basemul_avx
basemul_avx:
mov		%rsp,%r10
mov		%rsp,%rax
and		$31,%rax
sub		%rax,%rsp
sub		$1024,%rsp
mov     %rsp,%r8
#consts
#qinv
vmovdqa   0(%rcx),%ymm0
#q
vmovdqa   32(%rcx),%ymm1
vmovdqa   32(%rcx),%ymm11
vmovdqa   64(%rcx),%ymm2


add		$4928,%rcx
xor		%eax,%eax
_looptop:

call		mul
add		$256,%rdi
add		$256,%rsi
add		$256,%rdx
add		$32,%rcx
add		$128,%eax
cmp		$1024,%eax
jne		_looptop
mov		%r10,%rsp
ret




