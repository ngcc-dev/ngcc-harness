#include "consts.h"
.macro fqmul h,r

vpmullw   %ymm0,%ymm\h,%ymm13
vpmulhw   %ymm2,%ymm\h,%ymm14
vpmulhw %ymm1,%ymm13,%ymm13
vpsubw  %ymm13,%ymm14,%ymm\r

.endm

.text
.global tomont_avx
tomont_avx:
vmovdqa    20032(%rsi),%ymm0
vmovdqa    32(%rsi),%ymm1
vmovdqa    4896(%rsi),%ymm2


xor    %rax,%rax
_loopfm:
vmovdqa    (%rdi),%ymm3
vmovdqa    (32)(%rdi),%ymm4
vmovdqa    (64)(%rdi),%ymm5
vmovdqa    (96)(%rdi),%ymm6
vmovdqa    (128)(%rdi),%ymm7
vmovdqa    (160)(%rdi),%ymm8
vmovdqa    (192)(%rdi),%ymm9
vmovdqa    (224)(%rdi),%ymm10

fqmul      3,11
fqmul      4,3
fqmul      5,4
fqmul      6,5
fqmul      7,6
fqmul      8,7
fqmul      9,8
fqmul      10,9

vmovdqa    %ymm11,(%rdi)
vmovdqa    %ymm3,32(%rdi)
vmovdqa    %ymm4,64(%rdi)
vmovdqa    %ymm5,96(%rdi)
vmovdqa    %ymm6,128(%rdi)
vmovdqa    %ymm7,160(%rdi)
vmovdqa    %ymm8,192(%rdi)
vmovdqa    %ymm9,224(%rdi)


add        $256,%rdi
add        $1,%rax
cmp        $8,%rax
jb         _loopfm

ret
