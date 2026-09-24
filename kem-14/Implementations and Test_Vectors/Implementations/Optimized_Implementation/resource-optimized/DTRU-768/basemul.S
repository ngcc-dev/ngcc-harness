#include "consts.h"
# compute mont_reduce(a*zetas)
.macro fqmul1 l1,l2,h,r
#mul
vpmullw		%ymm\l1,%ymm\h,%ymm\r
vpmulhw		%ymm\l2,%ymm\h,%ymm15
#reduce
vpmulhw		%ymm0,%ymm\r,%ymm\r
vpsubw		%ymm\r,%ymm15,%ymm\r
.endm
# compute mont_reduce(a*b)
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
.global cdecl(basemul_avx)
cdecl(basemul_avx):
vmovdqa		_16xq(%rip),%ymm0
vmovdqa		_16xqinvv(%rip),%ymm1
lea		zetas_baseqinv_exp(%rip),%r8
lea		zetas_base_exp(%rip),%r9


xor     %rax,%rax
_loop_basemul:
 
vmovdqa   (%rsi),%ymm2
vmovdqa    32(%rsi),%ymm3
 
vmovdqa    (%rdx),%ymm4
vmovdqa    32(%rdx),%ymm5
 
fqmul2     2,4,6
 
fqmul2     3,5,7
 
vmovdqa    (%r8),%ymm8
vmovdqa    (%r9),%ymm9
fqmul1     8,9,7,10
 
vpaddw      %ymm10,%ymm6,%ymm10


 
vpaddw     %ymm2,%ymm3,%ymm3
 
vpaddw     %ymm4,%ymm5,%ymm5
 
fqmul2     3,5,11
 
vpaddw     %ymm6,%ymm7,%ymm7
vpsubw     %ymm7,%ymm11,%ymm11

vmovdqa		%ymm10,(%rdi)
vmovdqa		%ymm11,32(%rdi)

add    $64,%rsi
add    $64,%rdx
add    $64,%rdi
add    $32,%r8
add    $32,%r9
add    $1,%rax
cmp    $24,%rax
jb     _loop_basemul

ret
