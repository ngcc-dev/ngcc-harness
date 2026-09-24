#include "consts.h"
 
.macro fqmul1 l1,l2,h,r
#mul
vpmullw		%ymm\l1,%ymm\h,%ymm\r
vpmulhw		%ymm\l2,%ymm\h,%ymm15
#reduce
vpmulhw		%ymm0,%ymm\r,%ymm\r
vpsubw		%ymm\r,%ymm15,%ymm\r
.endm
 
.macro fqmul2 l,h,rh0
#mul
vpmullw		%ymm1,%ymm\l,%ymm15
vpmulhw		%ymm\l,%ymm\h,%ymm14
#reduce
vpmullw		%ymm15,%ymm\h,%ymm15
vpmulhw     %ymm0,%ymm15,%ymm15
vpsubw		%ymm15,%ymm14,%ymm\rh0
.endm

.text
.global cdecl(baseinv_avx)
cdecl(baseinv_avx):
vmovdqa		_16xq(%rip),%ymm0
vmovdqa		_16xqinvv(%rip),%ymm1
vmovdqa     _16xzero(%rip),%ymm13
lea		zetas_baseqinv_exp(%rip),%r8
lea		zetas_base_exp(%rip),%r9

xor     %rax,%rax
xor     %rcx,%rcx
_loop_baseinv:

vmovdqa   (%rsi),%ymm2
vmovdqa    32(%rsi),%ymm3

fqmul2     3,3,4
vmovdqa    (%r8),%ymm8
vmovdqa    (%r9),%ymm9
vpsubw     %ymm8,%ymm13,%ymm8
vpsubw     %ymm9,%ymm13,%ymm9
 
fqmul1     8,9,4,5
fqmul2     2,2,6
vpaddw     %ymm5,%ymm6,%ymm7


fqmul2     7,7,8
fqmul2     7,8,9
fqmul2     8,8,10
fqmul2     9,10,11
fqmul2     10,10,7
fqmul2     11,7,8
fqmul2     7,7,9
fqmul2     9,8,10
fqmul2     9,9,11
fqmul2     10,11,7
fqmul2     11,11,8
fqmul2     7,8,9
fqmul2     8,8,10
fqmul2     10,10,11
fqmul2     9,11,7
fqmul2     11,11,8
fqmul2     8,8,10
fqmul2     7,10,9
fqmul2     10,10,11
fqmul2     9,11,8
# y8 stores fqinv(det)
# check for invertibility
vpxor		%ymm7,%ymm7,%ymm7
vpcmpeqw	%ymm7,%ymm8,%ymm2
vextractf128   $1,%ymm2,%xmm3
por		%xmm3,%xmm2
vpermq  $0x00,%ymm2,%ymm3
vpermq  $0x01,%ymm2,%ymm2
por     %xmm3,%xmm2
vpshufd    $0x00,%ymm2,%ymm3
vpshufd    $0x01,%ymm2,%ymm2
por     %xmm3,%xmm2
vpextrd		$0,%xmm2,%r10d
or		    %r10d,%eax


fqmul2     2,8,9
vpsubw     %ymm3,%ymm13,%ymm3
fqmul2     3,8,10

vmovdqa		%ymm9,(%rdi)
vmovdqa		%ymm10,32(%rdi)



add    $64,%rsi
add    $64,%rdi
add    $32,%rdx
add    $32,%r8
add    $32,%r9
add    $1,%rcx
cmp    $24,%rcx
jb     _loop_baseinv

ret
