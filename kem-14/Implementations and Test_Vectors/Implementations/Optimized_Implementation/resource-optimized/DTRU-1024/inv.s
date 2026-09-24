.macro  mul64 l,h,r,a
vpsllq    $32,%ymm\l,%ymm9
vpsrlq    $32,%ymm9,%ymm9
vpsrlq    $32,%ymm\l,%ymm10
vpmuludq   %ymm7,%ymm9,%ymm9
vpmuludq   %ymm7,%ymm10,%ymm10
vpsrlq    $31,%ymm9,%ymm9
vpsrlq    $31,%ymm10,%ymm10
vpsllq    $32,%ymm10,%ymm10
vpblendd  $0x55,%ymm9,%ymm10,%ymm\r
vpsllq    $32,%ymm\h,%ymm9
vpsrlq    $32,%ymm9,%ymm9
vpsrlq    $32,%ymm\h,%ymm10
vpmuludq   %ymm7,%ymm9,%ymm9
vpmuludq   %ymm7,%ymm10,%ymm10
vpsrlq    $31,%ymm9,%ymm9
vpsrlq    $31,%ymm10,%ymm10
vpsllq    $32,%ymm10,%ymm10
vpblendd  $0x55,%ymm9,%ymm10,%ymm\a
.endm
.macro fq_freeze i

//phi[i]
vmovdqa   (0+32*\i)(%r8),%ymm5
//F[i]
vmovdqa   (288+32*\i)(%r8),%ymm6

vpslld $16,%ymm5,%ymm7
vpsrad $16,%ymm7,%ymm7
vpsrad $16,%ymm5,%ymm5

vpslld $16,%ymm6,%ymm8
vpsrad $16,%ymm8,%ymm8
vpsrad $16,%ymm6,%ymm6
//Phi0 * F[i]
vpmulld  %ymm3,%ymm8,%ymm8
vpmulld  %ymm1,%ymm6,%ymm6
//F0 * Phi[i]
vpmulld  %ymm4,%ymm7,%ymm7
vpmulld  %ymm2,%ymm5,%ymm5
 
//fq_freeze(x)
vpsubd   %ymm7,%ymm8,%ymm7
vpsubd   %ymm5,%ymm6,%ymm5

//8xhalfq
vmovdqa   416(%rcx),%ymm6
//x+halfq
vpaddd %ymm7,%ymm6,%ymm8
vpaddd %ymm5,%ymm6,%ymm6
//8 x 0x80000000
vmovdqa   288(%rcx),%ymm5
//x = 0x80000000+x
vpaddd   %ymm5,%ymm8,%ymm8
vpaddd   %ymm5,%ymm6,%ymm6
//4 x w = w/m 
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9

//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12


//x - qpart*m

vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6


//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
//qpart*m
vpsrld $16,%ymm0,%ymm9
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10
//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-=m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//8 x 1 y+1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7
vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7
//uq = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
vpsrad $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6
//ur
vpslld    $16,%ymm6,%ymm6
vpblendw  $0x55,%ymm8,%ymm6,%ymm6
vmovdqa   %ymm6,1312(%r8)
//uq
vmovdqa   %ymm13,1344(%r8)
vmovdqa   %ymm12,1376(%r8)

///////
//8 x 0x80000000
vmovdqa   288(%rcx),%ymm5
vpsrld  $0,%ymm5,%ymm8
vpsrld  $0,%ymm5,%ymm6
//4 x w = w/m
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12

//x - qpart*m
vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6
//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10

//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//8 x 1 y+=1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7

//uq2 = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

vpsrld $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur2 = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6


//ur 
vmovdqa   1312(%r8),%ymm7
vpslld    $16,%ymm7,%ymm9
vpsrld    $16,%ymm9,%ymm9
vpsrld    $16,%ymm7,%ymm7
//uq
vmovdqa   1344(%r8),%ymm11
vmovdqa   1376(%r8),%ymm10

//ur-ur2
vpsubd %ymm8,%ymm9,%ymm9
vpsubd %ymm6,%ymm7,%ymm7

//uq-uq2
vpsubd %ymm13,%ymm11,%ymm11
vpsubd %ymm12,%ymm10,%ymm10
//mask
vpsrld $31,%ymm9,%ymm8
vpsrld $31,%ymm7,%ymm6

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm8,%ymm5,%ymm8
vpsubd  %ymm6,%ymm5,%ymm6

//y = uq+=mask
vpaddd %ymm8,%ymm11,%ymm11
vpaddd %ymm6,%ymm10,%ymm10


//mask&m
vpsrld $16,%ymm0,%ymm14
vpsrld $0,%ymm14,%ymm15
vpand   %ymm8,%ymm15,%ymm15
vpand   %ymm6,%ymm14,%ymm14

//r = ur += mask&m  
vpaddd %ymm9,%ymm15,%ymm9
vpaddd %ymm7,%ymm14,%ymm7
vpslld    $16,%ymm7,%ymm7
//r
vpblendw  $0x55,%ymm9,%ymm7,%ymm9
//16 x halfq
vmovdqa 384(%rcx),%ymm6
vpsubw  %ymm6,%ymm9,%ymm6
vmovdqa   %ymm6,(288+32*\i)(%r8)
.endm

.macro loop i

//phi[i]
vmovdqa   (32*\i)(%r8),%ymm2
//F[i]
vmovdqa   (288+32*\i)(%r8),%ymm3
vpxor    %ymm2,%ymm3,%ymm4
//swap
vmovdqa  1280(%r8),%ymm1
//t
vpand    %ymm4,%ymm1,%ymm4

//Phi[i] ^= t
vpxor    %ymm2,%ymm4,%ymm2

//F[i]^=t
vpxor    %ymm3,%ymm4,%ymm3


//vmovdqa   %ymm3,32(%rdi)
//Phi[i]
vmovdqa   %ymm2,(32*\i)(%r8)
//F[i]
vmovdqa   %ymm3,(288+32*\i)(%r8)

////////////////////////////////////////////////
//V[i]
vmovdqa   (576+32*\i)(%r8),%ymm2
//S[i]
vmovdqa   (864+32*\i)(%r8),%ymm3
vpxor    %ymm2,%ymm3,%ymm4
//swap
vmovdqa  1280(%r8),%ymm1
//t
vpand    %ymm4,%ymm1,%ymm4

vpxor    %ymm2,%ymm4,%ymm2

vpxor    %ymm3,%ymm4,%ymm3
//V[i]
vmovdqa   %ymm2,(576+32*\i)(%r8)
//S[i]
vmovdqa   %ymm3,(864+32*\i)(%r8)
.endm
.macro fq_freeze1 i
//V[i]
vmovdqa   (576+32*\i)(%r8),%ymm5
//S[i]
vmovdqa   (864+32*\i)(%r8),%ymm6

vpslld $16,%ymm5,%ymm7
vpsrad $16,%ymm7,%ymm7
vpsrad $16,%ymm5,%ymm5

vpslld $16,%ymm6,%ymm8
vpsrad $16,%ymm8,%ymm8
vpsrad $16,%ymm6,%ymm6

vpmulld  %ymm3,%ymm8,%ymm8
vpmulld  %ymm1,%ymm6,%ymm6
vpmulld  %ymm4,%ymm7,%ymm7
vpmulld  %ymm2,%ymm5,%ymm5
//fq_freeze(x)
vpsubd   %ymm7,%ymm8,%ymm7
vpsubd   %ymm5,%ymm6,%ymm5
//8xhalfq
vmovdqa   416(%rcx),%ymm6
//x+halfq
vpaddd %ymm7,%ymm6,%ymm8
vpaddd %ymm5,%ymm6,%ymm6
//8 x 0x80000000
vmovdqa   288(%rcx),%ymm5
//x = 0x80000000+x
vpaddd   %ymm5,%ymm8,%ymm8
vpaddd   %ymm5,%ymm6,%ymm6
//4 x w = w/m 
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9

//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12


//x - qpart*m

vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6


//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
//qpart*m
vpsrld $16,%ymm0,%ymm9
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10
//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-=m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//y+1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7
vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7
//uq = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
vpsrad $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6
//ur
vpslld    $16,%ymm6,%ymm6
vpblendw  $0x55,%ymm8,%ymm6,%ymm6
vmovdqa   %ymm6,1312(%r8)
//uq
vmovdqa   %ymm13,1344(%r8)
vmovdqa   %ymm12,1376(%r8)

///////
//0x80000000
vmovdqa   288(%rcx),%ymm5
vpsrld  $0,%ymm5,%ymm8
vpsrld  $0,%ymm5,%ymm6
//w = w/m
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12

//x - qpart*m
vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6
//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10

//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//y+=1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7

//uq2 = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

vpsrld $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur2 = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6


//ur 
vmovdqa   1312(%r8),%ymm7
vpslld    $16,%ymm7,%ymm9
vpsrld    $16,%ymm9,%ymm9
vpsrld    $16,%ymm7,%ymm7
//uq
vmovdqa   1344(%r8),%ymm11
vmovdqa   1376(%r8),%ymm10

//ur-ur2
vpsubd %ymm8,%ymm9,%ymm9
vpsubd %ymm6,%ymm7,%ymm7

//uq-uq2
vpsubd %ymm13,%ymm11,%ymm11
vpsubd %ymm12,%ymm10,%ymm10
//mask
vpsrld $31,%ymm9,%ymm8
vpsrld $31,%ymm7,%ymm6

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm8,%ymm5,%ymm8
vpsubd  %ymm6,%ymm5,%ymm6

//y = uq+=mask
vpaddd %ymm8,%ymm11,%ymm11
vpaddd %ymm6,%ymm10,%ymm10


//mask&m
vpsrld $16,%ymm0,%ymm14
vpsrld $0,%ymm14,%ymm15
vpand   %ymm8,%ymm15,%ymm15
vpand   %ymm6,%ymm14,%ymm14

//r = ur += mask&m  
vpaddd %ymm9,%ymm15,%ymm9
vpaddd %ymm7,%ymm14,%ymm7
vpslld    $16,%ymm7,%ymm7
vpblendw  $0x55,%ymm9,%ymm7,%ymm9
//16 x halfq
vmovdqa 384(%rcx),%ymm6
vpsubw  %ymm6,%ymm9,%ymm6
vmovdqa   %ymm6,(864+32*\i)(%r8)

.endm
.macro fmull
//a*t
vpmulld %ymm1,%ymm3,%ymm3
vpmulld %ymm2,%ymm4,%ymm4
.endm

.macro fmull_sq
//a*a
vpmulld %ymm1,%ymm1,%ymm1
vpmulld %ymm2,%ymm2,%ymm2
.endm
.macro fq_freeze2a aa bb
vpsrad $0,%ymm\aa,%ymm7
vpsrad $0,%ymm\bb,%ymm5
///////////////////////////////////////////444////////////////////////////

//8xhalfq
vmovdqa   416(%rcx),%ymm6
//x+halfq
vpaddd %ymm7,%ymm6,%ymm8
vpaddd %ymm5,%ymm6,%ymm6
//8 x 0x80000000
vmovdqa   288(%rcx),%ymm5
//x = 0x80000000+x
vpaddd   %ymm5,%ymm8,%ymm8
vpaddd   %ymm5,%ymm6,%ymm6
//4 x w = w/m 
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9

//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12


//x - qpart*m

vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6


//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
//qpart*m
vpsrld $16,%ymm0,%ymm9
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10
//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-=m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//y+1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7
vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7
//uq = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
vpsrad $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6
//ur
vpslld    $16,%ymm6,%ymm6
vpblendw  $0x55,%ymm8,%ymm6,%ymm6
vmovdqa   %ymm6,1312(%r8)
//uq
vmovdqa   %ymm13,1344(%r8)
vmovdqa   %ymm12,1376(%r8)

///////
//0x80000000
vmovdqa   288(%rcx),%ymm5
vpsrld  $0,%ymm5,%ymm8
vpsrld  $0,%ymm5,%ymm6
//w = w/m
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12

//x - qpart*m
vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6
//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10

//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//y+=1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7

//uq2 = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

vpsrld $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur2 = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6


//ur 
vmovdqa   1312(%r8),%ymm7
vpslld    $16,%ymm7,%ymm9
vpsrld    $16,%ymm9,%ymm9
vpsrld    $16,%ymm7,%ymm7
//uq
vmovdqa   1344(%r8),%ymm11
vmovdqa   1376(%r8),%ymm10

//ur-ur2
vpsubd %ymm8,%ymm9,%ymm9
vpsubd %ymm6,%ymm7,%ymm7

//uq-uq2
vpsubd %ymm13,%ymm11,%ymm11
vpsubd %ymm12,%ymm10,%ymm10
//mask
vpsrld $31,%ymm9,%ymm8
vpsrld $31,%ymm7,%ymm6

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm8,%ymm5,%ymm8
vpsubd  %ymm6,%ymm5,%ymm6

//y = uq+=mask
vpaddd %ymm8,%ymm11,%ymm11
vpaddd %ymm6,%ymm10,%ymm10


//mask&m
vpsrld $16,%ymm0,%ymm14
vpsrld $0,%ymm14,%ymm15
vpand   %ymm8,%ymm15,%ymm15
vpand   %ymm6,%ymm14,%ymm14

//r = ur += mask&m  
vpaddd %ymm9,%ymm15,%ymm9
vpaddd %ymm7,%ymm14,%ymm7

//8 x halfq
vmovdqa 416(%rcx),%ymm6
vpsubd  %ymm6,%ymm9,%ymm1
vpsubd  %ymm6,%ymm7,%ymm2

.endm



////////////////////////////////////////////////////////////////////////////
.macro fq_freeze2t aa bb
vpsrad $0,%ymm\aa,%ymm7
vpsrad $0,%ymm\bb,%ymm5


//8xhalfq
vmovdqa   416(%rcx),%ymm6
//x+halfq
vpaddd %ymm7,%ymm6,%ymm8
vpaddd %ymm5,%ymm6,%ymm6
//8 x 0x80000000
vmovdqa   288(%rcx),%ymm5
//x = 0x80000000+x
vpaddd   %ymm5,%ymm8,%ymm8
vpaddd   %ymm5,%ymm6,%ymm6
//4 x w = w/m 
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9

//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12


//x - qpart*m

vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6


//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
//qpart*m
vpsrld $16,%ymm0,%ymm9
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10
//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-=m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//y+1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7
vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7
//uq = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
vpsrad $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6
//ur
vpslld    $16,%ymm6,%ymm6
vpblendw  $0x55,%ymm8,%ymm6,%ymm6
vmovdqa   %ymm6,1312(%r8)
//uq
vmovdqa   %ymm13,1344(%r8)
vmovdqa   %ymm12,1376(%r8)

///////
//0x80000000
vmovdqa   288(%rcx),%ymm5
vpsrld  $0,%ymm5,%ymm8
vpsrld  $0,%ymm5,%ymm6
//w = w/m
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12

//x - qpart*m
vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6
//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10

//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//y+=1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7

//uq2 = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

vpsrld $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur2 = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6


//ur 
vmovdqa   1312(%r8),%ymm7
vpslld    $16,%ymm7,%ymm9
vpsrld    $16,%ymm9,%ymm9
vpsrld    $16,%ymm7,%ymm7
//uq
vmovdqa   1344(%r8),%ymm11
vmovdqa   1376(%r8),%ymm10

//ur-ur2
vpsubd %ymm8,%ymm9,%ymm9
vpsubd %ymm6,%ymm7,%ymm7

//uq-uq2
vpsubd %ymm13,%ymm11,%ymm11
vpsubd %ymm12,%ymm10,%ymm10
//mask
vpsrld $31,%ymm9,%ymm8
vpsrld $31,%ymm7,%ymm6

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm8,%ymm5,%ymm8
vpsubd  %ymm6,%ymm5,%ymm6

//y = uq+=mask
vpaddd %ymm8,%ymm11,%ymm11
vpaddd %ymm6,%ymm10,%ymm10


//mask&m
vpsrld $16,%ymm0,%ymm14
vpsrld $0,%ymm14,%ymm15
vpand   %ymm8,%ymm15,%ymm15
vpand   %ymm6,%ymm14,%ymm14

//r = ur += mask&m  
vpaddd %ymm9,%ymm15,%ymm9
vpaddd %ymm7,%ymm14,%ymm7

//halfq
vmovdqa 416(%rcx),%ymm6
vpsubd  %ymm6,%ymm9,%ymm3
vpsubd  %ymm6,%ymm7,%ymm4
.endm

.macro fq_freeze3 
/////////////////////////////////////////////////////
//8xhalfq
vmovdqa   416(%rcx),%ymm6
//x+halfq
vpaddd %ymm7,%ymm6,%ymm8
vpaddd %ymm5,%ymm6,%ymm6
//8 x 0x80000000
vmovdqa   288(%rcx),%ymm5
//x = 0x80000000+x
vpaddd   %ymm5,%ymm8,%ymm8
vpaddd   %ymm5,%ymm6,%ymm6
//4 x w = w/m 
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9

//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12


//x - qpart*m

vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6


//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
//qpart*m
vpsrld $16,%ymm0,%ymm9
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10
//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-=m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//y+1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7
vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7
//uq = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
vpsrad $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6
//ur
vpslld    $16,%ymm6,%ymm6
vpblendw  $0x55,%ymm8,%ymm6,%ymm6
vmovdqa   %ymm6,1312(%r8)
//uq
vmovdqa   %ymm13,1344(%r8)
vmovdqa   %ymm12,1376(%r8)

///////
//0x80000000
vmovdqa   288(%rcx),%ymm5
vpsrld  $0,%ymm5,%ymm8
vpsrld  $0,%ymm5,%ymm6
//w = w/m
vmovdqa   352(%rcx),%ymm7
//qpart x*w
mul64 8,6,11,10
//m
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm13
vpmulld %ymm10,%ymm9,%ymm12

//x - qpart*m
vpsubd  %ymm13,%ymm8,%ymm8
vpsubd  %ymm12,%ymm6,%ymm6
//y+=qpart 12,13
vpsrld $0,%ymm11,%ymm13
vpsrld $0,%ymm10,%ymm12

//qpart x*w
mul64 8,6,11,10
//y+=qpart
vpaddd %ymm11,%ymm13,%ymm13
vpaddd %ymm10,%ymm12,%ymm12
vpsrld $16,%ymm0,%ymm9
//qpart*m
vpmulld %ymm11,%ymm9,%ymm11
vpmulld %ymm10,%ymm9,%ymm10

//x - qpart*m
vpsubd  %ymm11,%ymm8,%ymm8
vpsubd  %ymm10,%ymm6,%ymm6

//x-m
vpsubd  %ymm9,%ymm8,%ymm8
vpsubd  %ymm9,%ymm6,%ymm6

//y+=1
vmovdqa   256(%rcx),%ymm7
vpaddd %ymm7,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12
//x>>31
vpsrld  $31,%ymm8,%ymm9
vpsrld  $31,%ymm6,%ymm7

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm9,%ymm5,%ymm9
vpsubd  %ymm7,%ymm5,%ymm7

//uq2 = y+=mask
vpaddd %ymm9,%ymm13,%ymm13
vpaddd %ymm7,%ymm12,%ymm12

vpsrld $16,%ymm0,%ymm10
//mask&m
vpand   %ymm9,%ymm10,%ymm9
vpand   %ymm7,%ymm10,%ymm7

//ur2 = x+= mask&m
vpaddd %ymm9,%ymm8,%ymm8
vpaddd %ymm7,%ymm6,%ymm6


//ur 
vmovdqa   1312(%r8),%ymm7
vpslld    $16,%ymm7,%ymm9
vpsrld    $16,%ymm9,%ymm9
vpsrld    $16,%ymm7,%ymm7
//uq
vmovdqa   1344(%r8),%ymm11
vmovdqa   1376(%r8),%ymm10

//ur-ur2
vpsubd %ymm8,%ymm9,%ymm9
vpsubd %ymm6,%ymm7,%ymm7

//uq-uq2
vpsubd %ymm13,%ymm11,%ymm11
vpsubd %ymm12,%ymm10,%ymm10
//mask
vpsrld $31,%ymm9,%ymm8
vpsrld $31,%ymm7,%ymm6

vpxor   %ymm5,%ymm5,%ymm5
//mask
vpsubd  %ymm8,%ymm5,%ymm8
vpsubd  %ymm6,%ymm5,%ymm6

//y = uq+=mask
vpaddd %ymm8,%ymm11,%ymm11
vpaddd %ymm6,%ymm10,%ymm10

//mask&m
vpsrld $16,%ymm0,%ymm14
vpsrld $0,%ymm14,%ymm15
vpand   %ymm8,%ymm15,%ymm15
vpand   %ymm6,%ymm14,%ymm14

//r = ur += mask&m  
vpaddd %ymm9,%ymm15,%ymm9
vpaddd %ymm7,%ymm14,%ymm7

vpslld    $16,%ymm7,%ymm7
vpblendw  $0x55,%ymm9,%ymm7,%ymm9
//16 x halfq
vmovdqa 384(%rcx),%ymm6
vpsubw  %ymm6,%ymm9,%ymm6
.endm

.macro fq_inverse
//a
vmovdqa   0(%r8),%ymm2
vpslld  $16,%ymm2,%ymm1
vpsrad  $16,%ymm1,%ymm1
vpsrad  $16,%ymm2,%ymm2
//t
vpsrad  $0,%ymm1,%ymm3
vpsrad  $0,%ymm2,%ymm4
//1 2 3
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
//2 4 7
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
//3 8 15
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
//4 16 31
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
//5 32 63
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
//6 64 127
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
//7 128 127
fmull_sq
fq_freeze2a 1 2
//8 256 383
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
//9 512 383
fmull_sq
fq_freeze2a 1 2
//10 1024 1407
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
//11 2048 3455
fmull_sq
fq_freeze2a 1 2
fmull
fq_freeze2t 3 4
.endm

.macro loop8 i
//V[i]
vmovdqa   (800-32*\i)(%r8),%ymm5

vpslld $16,%ymm5,%ymm7
vpsrad $16,%ymm7,%ymm7
vpsrad $16,%ymm5,%ymm5
/*

vpslld $16,%ymm1,%ymm1
vpsrad $16,%ymm1,%ymm1
vpslld $16,%ymm2,%ymm2
vpsrad $16,%ymm2,%ymm2
*/
vpmulld %ymm1,%ymm7,%ymm7
vpmulld %ymm2,%ymm5,%ymm5
fq_freeze3
vmovdqa   %ymm6,(32*\i)(%rdi)
.endm


.macro loop15 i
//loop15:
//012345678->123456780
vmovdqa   800(%r8),%ymm1
vmovdqa   %ymm1,832(%r8)

vmovdqa   768(%r8),%ymm1
vmovdqa   %ymm1,800(%r8)

vmovdqa   736(%r8),%ymm1
vmovdqa   %ymm1,768(%r8)

vmovdqa   704(%r8),%ymm1
vmovdqa   %ymm1,736(%r8)

vmovdqa   672(%r8),%ymm1
vmovdqa   %ymm1,704(%r8)

vmovdqa   640(%r8),%ymm1
vmovdqa   %ymm1,672(%r8)

vmovdqa   608(%r8),%ymm1
vmovdqa   %ymm1,640(%r8)

vmovdqa   576(%r8),%ymm1
vmovdqa   %ymm1,608(%r8)

vpxor     %ymm1,%ymm1,%ymm1
vmovdqa   %ymm1,576(%r8)


//Delta
vmovdqa   1152(%r8),%ymm1
vmovdqa   1184(%r8),%ymm3
vpxor     %ymm2,%ymm2,%ymm2
//-Delta
vpsubd    %ymm1,%ymm2,%ymm1
vpsubd    %ymm3,%ymm2,%ymm3
//uint16_t
vpslld   $16,%ymm3,%ymm3
vpblendw $0x55,%ymm1, %ymm3, %ymm3

//int16_negative_mask
vpsrlw $15,%ymm3,%ymm3
vpslld $16,%ymm3,%ymm4
vpsrld $16,%ymm4,%ymm4
vpsrld $16,%ymm3,%ymm3
//3是高位
vpxor     %ymm2,%ymm2,%ymm2
vpsubd    %ymm4,%ymm2,%ymm4
vpsubd    %ymm3,%ymm2,%ymm3

//F[0]
vmovdqa  288(%r8),%ymm1

//int16_nonzero_mask
vpslld   $16,%ymm1,%ymm2
vpsrld   $16,%ymm2,%ymm2
vpsrld   $16,%ymm1,%ymm1

//
vpxor     %ymm5,%ymm5,%ymm5
vpsubd    %ymm2,%ymm5,%ymm2
vpsubd    %ymm1,%ymm5,%ymm1

vpsrld   $31,%ymm2,%ymm2
vpsrld   $31,%ymm1,%ymm1


vpsubd    %ymm2,%ymm5,%ymm2
vpsubd    %ymm1,%ymm5,%ymm1

//swap
vpand     %ymm2,%ymm4,%ymm2
vpand     %ymm1,%ymm3,%ymm1
vmovdqa  %ymm2,1216(%r8)
vmovdqa  %ymm1,1248(%r8)

vpslld   $16,%ymm1,%ymm1
//16bit swap
vpblendw $0x55,%ymm2, %ymm1, %ymm1
vmovdqa  %ymm1,1280(%r8)

loop 0
loop 1
loop 2
loop 3
loop 4
loop 5
loop 6
loop 7
loop 8
//delta

vmovdqa   1152(%r8),%ymm2
vmovdqa   1184(%r8),%ymm3
//-delta
vpxor     %ymm1,%ymm1,%ymm1
vpsubd    %ymm2,%ymm1,%ymm4
vpsubd    %ymm3,%ymm1,%ymm5
//delta^-delta
vpxor     %ymm4,%ymm2,%ymm2
vpxor     %ymm5,%ymm3,%ymm3
//swap
vmovdqa  1216(%r8),%ymm1
vmovdqa  1248(%r8),%ymm4
//swap&delta^-delta
vpand     %ymm1,%ymm2,%ymm2
vpand     %ymm4,%ymm3,%ymm3
//delta
vmovdqa   1152(%r8),%ymm1
vmovdqa   1184(%r8),%ymm4
//delta^=swap&delta^-delta
vpxor     %ymm1,%ymm2,%ymm2
vpxor     %ymm4,%ymm3,%ymm3
//8x1
vmovdqa   256(%rcx),%ymm1
vpaddd    %ymm1,%ymm2,%ymm2
vpaddd    %ymm1,%ymm3,%ymm3
//delta
vmovdqa   %ymm2,1152(%r8)
vmovdqa   %ymm3,1184(%r8)


//Phi0
vmovdqa   0(%r8),%ymm1
//F0
vmovdqa   288(%r8),%ymm2


vpslld $16,%ymm1,%ymm3
vpsrad $16,%ymm3,%ymm3
vpsrad $16,%ymm1,%ymm1

vpslld $16,%ymm2,%ymm4
vpsrad $16,%ymm4,%ymm4
vpsrad $16,%ymm2,%ymm2


fq_freeze 0
fq_freeze 1
fq_freeze 2
fq_freeze 3
fq_freeze 4
fq_freeze 5
fq_freeze 6
fq_freeze 7
fq_freeze 8

vmovdqa   320(%r8),%ymm10
vmovdqa   %ymm10,288(%r8)
vmovdqa   352(%r8),%ymm10
vmovdqa   %ymm10,320(%r8)
vmovdqa   384(%r8),%ymm10
vmovdqa   %ymm10,352(%r8)
vmovdqa   416(%r8),%ymm10
vmovdqa   %ymm10,384(%r8)
vmovdqa   448(%r8),%ymm10
vmovdqa   %ymm10,416(%r8)
vmovdqa   480(%r8),%ymm10
vmovdqa   %ymm10,448(%r8)
vmovdqa   512(%r8),%ymm10
vmovdqa   %ymm10,480(%r8)
vmovdqa   544(%r8),%ymm10
vmovdqa   %ymm10,512(%r8)
vpxor     %ymm10,%ymm10,%ymm10
vmovdqa   %ymm10,544(%r8)


fq_freeze1 0
fq_freeze1 1
fq_freeze1 2
fq_freeze1 3
fq_freeze1 4
fq_freeze1 5
fq_freeze1 6
fq_freeze1 7
fq_freeze1 8
.endm

/////

.text

rq_inverse: 
.p2align 5
//Phi=0+32*i
//phi0
vmovdqa   448(%rcx),%ymm1
vmovdqa   %ymm1,0(%r8)

//phi1-7
vpxor     %ymm1,%ymm1,%ymm1
vmovdqa   %ymm1,32(%r8)
vmovdqa   %ymm1,64(%r8)
vmovdqa   %ymm1,96(%r8)
vmovdqa   %ymm1,128(%r8)
vmovdqa   %ymm1,160(%r8)
vmovdqa   %ymm1,192(%r8)
vmovdqa   %ymm1,224(%r8)

//phi8 =-zeta
vmovdqa   (%rdx),%ymm1
vmovdqa   %ymm1,256(%r8)
//F=288+32*i
//F0-7
vmovdqa   (%rsi),%ymm1
vmovdqa   %ymm1,512(%r8)
vmovdqa   32(%rsi),%ymm1
vmovdqa   %ymm1,480(%r8)
vmovdqa   64(%rsi),%ymm1
vmovdqa   %ymm1,448(%r8)
vmovdqa   96(%rsi),%ymm1
vmovdqa   %ymm1,416(%r8)
vmovdqa   128(%rsi),%ymm1
vmovdqa   %ymm1,384(%r8)
vmovdqa   160(%rsi),%ymm1
vmovdqa   %ymm1,352(%r8)
vmovdqa   192(%rsi),%ymm1
vmovdqa   %ymm1,320(%r8)
vmovdqa   224(%rsi),%ymm1
vmovdqa   %ymm1,288(%r8)
//F8
vpxor     %ymm1,%ymm1,%ymm1
vmovdqa   %ymm1,544(%r8)

//V=576+32*i
//V0-8
vpxor     %ymm1,%ymm1,%ymm1
vmovdqa   %ymm1,576(%r8)
vmovdqa   %ymm1,608(%r8)
vmovdqa   %ymm1,640(%r8)
vmovdqa   %ymm1,672(%r8)
vmovdqa   %ymm1,704(%r8)
vmovdqa   %ymm1,736(%r8)
vmovdqa   %ymm1,768(%r8)
vmovdqa   %ymm1,800(%r8)
vmovdqa   %ymm1,832(%r8)
//S=864+32*i
//S0
vmovdqa   448(%rcx),%ymm1
vmovdqa   %ymm1,864(%r8)
//S1-8
vpxor     %ymm1,%ymm1,%ymm1
vmovdqa   %ymm1,896(%r8)
vmovdqa   %ymm1,928(%r8)
vmovdqa   %ymm1,960(%r8)
vmovdqa   %ymm1,992(%r8)
vmovdqa   %ymm1,1024(%r8)
vmovdqa   %ymm1,1056(%r8)
vmovdqa   %ymm1,1088(%r8)
vmovdqa   %ymm1,1120(%r8)
//8 x Delta = 1
vmovdqa   256(%rcx),%ymm1
vmovdqa   %ymm1,1152(%r8)
vmovdqa   %ymm1,1184(%r8)

loop15 0
loop15 1
loop15 2
loop15 3
loop15 4
loop15 5
loop15 6
loop15 7
loop15 8
loop15 9
loop15 10
loop15 11
loop15 12
loop15 13
loop15 14

fq_inverse

//scale
vpsrad $0,%ymm3,%ymm1
vpsrad $0,%ymm4,%ymm2

loop8 0
loop8 1
loop8 2
loop8 3
loop8 4
loop8 5
loop8 6
loop8 7

//check
//delta

vmovdqa  1152(%r8),%ymm2
vmovdqa  1184(%r8),%ymm1
vpslld   $16,%ymm1,%ymm1
/*
vpblendw $0x55,%ymm2, %ymm1, %ymm1
//int16_nonzero_mask
vpslld   $16,%ymm1,%ymm2
vpsrad   $16,%ymm2,%ymm2
vpsrad   $16,%ymm1,%ymm1
*/
//
vpxor     %ymm5,%ymm5,%ymm5
vpsubd    %ymm2,%ymm5,%ymm2
vpsubd    %ymm1,%ymm5,%ymm1

vpsrld   $31,%ymm2,%ymm2
vpsrld   $31,%ymm1,%ymm1


vpsubd    %ymm2,%ymm5,%ymm2
vpsubd    %ymm1,%ymm5,%ymm1

vpaddd    %ymm2,%ymm1,%ymm9


vextractf128   $1,%ymm9,%xmm10
vpaddd		   %ymm10,%ymm9,%ymm9


vpermq  $0x00,%ymm9,%ymm10
vpermq  $0x01,%ymm9,%ymm9
vpaddd  %ymm10,%ymm9,%ymm9

vpshufd    $0x00,%ymm9,%ymm10
vpshufd    $0x01,%ymm9,%ymm9
vpaddd  %ymm10,%ymm9,%ymm9

vpextrd		$0,%xmm9,%r9d
mov         1408(%r8),%eax
add         %r9d,%eax
mov         %eax,1408(%r8)
ret


.global rq_inverse_avx
rq_inverse_avx:
.p2align 5
mov		%rsp,%r10
mov		%rsp,%rax
and		$31,%rax
sub		%rax,%rsp
sub		$4096,%rsp
mov     %rsp,%r8

#q
vmovdqa 32(%rdx),%ymm0
add		$10208,%rdx
xor		%eax,%eax
mov     %eax,1408(%r8)
xor     %r11,%r11
mov     %rdx,%rcx

_looptop:
call    rq_inverse
add		$256,%rdi
add		$256,%rsi
add		$32,%rdx
add		$128,%r11
cmp		$1024,%r11 
jne		_looptop
mov     1408(%r8),%eax
mov		%r10,%rsp
ret
